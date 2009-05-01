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
#include <process.h>

#include <windows.h>
#include <vfw.h>
#include <commdlg.h>

#include "InputFile.h"
#include "InputFileAVI.h"
#include "AudioSource.h"
#include "VideoSource.h"
#include <vd2/system/debug.h>
#include <vd2/system/error.h>
#include <vd2/system/filesys.h>
#include <vd2/Dita/resources.h>
#include <vd2/Dita/services.h>
#include <vd2/Riza/audioformat.h>
#include "AVIStripeSystem.h"
#include "AVIReadHandler.h"

#include "gui.h"
#include "oshelper.h"
#include "prefs.h"
#include "misc.h"

#include "resource.h"

extern HINSTANCE g_hInst;
extern const wchar_t fileFiltersAppend[];
extern HWND g_hWnd;

bool VDPreferencesIsPreferInternalDecodersEnabled();

namespace {
	enum { kVDST_InputFileAVI = 4 };

	enum {
		kVDM_OpeningFile,			// AVI: Opening file "%ls"
		kVDM_RekeyNotSpecified,		// AVI: Keyframe flag reconstruction was not specified in open options and the video stream is not a known keyframe-only type.  Seeking in the video stream may be extremely slow.
		kVDM_Type1DVNoSound,		// AVI: Type-1 DV file detected -- VirtualDub cannot extract audio from this type of interleaved stream.
		kVDM_InvalidBlockAlign		// AVI: An invalid nBlockAlign value of zero in the audio stream format was fixed.
	};
}

/////////////////////////////////////////////////////////////////////

class VDInputDriverAVI2 : public vdrefcounted<IVDInputDriver> {
public:
	const wchar_t *GetSignatureName() { return L"Audio/video interleave input driver (internal)"; }

	int GetDefaultPriority() {
		return 0;
	}

	uint32 GetFlags() { return kF_Video | kF_Audio; }

	const wchar_t *GetFilenamePattern() {
		return L"Audio/video interleave (*.avi,*.divx)\0*.avi;*.divx\0";
	}

	bool DetectByFilename(const wchar_t *pszFilename) {
		return false;
	}

	int DetectBySignature(const void *pHeader, sint32 nHeaderSize, const void *pFooter, sint32 nFooterSize, sint64 nFileSize) {
		if (nHeaderSize >= 12) {
			if (!memcmp(pHeader, "RIFF", 4) && !memcmp((char*)pHeader+8, "AVI ", 4))
				return 1;
		}

		return -1;
	}

	InputFile *CreateInputFile(uint32 flags) {
		InputFileAVI *pf = new_nothrow InputFileAVI;

		if (!pf)
			throw MyMemoryError();

		if (flags & kOF_Quiet)
			pf->setAutomated(true);

		if (flags & kOF_AutoSegmentScan)
			pf->EnableSegmentAutoscan();

		return pf;
	}
};

class VDInputDriverAVI1 : public vdrefcounted<IVDInputDriver> {
public:
	const wchar_t *GetSignatureName() { return L"AVIFile/Avisynth input driver (internal)"; }

	int GetDefaultPriority() {
		return -4;
	}

	uint32 GetFlags() { return kF_Video | kF_Audio; }

	const wchar_t *GetFilenamePattern() {
		return L"AVIFile input driver (compat.) (*.avs,*.vdr)\0*.avs;*.vdr\0";
	}

	bool DetectByFilename(const wchar_t *pszFilename) {
		size_t l = wcslen(pszFilename);
		if (l > 4) {
			if (!_wcsicmp(pszFilename + l - 4, L".avs"))
				return true;
			if (!_wcsicmp(pszFilename + l - 4, L".vdr"))
				return true;
		}

		return false;
	}

	int DetectBySignature(const void *pHeader, sint32 nHeaderSize, const void *pFooter, sint32 nFooterSize, sint64 nFileSize) {
		if (nHeaderSize >= 12) {
			if (!memcmp(pHeader, "RIFF", 4) && !memcmp((char*)pHeader+8, "VDRM", 4))
				return 1;
		}

		return -1;
	}

	InputFile *CreateInputFile(uint32 flags) {
		InputFileAVI *pf = new_nothrow InputFileAVI;

		if (!pf)
			throw MyMemoryError();

		pf->ForceCompatibility();

		if (flags & kOF_Quiet)
			pf->setAutomated(true);

		if (flags & kOF_AutoSegmentScan)
			pf->EnableSegmentAutoscan();

		return pf;
	}
};

extern IVDInputDriver *VDCreateInputDriverAVI1() { return new VDInputDriverAVI1; }
extern IVDInputDriver *VDCreateInputDriverAVI2() { return new VDInputDriverAVI2; }

/////////////////////////////////////////////////////////////////////

char InputFileAVI::szME[]="AVI Import Filter";

