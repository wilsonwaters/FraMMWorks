//	VirtualDub - Video processing and capture application
//	Copyright (C) 1998-2003 Avery Lee
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
#include <vd2/system/profile.h>
#include <vd2/system/VDRingBuffer.h>
#include "DubIO.h"
#include "Dub.h"
#include "DubUtils.h"
#include "VideoSource.h"
#include "Audio.h"
#include "AVIPipe.h"

using namespace nsVDDub;

VDDubIOThread::VDDubIOThread(
		bool				bPhantom,
		const vdfastvector<IVDVideoSource *>& videoSources,
		VDRenderFrameIterator& videoFrameIterator,
		AudioStream			*pAudio,
		AVIPipe				*const pVideoPipe,
		VDAudioPipeline		*const pAudioPipe,
		volatile bool&		bAbort,
		DubAudioStreamInfo&	_aInfo,
		DubVideoStreamInfo& _vInfo,
		VDAtomicInt&		threadCounter
							 )
	: VDThread("Dub-I/O")
	, mbError(false)
	, mbPhantom(bPhantom)
	, mVideoSources(videoSources)
	, mVideoFrameIterator(videoFrameIterator)
	, mpAudio(pAudio)
	, mpVideoPipe(pVideoPipe)
	, mpAudioPipe(pAudioPipe)
	, mbAbort(bAbort)
	, aInfo(_aInfo)
	, vInfo(_vInfo)
	, mThreadCounter(threadCounter)
	, mpCurrentAction("starting up")
{
}

VDDubIOThread::~VDDubIOThread() {
}

void VDDubIOThread::SetThrottle(float f) {
	mLoopThrottle.SetThrottleFactor(f);
}

void VDDubIOThread::ThreadRun() {
	VDRTProfileChannel profchan("I/O");

	bool bAudioActive = mpAudioPipe && (mpAudio != 0);
	bool bVideoActive = mpVideoPipe && !mVideoSources.empty();

	double nVideoRate = bVideoActive ? vInfo.frameRateNoTelecine : 0;
	double nAudioRate = bAudioActive ? mpAudio->GetFormat()->nAvgBytesPerSec : 0;

	try {
		mpCurrentAction = "running main loop";

		while(!mbAbort && (bAudioActive || bVideoActive)) { 
			bool bBlocked = true;

			++mThreadCounter;

			if (!mLoopThrottle.Delay())
				continue;

			bool bCanWriteVideo = bVideoActive && !mpVideoPipe->full();
			bool bCanWriteAudio = bAudioActive && !mpAudioPipe->full();

			if (bCanWriteVideo && bCanWriteAudio) {
				const int nAudioLevel = mpAudioPipe->getLevel();
				int nVideoTotal, nVideoFinalQueued;
				mpVideoPipe->getQueueInfo(nVideoTotal, nVideoFinalQueued);

				if (nAudioLevel * nVideoRate < nVideoFinalQueued * nAudioRate)
					bCanWriteVideo = false;
			}

			if (bCanWriteVideo) {
				bBlocked = false;

				VDDubAutoThreadLocation loc(mpCurrentAction, "reading video data");

				profchan.Begin(0xffe0e0, "Video");

				if (!MainAddVideoFrame() && vInfo.cur_dst >= vInfo.end_dst) {
					bVideoActive = false;
					mpVideoPipe->finalize();
				}

				profchan.End();
				continue;
			}

			if (bCanWriteAudio) {
				bBlocked = false;

				VDDubAutoThreadLocation loc(mpCurrentAction, "reading audio data");

				profchan.Begin(0xe0e0ff, "Audio");

				if (!MainAddAudioFrame() && mpAudio->isEnd()) {
					bAudioActive = false;
					mpAudioPipe->CloseInput();
				}

				profchan.End();
				continue;
			}

			if (bBlocked) {
				if (bAudioActive && mpAudioPipe->isOutputClosed()) {
					bAudioActive = false;
					continue;
				}

				if (bVideoActive && mpVideoPipe->isFinalizeAcked()) {
					bVideoActive = false;
					continue;
				}

				VDDubAutoThreadLocation loc(mpCurrentAction, "stalled due to full pipe to processing thread");

//				profchan.Begin(0xe0e0e0, "Idle");
				if (bAudioActive) {
					if (bVideoActive)
						mpVideoPipe->getReadSignal().wait(&mpAudioPipe->getReadSignal());
					else
						mpAudioPipe->getReadSignal().wait();
				} else if (mpVideoPipe)
					mpVideoPipe->getReadSignal().wait();

//				profchan.End();
			}
		}
	} catch(MyError& e) {
		if (!mbError) {
			mError.TransferFrom(e);
			mbError = true;
		}

		mbAbort = true;

		if (mpAudioPipe)
			mpAudioPipe->CloseInput();
		if (mpVideoPipe)
			mpVideoPipe->finalize();
	}
}

