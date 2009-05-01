//	VirtualDub - Video processing and capture application
//	Copyright (C) 1998-2001 Avery Lee
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
#include <windows.h>

#include <vd2/system/file.h>
#include <vd2/system/error.h>
#include <vd2/system/filesys.h>
#include <vd2/system/vdstl.h>

#include "PositionControl.h"

#include "InputFile.h"
#include "InputFileImages.h"
#include "AudioSource.h"
#include "VideoSource.h"
#include "AVIOutput.h"
#include "AVIOutputWAV.h"
#include "AVIOutputImages.h"
#include "AVIOutputStriped.h"
#include "Dub.h"
#include "DubOutput.h"
#include "AudioFilterSystem.h"
#include "FrameSubset.h"
#include "ProgressDialog.h"
#include "oshelper.h"

#include "mpeg.h"
#include "gui.h"
#include "prefs.h"
#include "command.h"
#include "project.h"
#include "resource.h"

///////////////////////////////////////////////////////////////////////////

extern HWND					g_hWnd;
extern DubOptions			g_dubOpts;

extern wchar_t g_szInputWAVFile[MAX_PATH];

extern DubSource::ErrorMode	g_videoErrorMode;
extern DubSource::ErrorMode	g_audioErrorMode;

vdrefptr<InputFile>		inputAVI;
InputFileOptions	*g_pInputOpts			= NULL;

vdrefptr<VideoSource>	inputVideoAVI;
extern vdrefptr<AudioSource>	inputAudio;

int					 audioInputMode = AUDIOIN_AVI;

IDubber				*g_dubber				= NULL;

COMPVARS			g_Vcompression;
WAVEFORMATEX		*g_ACompressionFormat		= NULL;
DWORD				g_ACompressionFormatSize	= 0;
VDStringA			g_ACompressionFormatHint;

VDAudioFilterGraph	g_audioFilterGraph;

extern VDProject *g_project;


bool				g_drawDecompressedFrame	= FALSE;
bool				g_showStatusWindow		= TRUE;

extern uint32& VDPreferencesGetRenderOutputBufferSize();

///////////////////////////////////////////////////////////////////////////

void AppendAVI(const wchar_t *pszFile) {
	if (inputAVI) {
		VDPosition lTail = inputAVI->videoSrc->getEnd();

		if (inputAVI->Append(pszFile)) {
			g_project->BeginTimelineUpdate();
			FrameSubset& s = g_project->GetTimeline().GetSubset();

			s.insert(s.end(), FrameSubsetNode(lTail, inputAVI->videoSrc->getEnd() - lTail, false, 0));
			g_project->EndTimelineUpdate();
		}
	}
}

void AppendAVIAutoscan(const wchar_t *pszFile) {
	wchar_t buf[MAX_PATH];
	wchar_t *s = buf, *t;
	int count = 0;

	if (!inputAVI)
		return;

	VDPosition originalCount = inputAVI->videoSrc->getEnd();

	wcscpy(buf, pszFile);

	t = VDFileSplitExt(VDFileSplitPath(s));

	if (t>buf)
		--t;

	try {
		for(;;) {
			if (!VDDoesPathExist(buf))
				break;
			
			if (!inputAVI->Append(buf))
				break;

			++count;

			s = t;

			for(;;) {
				if (s<buf || !isdigit(*s)) {
					memmove(s+2, s+1, sizeof(wchar_t) * wcslen(s));
					s[1] = L'1';
					++t;
				} else {
					if (*s == L'9') {
						*s-- = L'0';
						continue;
					}
					++*s;
				}
				break;
			}
		}
	} catch(const MyError& e) {
		// if the first segment failed, turn the warning into an error
		if (!count)
			throw;

		// log append errors, but otherwise eat them
		VDLog(kVDLogWarning, VDTextAToW(e.gets()));
	}

	guiSetStatus("Appended %d segments (stopped at \"%s\")", 255, count, VDTextWToA(buf).c_str());

	if (count) {
		FrameSubset& s = g_project->GetTimeline().GetSubset();
		g_project->BeginTimelineUpdate();
		s.insert(s.end(), FrameSubsetNode(originalCount, inputAVI->videoSrc->getEnd() - originalCount, false, 0));
		g_project->EndTimelineUpdate();
	}
}

void SaveWAV(const wchar_t *szFilename, bool fProp, DubOptions *quick_opts) {
	if (!inputVideoAVI)
		throw MyError("No input file to process.");

	if (!inputAudio)
		throw MyError("No audio stream to process.");

	VDAVIOutputWAVSystem wavout(szFilename);
	g_project->RunOperation(&wavout, TRUE, quick_opts, 0, fProp);
}

///////////////////////////////////////////////////////////////////////////

void SaveAVI(const wchar_t *szFilename, bool fProp, DubOptions *quick_opts, bool fCompatibility) {
	VDAVIOutputFileSystem fileout;

	fileout.Set1GBLimit(g_prefs.fAVIRestrict1Gb != 0);
	fileout.SetCaching(false);
	fileout.SetIndexing(!fCompatibility);
	fileout.SetFilename(szFilename);
	fileout.SetBuffer(VDPreferencesGetRenderOutputBufferSize());
	fileout.SetTextInfo(g_project->GetTextInfo());

	g_project->RunOperation(&fileout, FALSE, quick_opts, g_prefs.main.iDubPriority, fProp);
}