InputFileAVI::InputFileAVI() {
	stripesys = NULL;
	stripe_files = NULL;
	fAutomated	= false;

	fAcceptPartial = false;
	fInternalDecoder = VDPreferencesIsPreferInternalDecodersEnabled();
	fDisableFastIO = false;
	iMJPEGMode = 0;
	fccForceVideo = 0;
	fccForceVideoHandler = 0;
	lForceAudioHz = 0;

	pAVIFile = NULL;

	fCompatibilityMode = fRedoKeyFlags = false;

	fAutoscanSegments = false;
}

InputFileAVI::~InputFileAVI() {
	audioSrc = NULL;
	videoSrc = NULL;

	if (stripe_files) {
		int i;

		for(i=0; i<stripe_count; i++)
			if (stripe_files[i])
				stripe_files[i]->Release();

		delete stripe_files;
	}
	delete stripesys;

	if (pAVIFile)
		pAVIFile->Release();
}

///////////////////////////////////////////////

class InputFileAVIOptions : public InputFileOptions {
public:
	struct InputFileAVIOpts {
		int len;
		int iMJPEGMode;
		FOURCC fccForceVideo;
		FOURCC fccForceVideoHandler;
		long lForceAudioHz;

		bool fCompatibilityMode;
		bool fAcceptPartial;
		bool fRedoKeyFlags;
		bool fInternalDecoder;
		bool fDisableFastIO;
	} opts;
		
	~InputFileAVIOptions();

	bool read(const char *buf);
	int write(char *buf, int buflen);

	static INT_PTR APIENTRY SetupDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
};

InputFileAVIOptions::~InputFileAVIOptions() {
}

bool InputFileAVIOptions::read(const char *buf) {
	const InputFileAVIOpts *pp = (const InputFileAVIOpts *)buf;

	if (pp->len != sizeof(InputFileAVIOpts))
		return false;

	opts = *pp;

	return true;
}

int InputFileAVIOptions::write(char *buf, int buflen) {
	InputFileAVIOpts *pp = (InputFileAVIOpts *)buf;

	if (buflen<sizeof(InputFileAVIOpts))
		return 0;

	opts.len = sizeof(InputFileAVIOpts);
	*pp = opts;

	return sizeof(InputFileAVIOpts);
}

///////

INT_PTR APIENTRY InputFileAVIOptions::SetupDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
 	InputFileAVIOptions *thisPtr = (InputFileAVIOptions *)GetWindowLongPtr(hDlg, DWLP_USER);

	switch(message) {
		case WM_INITDIALOG:
			SetWindowLongPtr(hDlg, DWLP_USER, lParam);
			SendDlgItemMessage(hDlg, IDC_FORCE_FOURCC, EM_LIMITTEXT, 4, 0);
			CheckDlgButton(hDlg, IDC_IF_NORMAL, BST_CHECKED);
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
			case IDCANCEL:
				if (GetDlgItem(hDlg, IDC_ACCEPTPARTIAL))
					thisPtr->opts.fAcceptPartial = !!IsDlgButtonChecked(hDlg, IDC_ACCEPTPARTIAL);

				thisPtr->opts.fCompatibilityMode = !!IsDlgButtonChecked(hDlg, IDC_AVI_COMPATIBILITYMODE);
				thisPtr->opts.fRedoKeyFlags = !!IsDlgButtonChecked(hDlg, IDC_AVI_REKEY);
				thisPtr->opts.fInternalDecoder = !!IsDlgButtonChecked(hDlg, IDC_AVI_INTERNALDECODER);
				thisPtr->opts.fDisableFastIO = !!IsDlgButtonChecked(hDlg, IDC_AVI_DISABLEOPTIMIZEDIO);

				if (IsDlgButtonChecked(hDlg, IDC_IF_NORMAL))
					thisPtr->opts.iMJPEGMode = VideoSourceAVI::IFMODE_NORMAL;
				else if (IsDlgButtonChecked(hDlg, IDC_IF_SWAP))
					thisPtr->opts.iMJPEGMode = VideoSourceAVI::IFMODE_SWAP;
				else if (IsDlgButtonChecked(hDlg, IDC_IF_SPLITNOSWAP))
					thisPtr->opts.iMJPEGMode = VideoSourceAVI::IFMODE_SPLIT1;
				else if (IsDlgButtonChecked(hDlg, IDC_IF_SPLITSWAP))
					thisPtr->opts.iMJPEGMode = VideoSourceAVI::IFMODE_SPLIT2;
				else if (IsDlgButtonChecked(hDlg, IDC_IF_DISCARDFIRST))
					thisPtr->opts.iMJPEGMode = VideoSourceAVI::IFMODE_DISCARD1;
				else if (IsDlgButtonChecked(hDlg, IDC_IF_DISCARDSECOND))
					thisPtr->opts.iMJPEGMode = VideoSourceAVI::IFMODE_DISCARD2;

				if (IsDlgButtonChecked(hDlg, IDC_FORCE_FOURCC)) {
					union {
						char c[5];
						FOURCC fccType;
					};
					int i;

					i = SendDlgItemMessage(hDlg, IDC_FOURCC, WM_GETTEXT, sizeof c, (LPARAM)c);

					memset(c+i, ' ', 5-i);

					if (fccType == 0x20202020)
						fccType = ' BID';		// force nothing to DIB, since 0 means no force

					thisPtr->opts.fccForceVideo = fccType;
				} else
					thisPtr->opts.fccForceVideo = 0;

				if (IsDlgButtonChecked(hDlg, IDC_FORCE_HANDLER)) {
					union {
						char c[5];
						FOURCC fccType;
					};
					int i;

					i = SendDlgItemMessage(hDlg, IDC_FOURCC2, WM_GETTEXT, sizeof c, (LPARAM)c);

					memset(c+i, ' ', 5-i);

					if (fccType == 0x20202020)
						fccType = ' BID';		// force nothing to DIB, since 0 means no force

					thisPtr->opts.fccForceVideoHandler = fccType;
				} else
					thisPtr->opts.fccForceVideoHandler = 0;

				if (IsDlgButtonChecked(hDlg, IDC_FORCE_SAMPRATE))
					thisPtr->opts.lForceAudioHz = GetDlgItemInt(hDlg, IDC_AUDIORATE, NULL, FALSE);
				else
					thisPtr->opts.lForceAudioHz = 0;
				
				EndDialog(hDlg, 0);
				return TRUE;

			case IDC_FORCE_FOURCC:
				EnableWindow(GetDlgItem(hDlg, IDC_FOURCC), IsDlgButtonChecked(hDlg, IDC_FORCE_FOURCC));
				return TRUE;

			case IDC_FORCE_HANDLER:
				EnableWindow(GetDlgItem(hDlg, IDC_FOURCC2), IsDlgButtonChecked(hDlg, IDC_FORCE_HANDLER));
				return TRUE;

			case IDC_FORCE_SAMPRATE:
				EnableWindow(GetDlgItem(hDlg, IDC_AUDIORATE), IsDlgButtonChecked(hDlg, IDC_FORCE_SAMPRATE));
				return TRUE;
			}
			break;
	}

	return FALSE;
}

