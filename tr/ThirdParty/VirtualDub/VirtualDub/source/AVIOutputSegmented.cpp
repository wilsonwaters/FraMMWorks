//	VirtualDub - Video processing and capture application
//	Copyright (C) 1998-2004 Avery Lee
//
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "stdafx.h"
#include <list>
#include <vd2/system/file.h>
#include <vd2/system/error.h>
#include <vd2/system/math.h>
#include "DubOutput.h"
#include "DubUtils.h"
#include "AVIOutput.h"

//////////////////////////////////////////////////////////////////////////////////////////////

class VDChunkedBuffer {
public:
	VDChunkedBuffer();
	~VDChunkedBuffer();

	void Write(const void *data, uint32 size);
	const void *LockRead(uint32 size, uint32& actual);
	void UnlockRead(uint32 size);

protected:
	void AllocChunk();
	void FreeChunk();

	struct ChunkInfo {
		void	*mpChunk;
		uint32	mChunkSize;
	};

	uint32	mChunkHead;
	uint32	mChunkTail;
	uint32	mChunkSize;
	std::list<ChunkInfo> mActiveChunks;
	std::list<ChunkInfo> mFreeChunks;
};

VDChunkedBuffer::VDChunkedBuffer()
	: mChunkHead(0)
	, mChunkTail(0)
	, mChunkSize(65536)
{
}

VDChunkedBuffer::~VDChunkedBuffer() {
	mFreeChunks.splice(mFreeChunks.end(), mActiveChunks);

	while(!mFreeChunks.empty()) {
		ChunkInfo& ci = mFreeChunks.back();

		VDFile::FreeUnbuffer(ci.mpChunk);
		mFreeChunks.pop_back();
	}
}

void VDChunkedBuffer::Write(const void *data, uint32 size) {
	if (!size)
		return;

	while(size) {
		if (mActiveChunks.empty())
			AllocChunk();

		ChunkInfo& ci = mActiveChunks.back();
		uint32 tc = ci.mChunkSize - mChunkHead;
		if (tc > size)
			tc = size;

		if (!tc) {
			AllocChunk();
			mChunkHead = 0;
			continue;
		}

		memcpy((char *)ci.mpChunk + mChunkHead, data, tc);
		data = (const char *)data + tc;
		size -= tc;
		mChunkHead += tc;
	}
}

const void *VDChunkedBuffer::LockRead(uint32 size, uint32& actual) {
	if (mActiveChunks.empty()) {
		actual = 0;
		return NULL;
	}

	ChunkInfo& ci = mActiveChunks.front();
	uint32 avail = ci.mChunkSize - mChunkTail;
	if (size > avail)
		size = avail;

	actual = size;
	return (const char *)ci.mpChunk + mChunkTail;
}

void VDChunkedBuffer::UnlockRead(uint32 size) {
	if (!size)
		return;

	ChunkInfo& ci = mActiveChunks.front();

	mChunkTail += size;
	if (mChunkTail >= ci.mChunkSize) {
		mChunkTail = 0;
		FreeChunk();
	}
}

void VDChunkedBuffer::AllocChunk() {
	if (mFreeChunks.empty()) {
		mFreeChunks.push_back(ChunkInfo());

		ChunkInfo& ci = mFreeChunks.back();
		ci.mpChunk		= VDFile::AllocUnbuffer(mChunkSize);
		if (!ci.mpChunk)
			throw MyMemoryError();
		ci.mChunkSize	= mChunkSize;
	}

	mActiveChunks.splice(mActiveChunks.end(), mFreeChunks, mFreeChunks.begin());
}

void VDChunkedBuffer::FreeChunk() {
	mFreeChunks.splice(mFreeChunks.begin(), mActiveChunks, mActiveChunks.begin());
}

///////////////////////////////////////////////////////////////////////////

class VDAVIOutputSegmentedStream;
class VDAVIOutputSegmentedAudioStream;
class VDAVIOutputSegmentedVideoStream;

class VDAVIOutputSegmented : public IVDMediaOutput, public IVDMediaOutputAutoInterleave {
public:
	VDAVIOutputSegmented(IVDDubberOutputSystem *pOutputSystem, double interval, double preload, sint64 max_bytes, sint64 max_frames);
	~VDAVIOutputSegmented();