void VDDubIOThread::ReadVideoFrame(int sourceIndex, VDPosition stream_frame, VDPosition target_frame, VDPosition display_frame, VDPosition timeline_frame, bool preload, bool direct, bool sameAsLast) {
	int hr;

	void *buffer;
	int handle;

	IVDVideoSource *vsrc = mVideoSources[sourceIndex];

	VDRenderVideoPipeFrameInfo frameInfo;
	frameInfo.mLength			= 0;
	frameInfo.mRawFrame			= stream_frame;
	frameInfo.mTargetFrame		= target_frame;
	frameInfo.mDisplayFrame		= display_frame;
	frameInfo.mTimelineFrame	= timeline_frame;
	frameInfo.mSrcIndex			= sourceIndex;
	frameInfo.mFlags			= (vsrc->isKey(display_frame) ? 0 : kBufferFlagDelta) + (preload ? kBufferFlagPreload : 0);
	frameInfo.mDroptype			= 0;
	frameInfo.mbFinal			= !preload;

	if (direct)
		frameInfo.mFlags |= kBufferFlagDirectWrite;

	if (sameAsLast)
		frameInfo.mFlags |= kBufferFlagSameAsLast;

	if (mbPhantom) {
		buffer = mpVideoPipe->getWriteBuffer(0, &handle);
		if (!buffer)
			return;	// hmm, aborted...

		frameInfo.mFlags |= kBufferFlagDirectWrite;
		mpVideoPipe->postBuffer(frameInfo);
		return;
	}

//	VDDEBUG("Reading frame %ld (%s)\n", lVStreamPos, preload ? "preload" : "process");

	uint32 size;
	{
		VDDubAutoThreadLocation loc(mpCurrentAction, "reading video data from disk");

		hr = vsrc->asStream()->read(stream_frame, 1, NULL, 0x7FFFFFFF, &size, NULL);
	}
	if (hr) {
		if (hr == AVIERR_FILEREAD)
			throw MyError("Video frame %d could not be read from the source. The file may be corrupt.", stream_frame);
		else
			throw MyAVIError("Dub/IO-Video", hr);
	}

	buffer = mpVideoPipe->getWriteBuffer(size + vsrc->streamGetDecodePadding(), &handle);
	if (!buffer) return;	// hmm, aborted...

	uint32 lActualBytes;
	{
		VDDubAutoThreadLocation loc(mpCurrentAction, "reading video data from disk");
		hr = vsrc->asStream()->read(stream_frame, 1, buffer, size,	&lActualBytes,NULL); 
	}
	if (hr) {
		if (hr == AVIERR_FILEREAD)
			throw MyError("Video frame %d could not be read from the source. The file may be corrupt.", stream_frame);
		else
			throw MyAVIError("Dub/IO-Video", hr);
	}

	display_frame = vsrc->streamToDisplayOrder(stream_frame);

	frameInfo.mLength	= lActualBytes;
	frameInfo.mDroptype	= vsrc->getDropType(display_frame);
	mpVideoPipe->postBuffer(frameInfo);
}