void InputFileAVI::setOptions(InputFileOptions *_ifo) {
	InputFileAVIOptions *ifo = (InputFileAVIOptions *)_ifo;

	fCompatibilityMode	= ifo->opts.fCompatibilityMode;
	fAcceptPartial		= ifo->opts.fAcceptPartial;
	fRedoKeyFlags		= ifo->opts.fRedoKeyFlags;
	fInternalDecoder	= ifo->opts.fInternalDecoder;
	fDisableFastIO		= ifo->opts.fDisableFastIO;
	iMJPEGMode			= ifo->opts.iMJPEGMode;
	fccForceVideo		= ifo->opts.fccForceVideo;
	fccForceVideoHandler= ifo->opts.fccForceVideoHandler;
	lForceAudioHz		= ifo->opts.lForceAudioHz;
}

InputFileOptions *InputFileAVI::createOptions(const void *buf, uint32 len) {
	InputFileAVIOptions *ifo = new InputFileAVIOptions();

	if (!ifo) throw MyMemoryError();

	if (!ifo->read((const char *)buf)) {
		delete ifo;
		return NULL;
	}

	return ifo;
}

InputFileOptions *InputFileAVI::promptForOptions(HWND hwnd) {
	InputFileAVIOptions *ifo = new InputFileAVIOptions();

	if (!ifo) throw MyMemoryError();

	DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_EXTOPENOPTS_AVI),
			hwnd, InputFileAVIOptions::SetupDlgProc, (LPARAM)ifo);

	// if we were forced to AVIFile mode (possibly due to an Avisynth script),
	// propagate that condition to the options
	ifo->opts.fCompatibilityMode |= fCompatibilityMode;

	return ifo;
}

///////////////////////////////////////////////

void InputFileAVI::EnableSegmentAutoscan() {
	fAutoscanSegments = true;
}

void InputFileAVI::ForceCompatibility() {
	fCompatibilityMode = true;
}

void InputFileAVI::setAutomated(bool fAuto) {
	fAutomated = fAuto;
}