	void *AsInterface(uint32 id);

	void Update();
	void Finish(int stream);

	IVDMediaOutputStream *createAudioStream();
	IVDMediaOutputStream *createVideoStream();
	IVDMediaOutputStream *getAudioOutput();
	IVDMediaOutputStream *getVideoOutput();

	bool init(const wchar_t *szFile);
	void finalize();

public:
	void GetNextPreferredStreamWrite(int& stream, sint32& count);

protected:
	void ReinitInterleaver();

	IVDDubberOutputSystem *mpOutputSystem;
	IVDMediaOutput *mpOutput;
	VDAVIOutputSegmentedAudioStream *mpFirstAudioStream;
	VDAVIOutputSegmentedVideoStream *mpFirstVideoStream;

	double	mAudioInterval;
	double	mAudioPreload;
	sint64	mAccumulatedBytes;
	sint64	mAccumulatedFrames;
	sint64	mSegmentSizeLimit;
	sint64	mSegmentFrameLimit;
	bool	mbSegmentEnding;

	VDStreamInterleaver::Action	mNextAction;
	int							mNextStream;
	sint32						mNextSamples;
	VDStreamInterleaver mStreamInterleaver;
};

IVDMediaOutput *VDCreateMediaOutputSegmented(IVDDubberOutputSystem *pOutputSystem, double interval, double preload, sint64 max_bytes, sint64 max_frames) {
	return new VDAVIOutputSegmented(pOutputSystem, interval, preload, max_bytes, max_frames);
}


///////////////////////////////////////////////////////////////////////////

class VDAVIOutputSegmentedStream : public AVIOutputStream {
public:
	enum FlushResult {
		kFlushPending,
		kFlushOK,
		kFlushEnded
	};

	virtual bool IsEnded() = 0;
	virtual void CloseSegmentStream() = 0;
	virtual void OpenSegmentStream(IVDMediaOutputStream *pOutput) = 0;
	virtual bool GetNextPendingRun(uint32& samples, uint32& bytes, VDTime& endTime) = 0;
	virtual bool GetPendingInfo(VDTime endTime, uint32& samples, uint32& bytes) = 0;
	virtual void ScheduleSamples(uint32 samples) = 0;
	virtual FlushResult Flush(uint32 samples, bool force) = 0;
};

///////////////////////////////////////////////////////////////////////////

class VDAVIOutputSegmentedVideoStream : public VDAVIOutputSegmentedStream {
public:
	VDAVIOutputSegmentedVideoStream(VDAVIOutputSegmented *pParent);

	bool IsEnded() { return mbEnded; }
	void CloseSegmentStream();
	void OpenSegmentStream(IVDMediaOutputStream *pOutput);
	bool GetNextPendingRun(uint32& samples, uint32& bytes, VDTime& endTime);
	bool GetPendingInfo(VDTime endTime, uint32& samples, uint32& bytes);
	VDTime GetPendingLevel();
	void ScheduleSamples(uint32 samples);
	FlushResult Flush(uint32 samples, bool force);

	void write(uint32 flags, const void *pBuffer, uint32 cbBuffer, uint32 samples);
	void partialWriteBegin(uint32 flags, uint32 bytes, uint32 samples);
	void partialWrite(const void *pBuffer, uint32 cbBuffer);
	void partialWriteEnd();
	void finish();

protected:
	VDAVIOutputSegmented *const mpParent;
	IVDMediaOutputStream *mpOutputStream;

	struct Block {
		void *data;
		uint32 size;
		uint32 capacity;
		uint32 flags;
	};

	struct Run {
		std::list<Block> mBlocks;
		uint32 mSize;
		bool mbClosed;
		VDTime mEndTime;

		Run() : mSize(0), mbClosed(false) {}
	};

	bool mbEnded;
	VDPosition mSamplesWritten;
	uint32 mExtraSamples;
	uint32 mScheduledSamples;
	uint32 mBufferedSamples;
	std::list<Run> mClearedRuns;
	std::list<Run> mPendingRuns;
};