void VDDubIOThread::ReadNullVideoFrame(int sourceIndex, VDPosition displayFrame, VDPosition timelineFrame, bool direct, bool sameAsLast) {
	void *buffer;
	int handle;

	buffer = mpVideoPipe->getWriteBuffer(1, &handle);
	if (!buffer) return;	// hmm, aborted...

	IVDVideoSource *vsrc = mVideoSources[sourceIndex];

	VDRenderVideoPipeFrameInfo frameInfo;
	frameInfo.mLength			= 0;
	frameInfo.mDisplayFrame		= displayFrame;
	frameInfo.mTimelineFrame	= timelineFrame;
	frameInfo.mTargetFrame		= vsrc->displayToStreamOrder(displayFrame);
	frameInfo.mSrcIndex			= sourceIndex;
	frameInfo.mbFinal			= true;

	if (displayFrame >= 0) {
		frameInfo.mRawFrame			= -1;
		frameInfo.mFlags			= (vsrc->isKey(displayFrame) ? 0 : kBufferFlagDelta);
		frameInfo.mDroptype			= vsrc->getDropType(displayFrame);
	} else {
		// can happen in direct mode for pad frames
		frameInfo.mRawFrame			= -1;
		frameInfo.mTargetFrame		= -1;
		frameInfo.mFlags			= kBufferFlagDelta;
		frameInfo.mDroptype			= AVIPipe::kDroppable;
	}

	if (direct)
		frameInfo.mFlags |= kBufferFlagDirectWrite;

	if (sameAsLast)
		frameInfo.mFlags |= kBufferFlagSameAsLast;

	frameInfo.mFlags |= kBufferFlagInternalDecode;

	mpVideoPipe->postBuffer(frameInfo);
}

//////////////////////

bool VDDubIOThread::MainAddVideoFrame() {
	if (vInfo.cur_dst >= vInfo.end_dst)
		return false;
	
	VDRenderFrameStep step(mVideoFrameIterator.Next());

	if (step.mSourceFrame < 0)
		ReadNullVideoFrame(step.mSrcIndex, step.mDisplayFrame, step.mTimelineFrame, step.mbDirect, step.mbSameAsLast);
	else
		ReadVideoFrame(step.mSrcIndex, step.mSourceFrame, step.mTargetSample, step.mDisplayFrame, step.mTimelineFrame, step.mbIsPreroll, step.mbDirect, step.mbSameAsLast);

	if (!step.mbIsPreroll)
		++vInfo.cur_dst;

	return true;
}

bool VDDubIOThread::MainAddAudioFrame() {
	LONG lActualBytes=0;
	LONG lActualSamples=0;

	const int blocksize = mpAudio->GetFormat()->nBlockAlign;
	int samples = mpAudioPipe->getSpace();

	while(samples > 0) {
		int len = samples * blocksize;

		int tc;
		void *dst;
		
		dst = mpAudioPipe->BeginWrite(len, tc);

		if (!tc)
			break;

		if (mbAbort)
			break;

		LONG ltActualBytes, ltActualSamples;
		{
			VDDubAutoThreadLocation loc(mpCurrentAction, "reading/processing audio data");
			ltActualSamples = mpAudio->Read(dst, tc / blocksize, &ltActualBytes);
			VDASSERT(ltActualBytes <= tc);
		}

		if (ltActualSamples <= 0)
			break;

		int residue = ltActualBytes % blocksize;

		if (residue) {
			VDASSERT(false);	// This is bad -- it means the input file has partial samples.

			ltActualBytes += blocksize - residue;
		}

		mpAudioPipe->EndWrite(ltActualBytes);

		aInfo.total_size += ltActualBytes;

		lActualBytes += ltActualBytes;
		lActualSamples += ltActualSamples;

		samples -= ltActualSamples;
	}

	return lActualSamples > 0;
}