void InputFileAVI::Init(const wchar_t *szFile) {
	VDLogAppMessage(kVDLogMarker, kVDST_InputFileAVI, kVDM_OpeningFile, 1, &szFile);

	HRESULT err;
	PAVIFILE paf;

	AddFilename(szFile);

	if (fCompatibilityMode) {

		{
			VDExternalCodeBracket bracket(L"An AVIFile input handler", __FILE__, __LINE__);

			err = AVIFileOpen(&paf, VDTextWToA(szFile).c_str(), OF_READ, NULL);
		}

		if (err)
			throw MyAVIError(szME, err);

		if (!(pAVIFile = CreateAVIReadHandler(paf))) {
			AVIFileRelease(paf);
			throw MyMemoryError();
		}
	} else {
		if (!(pAVIFile = CreateAVIReadHandler(szFile)))
			throw MyMemoryError();
	}

	if (fDisableFastIO)
		pAVIFile->EnableFastIO(false);

	if (!(videoSrc = new VideoSourceAVI(pAVIFile, NULL, NULL, fInternalDecoder, iMJPEGMode, fccForceVideo, fccForceVideoHandler)))
		throw MyMemoryError();

	if (!videoSrc->init())
		throw MyError("%s: problem opening video stream", szME);

	if (fRedoKeyFlags)
		static_cast<VideoSourceAVI *>(&*videoSrc)->redoKeyFlags();
	else if (pAVIFile->isIndexFabricated() && !videoSrc->isKeyframeOnly()) {
		VDLogAppMessage(kVDLogWarning, kVDST_InputFileAVI, kVDM_RekeyNotSpecified);
	}

	audioSrc = new AudioSourceAVI(pAVIFile, fAutomated);
	if (!audioSrc->init()) {
		audioSrc = NULL;
	} else {
		WAVEFORMATEX *pwfex = (WAVEFORMATEX *)audioSrc->getFormat();

		if (pwfex->wFormatTag == WAVE_FORMAT_MPEGLAYER3 && pwfex->nBlockAlign == 0) {
			VDLogAppMessage(kVDLogWarning, kVDST_InputFileAVI, kVDM_InvalidBlockAlign);
			pwfex->nBlockAlign = 1;
		}

		if (lForceAudioHz) {
			pwfex->nAvgBytesPerSec = MulDiv(pwfex->nAvgBytesPerSec, lForceAudioHz, pwfex->nSamplesPerSec);
			pwfex->nSamplesPerSec = lForceAudioHz;
			static_cast<AudioSourceAVI *>(&*audioSrc)->setRate(VDFraction(pwfex->nAvgBytesPerSec, pwfex->nBlockAlign));
		}
	}

	if (!audioSrc && videoSrc->isType1()) {
		audioSrc = new AudioSourceDV(pAVIFile->GetStream('svai', 0), fAutomated);
		if (!audioSrc->init())
			audioSrc = NULL;
	}

	if (fAutoscanSegments) {
		const wchar_t *pszName = VDFileSplitPath(szFile);
		VDStringW sPathBase(szFile, pszName - szFile);
		VDStringW sPathTail(pszName);

		if (sPathTail.size() >= 7 && !_wcsicmp(sPathTail.c_str() + sPathTail.size() - 7, L".00.avi")) {
			int nSegment = 0;

			sPathTail.resize(sPathTail.size() - 7);

			VDStringW sPathPattern(VDMakePath(sPathBase.c_str(), sPathTail.c_str()));

			try {
				const char *pszPathHint;

				while(pAVIFile->getSegmentHint(&pszPathHint)) {

					++nSegment;

					VDStringW sPath(sPathPattern + VDswprintf(L".%02d.avi", 1, &nSegment));

					if (!VDDoesPathExist(sPath.c_str())) {
						if (pszPathHint && *pszPathHint) {
							sPathPattern = VDTextAToW(pszPathHint) + sPathTail;
						}

						sPath = (sPathPattern + VDswprintf(L".%02d.avi", 1, &nSegment));
					}

					if (!VDDoesPathExist(sPath.c_str())) {
						char szPath[MAX_PATH];
						wchar_t szTitle[MAX_PATH];

						swprintf(szTitle, MAX_PATH, L"Cannot find file %s", sPath.c_str());

						strcpy(szPath, VDTextWToA(sPath).c_str());

						const VDStringW fname(VDGetLoadFileName(VDFSPECKEY_LOADVIDEOFILE, (VDGUIHandle)g_hWnd, szTitle, fileFiltersAppend, L"avi", 0, 0));

						if (fname.empty())
							throw MyUserAbortError();

						if (!Append(fname.c_str()))
							break;

						sPathPattern = VDMakePath(VDFileSplitPathLeft(fname).c_str(), sPathTail.c_str());
					} else if (!Append(sPath.c_str()))
						break;
				}
			} catch(const MyError& e) {
				char err[128];
				sprintf(err, "Cannot load video segment %02d", nSegment);

				e.post(NULL, err);
			}
		}
	}
}

bool InputFileAVI::Append(const wchar_t *szFile) {
	if (fCompatibilityMode || stripesys)
		return false;

	if (!szFile)
		return true;

	if (pAVIFile->AppendFile(szFile)) {
		if (videoSrc)
			static_cast<VideoSourceAVI *>(&*videoSrc)->Reinit();
		if (audioSrc)
			static_cast<VDAudioSourceAVISourced *>(&*audioSrc)->Reinit();

		AddFilename(szFile);

		return true;
	}

	return false;
}