VDAVIOutputSegmentedVideoStream::VDAVIOutputSegmentedVideoStream(VDAVIOutputSegmented *pParent)
	: mpParent(pParent)
	, mbEnded(false)
	, mSamplesWritten(0)
	, mExtraSamples(0)
	, mScheduledSamples(0)
	, mBufferedSamples(0)
{
}

void VDAVIOutputSegmentedVideoStream::CloseSegmentStream() {
	mpOutputStream = NULL;
}

void VDAVIOutputSegmentedVideoStream::OpenSegmentStream(IVDMediaOutputStream *pOutput) {
	mpOutputStream = pOutput;
}

bool VDAVIOutputSegmentedVideoStream::GetNextPendingRun(uint32& samples, uint32& bytes, VDTime& endTime) {
	if (mPendingRuns.empty())
		return false;

	Run& run = mPendingRuns.front();
	if (!run.mbClosed)
		return false;

	samples = (uint32)run.mBlocks.size();
	bytes = run.mSize;
	endTime = run.mEndTime;
	return true;
}

bool VDAVIOutputSegmentedVideoStream::GetPendingInfo(VDTime endTime, uint32& samples, uint32& bytes) {
	std::list<Run>::const_iterator it(mPendingRuns.begin()), itEnd(mPendingRuns.end());
	bytes = 0;
	samples = 0;

	for(; it!=itEnd; ++it) {
		const Run& run = *it;
		if (!run.mbClosed)
			return false;

		if (run.mEndTime > endTime && endTime >= 0)
			break;

		samples += (uint32)run.mBlocks.size();
		bytes += run.mSize;
	}

	return true;
}

VDTime VDAVIOutputSegmentedVideoStream::GetPendingLevel() {
	return VDRoundToInt64(mBufferedSamples * (1000000.0 * (double)streamInfo.dwScale / (double)streamInfo.dwRate));
}

void VDAVIOutputSegmentedVideoStream::ScheduleSamples(uint32 samples) {
	std::list<Run>::iterator it(mPendingRuns.begin()), itEnd(mPendingRuns.end());
	for(; it!=itEnd; ++it) {
		const Run& run = *it;
		if (!run.mbClosed)
			break;

		uint32 n = (uint32)run.mBlocks.size();

		if (n > samples)
			break;

		samples -= n;
		mScheduledSamples += n;
	}

	if (it != mPendingRuns.begin())
		mClearedRuns.splice(mClearedRuns.end(), mPendingRuns, mPendingRuns.begin(), it);
}

VDAVIOutputSegmentedStream::FlushResult VDAVIOutputSegmentedVideoStream::Flush(uint32 samples, bool force) {
	if (!samples)
		return kFlushOK;

	bool gotall = true;

	if (samples > mScheduledSamples) {
		if (!force || !mScheduledSamples) {
			if (mbEnded && mPendingRuns.empty())
				return kFlushEnded;
			else
				return kFlushPending;
		}

		gotall = false;
		samples = mScheduledSamples;
	}

	samples += mExtraSamples;

	while(samples > 0) {
		Run& run = mClearedRuns.front();

		while(!run.mBlocks.empty() && samples) {
			Block& block = run.mBlocks.front();
			mpOutputStream->write(block.flags, block.data, block.size, 1);
			delete[] (char *)block.data;
			run.mBlocks.pop_front();
			--samples;
			--mScheduledSamples;
			--mBufferedSamples;
		}

		if (run.mBlocks.empty())
			mClearedRuns.pop_front();
	}

	mExtraSamples = samples;
	return kFlushOK;
}

void VDAVIOutputSegmentedVideoStream::write(uint32 flags, const void *pBuffer, uint32 cbBuffer, uint32 samples) {
	if (mPendingRuns.empty())
		mPendingRuns.push_back(Run());
	else if (flags & AVIIF_KEYFRAME) {
		mPendingRuns.back().mbClosed = true;
		mPendingRuns.push_back(Run());
	}

	Run& run = mPendingRuns.back();
	run.mBlocks.push_back(Block());
	run.mSize += (cbBuffer + 1) & ~1;			// evenify for AVI
	Block& block = run.mBlocks.back();

	block.data = new char[cbBuffer];
	block.size = cbBuffer;
	block.capacity = cbBuffer;
	block.flags = flags;

	memcpy(block.data, pBuffer, cbBuffer);
	++mSamplesWritten;
	++mBufferedSamples;

	run.mEndTime = VDRoundToInt64(mSamplesWritten * (1000000.0 * (double)streamInfo.dwScale / (double)streamInfo.dwRate));

	mpParent->Update();
}