void SaveStripedAVI(const wchar_t *szFile) {
	if (!inputVideoAVI)
		throw MyError("No input video stream to process.");

	VDAVIOutputStripedSystem outstriped(szFile);

	outstriped.Set1GBLimit(g_prefs.fAVIRestrict1Gb != 0);

	g_project->RunOperation(&outstriped, FALSE, NULL, g_prefs.main.iDubPriority, false, 0, 0);
}

void SaveStripeMaster(const wchar_t *szFile) {
	if (!inputVideoAVI)
		throw MyError("No input video stream to process.");

	VDAVIOutputStripedSystem outstriped(szFile);

	outstriped.Set1GBLimit(g_prefs.fAVIRestrict1Gb != 0);

	g_project->RunOperation(&outstriped, 2, NULL, g_prefs.main.iDubPriority, false, 0, 0);
}

void SaveSegmentedAVI(const wchar_t *szFilename, bool fProp, DubOptions *quick_opts, long lSpillThreshold, long lSpillFrameThreshold) {
	if (!inputVideoAVI)
		throw MyError("No input file to process.");

	VDAVIOutputFileSystem outfile;

	outfile.SetIndexing(false);
	outfile.SetCaching(false);
	outfile.SetBuffer(VDPreferencesGetRenderOutputBufferSize());

	const VDStringW filename(szFilename);
	outfile.SetFilenamePattern(VDFileSplitExtLeft(filename).c_str(), VDFileSplitExtRight(filename).c_str(), 2);

	g_project->RunOperation(&outfile, FALSE, quick_opts, g_prefs.main.iDubPriority, fProp, lSpillThreshold, lSpillFrameThreshold);
}

void SaveImageSequence(const wchar_t *szPrefix, const wchar_t *szSuffix, int minDigits, bool fProp, DubOptions *quick_opts, int targetFormat, int quality) {
	VDAVIOutputImagesSystem outimages;

	outimages.SetFilenamePattern(szPrefix, szSuffix, minDigits);
	outimages.SetFormat(targetFormat, quality);
		
	g_project->RunOperation(&outimages, FALSE, quick_opts, g_prefs.main.iDubPriority, fProp, 0, 0);
}

///////////////////////////////////////////////////////////////////////////


void SetSelectionStart(long ms) {
	if (!inputVideoAVI)
		return;

	g_project->SetSelectionStart(inputVideoAVI->msToSamples(ms));
}

void SetSelectionEnd(long ms) {
	if (!inputVideoAVI)
		return;

	g_project->SetSelectionEnd(g_project->GetFrameCount() - inputVideoAVI->msToSamples(ms));
}

void ScanForUnreadableFrames(FrameSubset *pSubset, VideoSource *pVideoSource) {
	const VDPosition lFirst = pVideoSource->getStart();
	const VDPosition lLast = pVideoSource->getEnd();
	VDPosition lFrame = lFirst;
	vdblock<char>	buffer;

	IVDStreamSource::ErrorMode oldErrorMode(pVideoSource->getDecodeErrorMode());
	pVideoSource->setDecodeErrorMode(IVDStreamSource::kErrorModeReportAll);

	try {
		ProgressDialog pd(g_hWnd, "Frame scan", "Scanning for unreadable frames", lLast-lFrame, true);
		bool bLastValid = true;
		VDPosition lRangeFirst;
		long lDeadFrames = 0;
		long lMaskedFrames = 0;

		pd.setValueFormat("Frame %d of %d");

		pVideoSource->streamBegin(false, true);

		const uint32 padSize = pVideoSource->streamGetDecodePadding();

		while(lFrame <= lLast) {
			uint32 lActualBytes, lActualSamples;
			int err;
			bool bValid;

			pd.advance(lFrame - lFirst);
			pd.check();

			do {
				bValid = false;

				if (!bLastValid && !pVideoSource->isKey(lFrame))
					break;

				if (lFrame < lLast) {
					err = pVideoSource->read(lFrame, 1, NULL, 0, &lActualBytes, &lActualSamples);

					if (err)
						break;

					if (buffer.empty() || buffer.size() < lActualBytes + padSize)
						buffer.resize(((lActualBytes + padSize + 65535) & ~65535) + !lActualBytes);

					err = pVideoSource->read(lFrame, 1, buffer.data(), buffer.size(), &lActualBytes, &lActualSamples);

					if (err)
						break;

					try {
						pVideoSource->streamGetFrame(buffer.data(), lActualBytes, FALSE, lFrame, lFrame);
					} catch(...) {
						++lDeadFrames;
						break;
					}
				}

				bValid = true;
			} while(false);

			if (!bValid)
				++lMaskedFrames;

			if (bValid ^ bLastValid) {
				if (!bValid)
					lRangeFirst = lFrame;
				else
					pSubset->setRange(lRangeFirst, lFrame - lRangeFirst, true, 0);

				bLastValid = bValid;
			}

			++lFrame;
		}

		pVideoSource->streamEnd();

		guiSetStatus("%ld frames masked (%ld frames bad, %ld frames good but undecodable)", 255, lMaskedFrames, lDeadFrames, lMaskedFrames-lDeadFrames);

	} catch(...) {
		pVideoSource->setDecodeErrorMode(oldErrorMode);
		pVideoSource->invalidateFrameBuffer();
		throw;
	}
	pVideoSource->setDecodeErrorMode(oldErrorMode);
	pVideoSource->invalidateFrameBuffer();

	g_project->DisplayFrame();
}