void InputFileAVI::InitStriped(const char *szFile) {
	int i;
	HRESULT err;
	PAVIFILE paf;
	IAVIReadHandler *index_file;

	if (!(stripesys = new AVIStripeSystem(szFile)))
		throw MyMemoryError();

	stripe_count = stripesys->getStripeCount();

	if (!(stripe_files = new IAVIReadHandler *[stripe_count]))
		throw MyMemoryError();

	for(i=0; i<stripe_count; i++)
		stripe_files[i]=NULL;

	for(i=0; i<stripe_count; i++) {
		AVIStripe *asdef = stripesys->getStripeInfo(i);

		// Ordinarily, OF_SHARE_DENY_WRITE would be better, but XingMPEG
		// Encoder requires write access to AVI files... *sigh*

		if (err = AVIFileOpen(&paf, asdef->szName, OF_READ | OF_SHARE_DENY_NONE, NULL))
			throw MyAVIError("AVI Striped Import Filter", err);

		if (!(stripe_files[i] = CreateAVIReadHandler(paf))) {
			AVIFileRelease(paf);
			throw MyMemoryError();
		}

		if (asdef->isIndex())
			index_file = stripe_files[i];
	}

	if (!(videoSrc = new VideoSourceAVI(index_file, stripesys, stripe_files, fInternalDecoder, iMJPEGMode, fccForceVideo)))
		throw MyMemoryError();

	if (!videoSrc->init())
		throw MyError("%s: problem opening video stream", szME);

	if (fRedoKeyFlags)
		static_cast<VideoSourceAVI *>(&*videoSrc)->redoKeyFlags();

	if (!(audioSrc = new AudioSourceAVI(index_file, fAutomated)))
		throw MyMemoryError();

	if (!audioSrc->init()) {
		audioSrc = NULL;
	}
}

void InputFileAVI::GetTextInfo(tFileTextInfo& info) {
	pAVIFile->GetTextInfo(info);	
}

bool InputFileAVI::isOptimizedForRealtime() {
	return pAVIFile->isOptimizedForRealtime();
}

bool InputFileAVI::isStreaming() {
	return pAVIFile->isStreaming();
}

///////////////////////////////////////////////////////////////////////////

typedef struct MyFileInfo {
	InputFileAVI *thisPtr;

	volatile HWND hWndAbort;
	UINT statTimer;
	long	lVideoKFrames;
	long	lVideoKMinSize;
	sint64 i64VideoKTotalSize;
	long	lVideoKMaxSize;
	long	lVideoCFrames;
	long	lVideoCMinSize;
	sint64	i64VideoCTotalSize;
	long	lVideoCMaxSize;

	long	lAudioFrames;
	long	lAudioMinSize;
	sint64	i64AudioTotalSize;
	long	lAudioMaxSize;

	long	lAudioPreload;

	bool	bAudioFramesIndeterminate;
} MyFileInfo;

void InputFileAVI::_InfoDlgThread(void *pvInfo) {
	MyFileInfo *pInfo = (MyFileInfo *)pvInfo;
	VDPosition i;
	uint32 lActualBytes, lActualSamples;
	VideoSourceAVI *inputVideoAVI = static_cast<VideoSourceAVI *>(&*pInfo->thisPtr->videoSrc);
	AudioSource *inputAudioAVI = pInfo->thisPtr->audioSrc;

	pInfo->lVideoCMinSize = 0x7FFFFFFF;
	pInfo->lVideoKMinSize = 0x7FFFFFFF;

	const VDPosition videoFrameStart	= inputVideoAVI->getStart();
	const VDPosition videoFrameEnd		= inputVideoAVI->getEnd();

	for(i=videoFrameStart; i<videoFrameEnd; ++i) {
		if (inputVideoAVI->isKey(i)) {
			++pInfo->lVideoKFrames;

			if (!inputVideoAVI->read(i, 1, NULL, 0, &lActualBytes, NULL)) {
				pInfo->i64VideoKTotalSize += lActualBytes;
				if (lActualBytes < pInfo->lVideoKMinSize) pInfo->lVideoKMinSize = lActualBytes;
				if (lActualBytes > pInfo->lVideoKMaxSize) pInfo->lVideoKMaxSize = lActualBytes;
			}
		} else {
			++pInfo->lVideoCFrames;

			if (!inputVideoAVI->read(i, 1, NULL, 0, &lActualBytes, NULL)) {
				pInfo->i64VideoCTotalSize += lActualBytes;
				if (lActualBytes < pInfo->lVideoCMinSize) pInfo->lVideoCMinSize = lActualBytes;
				if (lActualBytes > pInfo->lVideoCMaxSize) pInfo->lVideoCMaxSize = lActualBytes;
			}
		}

		if (pInfo->hWndAbort) {
			SendMessage(pInfo->hWndAbort, WM_USER+256, 0, 0);
			return;
		}
	}

	if (inputAudioAVI) {
		const VDPosition audioFrameStart	= inputAudioAVI->getStart();
		const VDPosition audioFrameEnd		= inputAudioAVI->getEnd();

		pInfo->lAudioMinSize = 0x7FFFFFFF;
		pInfo->bAudioFramesIndeterminate = false;
		pInfo->lAudioPreload = static_cast<VDAudioSourceAVISourced *>(inputAudioAVI)->GetPreloadSamples();

		i = audioFrameStart;
		while(i < audioFrameEnd) {
			if (inputAudioAVI->read(i, AVISTREAMREAD_CONVENIENT, NULL, 0, &lActualBytes, &lActualSamples))
				break;

			if (!lActualSamples) {
				pInfo->bAudioFramesIndeterminate = true;
				break;
			}

			++pInfo->lAudioFrames;
			i += lActualSamples;

			pInfo->i64AudioTotalSize += lActualBytes;
			if (lActualBytes < pInfo->lAudioMinSize) pInfo->lAudioMinSize = lActualBytes;
			if (lActualBytes > pInfo->lAudioMaxSize) pInfo->lAudioMaxSize = lActualBytes;

			if (pInfo->hWndAbort) {
				SendMessage(pInfo->hWndAbort, WM_USER+256, 0, 0);
				return;
			}
		}
	}

	pInfo->hWndAbort = (HWND)1;
}