void VDAVIOutputSegmentedVideoStream::partialWriteBegin(uint32 flags, uint32 bytes, uint32 samples) {
	throw MyError("Partial writes are not allowed for video streams.");
}

void VDAVIOutputSegmentedVideoStream::partialWrite(const void *pBuffer, uint32 cbBuffer) {
}

void VDAVIOutputSegmentedVideoStream::partialWriteEnd() {
}

void VDAVIOutputSegmentedVideoStream::finish() {
	mbEnded = true;
	if (!mPendingRuns.empty())
		mPendingRuns.back().mbClosed = true;
}

///////////////////////////////////////////////////////////////////////////

class VDAVIOutputSegmentedAudioStream : public VDAVIOutputSegmentedStream {
public:
	VDAVIOutputSegmentedAudioStream(VDAVIOutputSegmented *pParent);

	bool IsEnded() { return mbEnded; }
	void CloseSegmentStream();
	void OpenSegmentStream(IVDMediaOutputStream *pOutput);
	bool GetNextPendingRun(uint32& samples, uint32& bytes, VDTime& endTime);
	bool GetPendingInfo(VDTime endTime, uint32& samples, uint32& bytes);
	VDTime GetPendingLevel();
	void ScheduleSamples(uint32 samples);
	FlushResult Flush(uint32 samples, bool force);

	void write(uint32 flags, const void *pBuffer, uint32 cbBuffer, uint32 samples);
	void partialWriteBegin(uint32 flags, uint32 bytes, uint32 samples);
	void partialWrite(const void *pBuffer, uint32 cbBuffer);
	void partialWriteEnd();
	void finish();

protected:
	VDAVIOutputSegmented *const mpParent;
	IVDMediaOutputStream *mpOutputStream;

	bool	mbEnded;
	uint32	mBufferedSamples;
	uint32	mScheduledSamples;
	uint32	mExtraSamples;
	VDPosition	mTotalBufferedSamples;
	VDPosition	mTotalScheduledSamples;
	VDChunkedBuffer	mBuffer;
};

VDAVIOutputSegmentedAudioStream::VDAVIOutputSegmentedAudioStream(VDAVIOutputSegmented *pParent)
	: mpParent(pParent)
	, mbEnded(false)
	, mBufferedSamples(0)
	, mScheduledSamples(0)
	, mExtraSamples(0)
	, mTotalBufferedSamples(0)
	, mTotalScheduledSamples(0)
{
}

void VDAVIOutputSegmentedAudioStream::CloseSegmentStream() {
	mpOutputStream = NULL;
}

void VDAVIOutputSegmentedAudioStream::OpenSegmentStream(IVDMediaOutputStream *pOutput) {
	mpOutputStream = pOutput;
}

bool VDAVIOutputSegmentedAudioStream::GetNextPendingRun(uint32& samples, uint32& bytes, VDTime& endTime) {
	if (!mBufferedSamples)
		return false;

	samples = mBufferedSamples;
	bytes = samples * ((const WAVEFORMATEX *)getFormat())->nBlockAlign;
	endTime = VDRoundToInt64(mTotalBufferedSamples * (1000000.0 * (double)streamInfo.dwScale / (double)streamInfo.dwRate));
	return true;
}

bool VDAVIOutputSegmentedAudioStream::GetPendingInfo(VDTime endTime, uint32& samples, uint32& bytes) {
	bool ok = true;

	if (endTime < 0)
		samples = mBufferedSamples;
	else {
		samples = (uint32)(VDRoundToInt64(endTime * 1.0 / 1000000.0 * (double)streamInfo.dwRate / (double)streamInfo.dwScale) - mTotalScheduledSamples);

		if (samples > mBufferedSamples) {
			samples = mBufferedSamples;
			if (!mbEnded)
				ok = false;
		}
	}

	bytes = samples * ((const WAVEFORMATEX *)getFormat())->nBlockAlign;

	bytes = (bytes + 1) & ~1;		// evenify for AVI
	return ok;
}

VDTime VDAVIOutputSegmentedAudioStream::GetPendingLevel() {
	return VDRoundToInt64(mTotalBufferedSamples * (1000000.0 * (double)streamInfo.dwScale / (double)streamInfo.dwRate));
}

void VDAVIOutputSegmentedAudioStream::ScheduleSamples(uint32 samples) {
	mScheduledSamples += samples;
	mTotalScheduledSamples += samples;
}

VDAVIOutputSegmentedStream::FlushResult VDAVIOutputSegmentedAudioStream::Flush(uint32 samples, bool force) {
	if (!samples)
		return kFlushOK;

	bool gotall = true;
	if (samples > mScheduledSamples) {
		if (!force || !mScheduledSamples) {
			if (mbEnded && mBufferedSamples <= mScheduledSamples)
				return kFlushEnded;
			else
				return kFlushPending;
		}

		gotall = false;
		samples = mScheduledSamples;
	}
	mScheduledSamples -= samples;

	samples += mExtraSamples;
	mExtraSamples = 0;

	if (samples > mBufferedSamples) {
		samples = mBufferedSamples;
		mExtraSamples = samples - mBufferedSamples;
	}

	mBufferedSamples -= samples;

	uint32 bytes = samples * ((const WAVEFORMATEX *)getFormat())->nBlockAlign;

	mpOutputStream->partialWriteBegin(0, bytes, samples);

	while(bytes > 0) {
		uint32 avail = 0;
		const void *src = mBuffer.LockRead(bytes, avail);
		VDASSERT(avail > 0 && avail <= bytes);
		if (!avail)
			break;
		mpOutputStream->partialWrite(src, avail);
		mBuffer.UnlockRead(avail);
		bytes -= avail;
	}

	mpOutputStream->partialWriteEnd();
	return kFlushOK;
}

void VDAVIOutputSegmentedAudioStream::write(uint32 flags, const void *pBuffer, uint32 cbBuffer, uint32 samples) {
	if (!samples)
		return;

	mBufferedSamples += samples;
	mTotalBufferedSamples += samples;
	mBuffer.Write(pBuffer, cbBuffer);

	mpParent->Update();
}

void VDAVIOutputSegmentedAudioStream::partialWriteBegin(uint32 flags, uint32 bytes, uint32 samples) {
	mBufferedSamples += samples;
}

void VDAVIOutputSegmentedAudioStream::partialWrite(const void *pBuffer, uint32 cbBuffer) {
	mBuffer.Write(pBuffer, cbBuffer);
}

void VDAVIOutputSegmentedAudioStream::partialWriteEnd() {
}

void VDAVIOutputSegmentedAudioStream::finish() {
	mbEnded = true;
}

///////////////////////////////////////////////////////////////////////////

VDAVIOutputSegmented::VDAVIOutputSegmented(IVDDubberOutputSystem *pOutputSystem, double interval, double preload, sint64 max_bytes, sint64 max_frames)
	: mNextAction(VDStreamInterleaver::kActionNone)
	, mpOutputSystem(pOutputSystem)
	, mpOutput(NULL)
	, mpFirstAudioStream(NULL)
	, mpFirstVideoStream(NULL)
	, mAccumulatedBytes(0)
	, mAccumulatedFrames(0)
	, mSegmentSizeLimit(max_bytes)
	, mSegmentFrameLimit(max_frames)
	, mbSegmentEnding(false)
	, mAudioInterval(interval)
	, mAudioPreload(preload)
{

	if (mSegmentSizeLimit <= 0)
		mSegmentSizeLimit = 0x0000FFFFFFFFFFFF;

	if (mSegmentFrameLimit <= 0)
		mSegmentFrameLimit = 0x0000FFFFFFFFFFFF;

	mSegmentSizeLimit -= 16384;		// a little slop for indices
}

VDAVIOutputSegmented::~VDAVIOutputSegmented() {
	delete mpFirstAudioStream;
	delete mpFirstVideoStream;
}

//////////////////////////////////