INT_PTR APIENTRY InputFileAVI::_InfoDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	MyFileInfo *pInfo = (MyFileInfo *)GetWindowLongPtr(hDlg, DWLP_USER);
	InputFileAVI *thisPtr;

	if (pInfo)
		thisPtr = pInfo->thisPtr;

    switch (message)
    {
        case WM_INITDIALOG:
			{
				char buf[128];

				SetWindowLongPtr(hDlg, DWLP_USER, lParam);
				pInfo = (MyFileInfo *)lParam;
				thisPtr = pInfo->thisPtr;

				if (thisPtr->videoSrc) {
					char *s;
					VideoSourceAVI *pvs = static_cast<VideoSourceAVI *>(&*thisPtr->videoSrc);

					sprintf(buf, "%dx%d, %.3f fps (%ld �s)",
								thisPtr->videoSrc->getImageFormat()->biWidth,
								thisPtr->videoSrc->getImageFormat()->biHeight,
								thisPtr->videoSrc->getRate().asDouble(),
								VDRoundToLong(1000000.0 / thisPtr->videoSrc->getRate().asDouble()));
					SetDlgItemText(hDlg, IDC_VIDEO_FORMAT, buf);

					const sint64 length = thisPtr->videoSrc->getLength();
					s = buf + sprintf(buf, "%I64d frames (", length);
					DWORD ticks = VDRoundToInt(1000.0*length/thisPtr->videoSrc->getRate().asDouble());
					ticks_to_str(s, (buf + sizeof(buf)/sizeof(buf[0])) - s, ticks);
					sprintf(s+strlen(s),".%02d)", (ticks/10)%100);
					SetDlgItemText(hDlg, IDC_VIDEO_NUMFRAMES, buf);

					strcpy(buf, "Unknown");

					if (const wchar_t *name = pvs->getDecompressorName()) {
						VDTextWToA(buf, sizeof(buf)-7, name, -1);
						
						char fcc[5];
						*(long *)fcc = pvs->getImageFormat()->biCompression;
						fcc[4] = 0;
						for(int i=0; i<4; ++i)
							if ((uint8)(fcc[i] - 0x20) >= 0x7f)
								fcc[i] = ' ';

						sprintf(buf+strlen(buf), " (%s)", fcc);
					} else {
						const uint32 comp = pvs->getImageFormat()->biCompression;

						if (comp == '2YUY')
							strcpy(buf, "YCbCr 4:2:2 (YUY2)");
						else if (comp == 'YVYU')
							strcpy(buf, "YCbCr 4:2:2 (UYVY)");
						else if (comp == '024I')
							strcpy(buf, "YCbCr 4:2:0 planar (I420)");
						else if (comp == 'VUYI')
							strcpy(buf, "YCbCr 4:2:0 planar (IYUV)");
						else if (comp == '21VY')
							strcpy(buf, "YCbCr 4:2:0 planar (YV12)");
						else if (comp == '61VY')
							strcpy(buf, "YCbCr 4:2:2 planar (YV16)");
						else if (comp == '9UVY')
							strcpy(buf, "YCbCr 4:1:0 planar (YVU9)");
						else if (comp == '  8Y')
							strcpy(buf, "Monochrome (Y8)");
						else if (comp == '008Y')
							strcpy(buf, "Monochrome (Y800)");
						else
							sprintf(buf, "Uncompressed RGB%d", pvs->getImageFormat()->biBitCount);
					}

					SetDlgItemText(hDlg, IDC_VIDEO_COMPRESSION, buf);
				}
				if (thisPtr->audioSrc) {
					WAVEFORMATEX *fmt = thisPtr->audioSrc->getWaveFormat();
					DWORD cbwfxTemp;
					WAVEFORMATEX *pwfxTemp;
					HACMSTREAM has;
					HACMDRIVERID hadid;
					ACMDRIVERDETAILS add;
					bool fSuccessful = false;

					sprintf(buf, "%ldHz", fmt->nSamplesPerSec);
					SetDlgItemText(hDlg, IDC_AUDIO_SAMPLINGRATE, buf);

					if (fmt->nChannels == 8)
						strcpy(buf, "7.1");
					else if (fmt->nChannels == 6)
						strcpy(buf, "5.1");
					else if (fmt->nChannels > 2)
						sprintf(buf, "%d", fmt->nChannels);
					else
						sprintf(buf, "%d (%s)", fmt->nChannels, fmt->nChannels>1 ? "Stereo" : "Mono");
					SetDlgItemText(hDlg, IDC_AUDIO_CHANNELS, buf);

					if (fmt->wFormatTag == WAVE_FORMAT_PCM) {
						sprintf(buf, "%d-bit", fmt->wBitsPerSample);
						SetDlgItemText(hDlg, IDC_AUDIO_PRECISION, buf);
					} else
						SetDlgItemText(hDlg, IDC_AUDIO_PRECISION, "N/A");

					sint64 len = thisPtr->audioSrc->getLength();
					const WAVEFORMATEX *pWaveFormat = thisPtr->audioSrc->getWaveFormat();

					char *s = buf + sprintf(buf, "%I64d samples (", len);
					DWORD ticks = VDRoundToInt(1000.0*len*pWaveFormat->nBlockAlign/pWaveFormat->nAvgBytesPerSec);
					ticks_to_str(s, (buf + sizeof(buf)/sizeof(buf[0])) - s, ticks);
					sprintf(s+strlen(s),".%02d)", (ticks/10)%100);
					SetDlgItemText(hDlg, IDC_AUDIO_LENGTH, buf);

					////////// Attempt to detect audio compression //////////

					if (fmt->wFormatTag == nsVDWinFormats::kWAVE_FORMAT_EXTENSIBLE) {
						const nsVDWinFormats::WaveFormatExtensible& wfe = *(const nsVDWinFormats::WaveFormatExtensible *)fmt;

						if (wfe.mGuid == nsVDWinFormats::kKSDATAFORMAT_SUBTYPE_PCM) {
							sprintf(buf, "PCM (%d bits real, chmask %x)", wfe.mBitDepth, wfe.mChannelMask);
							SetDlgItemText(hDlg, IDC_AUDIO_COMPRESSION, buf);
						} else {
							SetDlgItemText(hDlg, IDC_AUDIO_COMPRESSION, "Unknown extended format");
						}
					} else if (fmt->wFormatTag != WAVE_FORMAT_PCM) {
						// Retrieve maximum format size.

						acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, (LPVOID)&cbwfxTemp);

						// Fill out a destination wave format (PCM).

						if (pwfxTemp = (WAVEFORMATEX *)allocmem(cbwfxTemp)) {
							pwfxTemp->wFormatTag	= WAVE_FORMAT_PCM;

							// Ask ACM to fill out the details.

							if (!acmFormatSuggest(NULL, fmt, pwfxTemp, cbwfxTemp, ACM_FORMATSUGGESTF_WFORMATTAG)) {
								if (!acmStreamOpen(&has, NULL, fmt, pwfxTemp, NULL, NULL, NULL, ACM_STREAMOPENF_NONREALTIME)) {
									if (!acmDriverID((HACMOBJ)has, &hadid, 0)) {
										memset(&add, 0, sizeof add);

										add.cbStruct = sizeof add;

										if (!acmDriverDetails(hadid, &add, 0)) {
											SetDlgItemText(hDlg, IDC_AUDIO_COMPRESSION, add.szLongName);

											fSuccessful = true;
										}
									}

									acmStreamClose(has, 0);
								}
							}

							freemem(pwfxTemp);
						}

						if (!fSuccessful) {
							char buf[32];

							wsprintf(buf, "Unknown (tag %04X)", fmt->wFormatTag);
							SetDlgItemText(hDlg, IDC_AUDIO_COMPRESSION, buf);
						}
					} else {
						// It's a PCM format...

						SetDlgItemText(hDlg, IDC_AUDIO_COMPRESSION, "PCM (Uncompressed)");
					}
				}
			}

			_beginthread(_InfoDlgThread, 10000, pInfo);

			pInfo->statTimer = SetTimer(hDlg, 1, 250, NULL);

            return (TRUE);

        case WM_COMMAND:                      
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) 
            {
				if (pInfo->hWndAbort == (HWND)1)
					EndDialog(hDlg, TRUE);
				else
					pInfo->hWndAbort = hDlg;
                return TRUE;
            }
            break;

		case WM_DESTROY:
			if (pInfo->statTimer) KillTimer(hDlg, pInfo->statTimer);
			break;

		case WM_TIMER:
			{
				char buf[128];

				sprintf(buf, "%ld", pInfo->lVideoKFrames);
				SetDlgItemText(hDlg, IDC_VIDEO_NUMKEYFRAMES, buf);

				if (pInfo->lVideoKFrames)
					sprintf(buf, "%ld/%I64d/%ld (%I64dK)"
								,pInfo->lVideoKMinSize
								,pInfo->i64VideoKTotalSize/pInfo->lVideoKFrames
								,pInfo->lVideoKMaxSize
								,(pInfo->i64VideoKTotalSize+1023)>>10);
				else
					strcpy(buf,"(no key frames)");
				SetDlgItemText(hDlg, IDC_VIDEO_KEYFRAMESIZES, buf);

				if (pInfo->lVideoCFrames)
					sprintf(buf, "%ld/%I64d/%ld (%I64dK)"
								,pInfo->lVideoCMinSize
								,pInfo->i64VideoCTotalSize/pInfo->lVideoCFrames
								,pInfo->lVideoCMaxSize
								,(pInfo->i64VideoCTotalSize+1023)>>10);
				else
					strcpy(buf,"(no delta frames)");
				SetDlgItemText(hDlg, IDC_VIDEO_NONKEYFRAMESIZES, buf);

				if (thisPtr->audioSrc) {
					if (pInfo->bAudioFramesIndeterminate) {
						SetDlgItemText(hDlg, IDC_AUDIO_NUMFRAMES, "(indeterminate)");
						SetDlgItemText(hDlg, IDC_AUDIO_FRAMESIZES, "(indeterminate)");
						SetDlgItemText(hDlg, IDC_AUDIO_PRELOAD, "(indeterminate)");
					} else {

						if (pInfo->lAudioFrames)
							sprintf(buf, "%ld/%I64d/%ld (%I64dK)"
									,pInfo->lAudioMinSize
									,pInfo->i64AudioTotalSize/pInfo->lAudioFrames
									,pInfo->lAudioMaxSize
									,(pInfo->i64AudioTotalSize+1023)>>10);
						else
							strcpy(buf,"(no audio frames)");
						SetDlgItemText(hDlg, IDC_AUDIO_FRAMESIZES, buf);

						const WAVEFORMATEX *pWaveFormat = thisPtr->audioSrc->getWaveFormat();

						sprintf(buf, "%ld chunks (%.2fs preload)", pInfo->lAudioFrames,
								(double)pInfo->lAudioPreload * pWaveFormat->nBlockAlign / pWaveFormat->nAvgBytesPerSec
								);
						SetDlgItemText(hDlg, IDC_AUDIO_LAYOUT, buf);

						const double audioRate = (double)pWaveFormat->nAvgBytesPerSec * (1.0 / 125.0);
						const double rawOverhead = 24.0 * pInfo->lAudioFrames;
						const double audioOverhead = 100.0 * rawOverhead / (rawOverhead + pInfo->i64AudioTotalSize);
						sprintf(buf, "%.0f kbps (%.2f%% overhead)", audioRate, audioOverhead);
						SetDlgItemText(hDlg, IDC_AUDIO_DATARATE, buf);
					}
				}

				double totalVideoFrames = (double)pInfo->lVideoKFrames + (sint64)pInfo->lVideoCFrames;
				if (totalVideoFrames > 0) {
					VideoSourceAVI *pvs = static_cast<VideoSourceAVI *>(&*thisPtr->videoSrc);
					const double seconds = (double)pvs->getLength() / (double)pvs->getRate().asDouble();
					const double rawOverhead = (24.0 * totalVideoFrames);
					const double totalSize = (double)(pInfo->i64VideoKTotalSize + pInfo->i64VideoCTotalSize);
					const double videoRate = (1.0 / 125.0) * totalSize / seconds;
					const double videoOverhead = totalSize > 0 ? 100.0 * rawOverhead / (rawOverhead + totalSize) : 0;
					sprintf(buf, "%.0f kbps (%.2f%% overhead)", videoRate, videoOverhead);
					SetDlgItemText(hDlg, IDC_VIDEO_DATARATE, buf);
				}
			}

			/////////

			if (pInfo->hWndAbort) {
				KillTimer(hDlg, pInfo->statTimer);
				return TRUE;
			}

			break;

		case WM_USER+256:
			EndDialog(hDlg, TRUE);  
			break;
    }
    return FALSE;
}

void InputFileAVI::InfoDialog(HWND hwndParent) {
	MyFileInfo mai;

	memset(&mai, 0, sizeof mai);
	mai.thisPtr = this;

	DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_AVI_INFO), hwndParent, _InfoDlgProc, (LPARAM)&mai);
}