void *VDAVIOutputSegmented::AsInterface(uint32 id) {
	if (id == IVDMediaOutput::kTypeID)
		return static_cast<IVDMediaOutput *>(this);

	return NULL;
}

void VDAVIOutputSegmented::Update() {
	if (!mbSegmentEnding) {
		uint32 videoSamples, videoBytes;
		VDTime endTime;
		if (mpFirstVideoStream->GetNextPendingRun(videoSamples, videoBytes, endTime)) {
			uint32 audioSamples, audioBytes;

			if (!mpFirstAudioStream) {
				sint64 newSize = mAccumulatedBytes + videoBytes;
				if (mAccumulatedBytes > 0 && (newSize + mAccumulatedFrames * 24 > mSegmentSizeLimit || mAccumulatedFrames+videoSamples >= mSegmentFrameLimit)) {
					mbSegmentEnding = true;
				} else {
					mAccumulatedBytes = newSize;
					mAccumulatedFrames += videoSamples;
					mpFirstVideoStream->ScheduleSamples(videoSamples);
				}
			} else if (mpFirstAudioStream->GetPendingInfo(endTime, audioSamples, audioBytes)) {
				sint64 newSize = mAccumulatedBytes + audioBytes + videoBytes;

				if (mAccumulatedBytes > 0 && (newSize + VDFloorToInt64(mAccumulatedFrames * (1.0 + 1.0 / mAudioInterval)) * 24 > mSegmentSizeLimit || mAccumulatedFrames+videoSamples >= mSegmentFrameLimit)) {
					mbSegmentEnding = true;
				} else {
					mAccumulatedBytes = newSize;
					mAccumulatedFrames += videoSamples;
					mpFirstVideoStream->ScheduleSamples(videoSamples);
					mpFirstAudioStream->ScheduleSamples(audioSamples);
				}
			}
		} else if (mpFirstVideoStream->IsEnded()) {
			uint32 audioSamples, audioBytes;

			if (mpFirstAudioStream->GetPendingInfo(-1, audioSamples, audioBytes)) {
				sint64 newSize = mAccumulatedBytes + audioBytes + 24;

				if (newSize + VDFloorToInt64(mAccumulatedFrames * (1.0 + 1.0 / mAudioInterval)) * 24 > mSegmentSizeLimit && mAccumulatedBytes > 0) {
					mbSegmentEnding = true;
				} else {
					mAccumulatedBytes = newSize;
					mpFirstAudioStream->ScheduleSamples(audioSamples);
				}
			}
		}
	}

	for(;;) {
		if (mNextAction == VDStreamInterleaver::kActionFinish) {
			if (mbSegmentEnding) {
				ReinitInterleaver();
				mbSegmentEnding = false;

				if (mpFirstAudioStream)
					mpFirstAudioStream->CloseSegmentStream();

				mpFirstVideoStream->CloseSegmentStream();

				mpOutputSystem->CloseSegment(mpOutput, false);
				mpOutput = mpOutputSystem->CreateSegment();

				mpFirstVideoStream->OpenSegmentStream(mpOutput->getVideoOutput());
				if (mpFirstAudioStream)
					mpFirstAudioStream->OpenSegmentStream(mpOutput->getAudioOutput());
				mNextAction = VDStreamInterleaver::kActionNone;
				mAccumulatedBytes = 0;
				mAccumulatedFrames = 0;
			} else
				return;
		}

		if (mNextAction == VDStreamInterleaver::kActionWrite) {
			VDAVIOutputSegmentedStream *stream = mNextStream ? static_cast<VDAVIOutputSegmentedStream *>(mpFirstAudioStream) : static_cast<VDAVIOutputSegmentedStream *>(mpFirstVideoStream);

			switch(stream->Flush(mNextSamples, mbSegmentEnding)) {
				case VDAVIOutputSegmentedStream::kFlushPending:
					if (!mbSegmentEnding)
						return;
					// fall through
				case VDAVIOutputSegmentedStream::kFlushEnded:
					mStreamInterleaver.EndStream(mNextStream);
					break;

				case VDAVIOutputSegmentedStream::kFlushOK:
					break;
			}
		}

		mNextAction = mStreamInterleaver.GetNextAction(mNextStream, mNextSamples);
	}
}

void VDAVIOutputSegmented::Finish(int stream) {
	mStreamInterleaver.EndStream(stream);
}

void VDAVIOutputSegmented::ReinitInterleaver() {
	if (mpFirstAudioStream) {
		mStreamInterleaver.Init(2);
		mStreamInterleaver.EnableInterleaving(true);
		mStreamInterleaver.InitStream(0, 0, 0, 1, 1, 1);
		const WAVEFORMATEX& wfex = *(const WAVEFORMATEX *)mpFirstAudioStream->getFormat();
		const AVIStreamHeader_fixed& hdr = mpFirstVideoStream->getStreamInfo();
		double samplesPerSecond = (double)wfex.nAvgBytesPerSec / (double)wfex.nBlockAlign;
		double invFrameRate = (double)hdr.dwScale / (double)hdr.dwRate;
		sint32 preloadSamples = VDRoundToInt32(mAudioPreload * samplesPerSecond);
		double samplesPerFrame = invFrameRate * samplesPerSecond;
		mStreamInterleaver.InitStream(1, wfex.nBlockAlign, preloadSamples, samplesPerFrame, mAudioInterval, 0x7FFFFFFF / wfex.nBlockAlign);
	} else {
		mStreamInterleaver.Init(1);
		mStreamInterleaver.InitStream(0, 0, 0, 1, 1, 1);
	}
}

//////////////////////////////////

IVDMediaOutputStream *VDAVIOutputSegmented::createVideoStream() {
	VDAVIOutputSegmentedVideoStream *pStream = new_nothrow VDAVIOutputSegmentedVideoStream(this);
	if (!pStream)
		throw MyMemoryError();

	if (!mpFirstVideoStream)
		mpFirstVideoStream = pStream;

	return pStream;
}

IVDMediaOutputStream *VDAVIOutputSegmented::createAudioStream() {
	VDAVIOutputSegmentedAudioStream *pStream = new_nothrow VDAVIOutputSegmentedAudioStream(this);
	if (!pStream)
		throw MyMemoryError();

	if (!mpFirstAudioStream)
		mpFirstAudioStream = pStream;

	return pStream;
}

IVDMediaOutputStream *VDAVIOutputSegmented::getAudioOutput() {
	return mpFirstAudioStream;
}

IVDMediaOutputStream *VDAVIOutputSegmented::getVideoOutput() {
	return mpFirstVideoStream;
}

bool VDAVIOutputSegmented::init(const wchar_t *szFile) {
	mpOutput = mpOutputSystem->CreateSegment();
	mpFirstVideoStream->OpenSegmentStream(mpOutput->getVideoOutput());
	if (mpFirstAudioStream)
		mpFirstAudioStream->OpenSegmentStream(mpOutput->getAudioOutput());
	ReinitInterleaver();
	return true;
}

void VDAVIOutputSegmented::finalize() {
	if (mpFirstAudioStream)
		mpFirstAudioStream->finish();
	if (mpFirstVideoStream)
		mpFirstVideoStream->finish();

	while(mNextAction != VDStreamInterleaver::kActionFinish)
		Update();

	if (mpOutput) {
		IVDMediaOutput *pOutput = mpOutput;
		mpOutput = NULL;
		mpOutputSystem->CloseSegment(pOutput, true);
	}
}

void VDAVIOutputSegmented::GetNextPreferredStreamWrite(int& stream, sint32& count) {
	stream = 0;

	if (mpFirstVideoStream->IsEnded() || (mpFirstAudioStream && !mpFirstAudioStream->IsEnded())) {
		if (mpFirstVideoStream->GetPendingLevel() < mpFirstAudioStream->GetPendingLevel())
			stream = 1;
	}

	if (stream) {
		const AVIStreamHeader_fixed& vhdr = mpFirstVideoStream->getStreamInfo();
		const AVIStreamHeader_fixed& ahdr = mpFirstAudioStream->getStreamInfo();

		count = VDRoundToInt32(
			((double)ahdr.dwRate / (double)ahdr.dwScale) *
			((double)vhdr.dwScale / (double)vhdr.dwRate)
		);
	} else {
		count = 1;
	}
}

