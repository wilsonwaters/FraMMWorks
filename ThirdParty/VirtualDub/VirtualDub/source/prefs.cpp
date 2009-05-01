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

#define f_PREFS_CPP

#include <windows.h>
#include <commctrl.h>

#include <vd2/Dita/interface.h>
#include <vd2/system/registry.h>
#include <vd2/system/fileasync.h>
#include <vd2/Riza/display.h>

#include "resource.h"
#include "helpfile.h"

#include "gui.h"
#include "oshelper.h"
#include "dub.h"
#include "dubstatus.h"
#include "prefs.h"

extern HINSTANCE g_hInst;

namespace {
	struct VDPreferences2 {
		Preferences		mOldPrefs;
		VDStringW		mTimelineFormat;
		bool			mbAllowDirectYCbCrDecoding;
		bool			mbDisplayEnableDebugInfo;
		bool			mbConfirmRenderAbort;
		bool			mbEnableAVIAlignmentThreshold;
		bool			mbPreferInternalDecoders;
		uint32			mAVIAlignmentThreshold;
		uint32			mRenderOutputBufferSize;
		uint32			mRenderWaveBufferSize;
		uint32			mRenderVideoBufferCount;
		uint32			mRenderThrottlePercent;
		VDStringW		mD3DFXFile;
		uint32			mFileAsyncDefaultMode;
		uint32			mAVISuperindexLimit;
		uint32			mAVISubindexLimit;

		bool			mbDisplayAllowDirectXOverlays;
	} g_prefs2;
}

void VDSavePreferences(VDPreferences2& prefs);

Preferences g_prefs={
	{ 0, PreferencesMain::DEPTH_FASTEST, 0, TRUE, 0 },
	{ 50*16, 4*16 },
};

static char g_szMainPrefs[]="Main prefs";

void VDPreferencesUpdated();

////////////////////////////////////////////////////////////////

class VDDialogPreferencesGeneral : public VDDialogBase {
public:
	VDPreferences2& mPrefs;
	VDDialogPreferencesGeneral(VDPreferences2& p) : mPrefs(p) {}

	bool HandleUIEvent(IVDUIBase *pBase, IVDUIWindow *pWin, uint32 id, eEventType type, int item) {
		switch(type) {
		case kEventAttach:
			mpBase = pBase;
			SetValue(100, mPrefs.mOldPrefs.main.iPreviewDepth);
			SetValue(101, mPrefs.mOldPrefs.main.iPreviewPriority);
			SetValue(102, mPrefs.mOldPrefs.main.iDubPriority);
			SetValue(103, mPrefs.mOldPrefs.main.fAttachExtension);

			pWin = mpBase->GetControl(104);
			{
				IVDUITrackbar *pTrackbar = vdpoly_cast<IVDUITrackbar *>(pWin);

				if (pTrackbar) {
					pTrackbar->SetRange(1, 10);
					pWin->SetValue((mPrefs.mRenderThrottlePercent + 5) / 10);
				}
			}
			pBase->ExecuteAllLinks();
			return true;
		case kEventSync:
		case kEventDetach:
			mPrefs.mOldPrefs.main.iPreviewDepth		= (char)GetValue(100);
			mPrefs.mOldPrefs.main.iPreviewPriority	= (char)GetValue(101);
			mPrefs.mOldPrefs.main.iDubPriority		= (char)GetValue(102);
			mPrefs.mOldPrefs.main.fAttachExtension	= (char)GetValue(103);
			mPrefs.mRenderThrottlePercent			= GetValue(104) * 10;
			return true;
		}
		return false;
	}
};

class VDDialogPreferencesDisplay : public VDDialogBase {
public:
	VDPreferences2& mPrefs;
	VDDialogPreferencesDisplay(VDPreferences2& p) : mPrefs(p) {}

	bool HandleUIEvent(IVDUIBase *pBase, IVDUIWindow *pWin, uint32 id, eEventType type, int item) {
		switch(type) {
		case kEventAttach:
			mpBase = pBase;
			SetValue(100, 0 != (mPrefs.mOldPrefs.fDisplay & Preferences::kDisplayDither16));
			SetValue(101,     !(mPrefs.mOldPrefs.fDisplay & Preferences::kDisplayDisableDX));
			SetValue(102, 0 != (mPrefs.mOldPrefs.fDisplay & Preferences::kDisplayUseDXWithTS));
			SetValue(103, 0 != (mPrefs.mOldPrefs.fDisplay & Preferences::kDisplayEnableD3D));
			SetValue(104, 0 != (mPrefs.mOldPrefs.fDisplay & Preferences::kDisplayEnableOpenGL));
			SetValue(105, 0 != (mPrefs.mOldPrefs.fDisplay & Preferences::kDisplayEnableD3DFX));
			SetValue(106, 0 != (mPrefs.mOldPrefs.fDisplay & Preferences::kDisplayEnableVSync));
			SetValue(107, mPrefs.mbDisplayAllowDirectXOverlays);
			SetValue(108, mPrefs.mbDisplayEnableDebugInfo);
			SetCaption(300, mPrefs.mD3DFXFile.c_str());
			pBase->ExecuteAllLinks();
			return true;
		case kEventSync:
		case kEventDetach:
			mPrefs.mOldPrefs.fDisplay = 0;
			if ( GetValue(100)) mPrefs.mOldPrefs.fDisplay |= Preferences::kDisplayDither16;
			if (!GetValue(101)) mPrefs.mOldPrefs.fDisplay |= Preferences::kDisplayDisableDX;
			if ( GetValue(102)) mPrefs.mOldPrefs.fDisplay |= Preferences::kDisplayUseDXWithTS;
			if ( GetValue(103)) mPrefs.mOldPrefs.fDisplay |= Preferences::kDisplayEnableD3D;
			if ( GetValue(104)) mPrefs.mOldPrefs.fDisplay |= Preferences::kDisplayEnableOpenGL;
			if ( GetValue(105)) mPrefs.mOldPrefs.fDisplay |= Preferences::kDisplayEnableD3DFX;
			if ( GetValue(106)) mPrefs.mOldPrefs.fDisplay |= Preferences::kDisplayEnableVSync;
			mPrefs.mbDisplayAllowDirectXOverlays = GetValue(107) != 0;
			mPrefs.mbDisplayEnableDebugInfo = GetValue(108) != 0;
			mPrefs.mD3DFXFile = GetCaption(300);
			return true;
		}
		return false;
	}
};

class VDDialogPreferencesCPU : public VDDialogBase {
public:
	VDPreferences2& mPrefs;
	VDDialogPreferencesCPU(VDPreferences2& p) : mPrefs(p) {}

	bool HandleUIEvent(IVDUIBase *pBase, IVDUIWindow *pWin, uint32 id, eEventType type, int item) {
		switch(type) {
		case kEventAttach:
			mpBase = pBase;
			SetValue(100, 0 != (mPrefs.mOldPrefs.main.fOptimizations & PreferencesMain::OPTF_FORCE));
			SetValue(200, 0 != (mPrefs.mOldPrefs.main.fOptimizations & PreferencesMain::OPTF_FPU));
			SetValue(201, 0 != (mPrefs.mOldPrefs.main.fOptimizations & PreferencesMain::OPTF_MMX));
			SetValue(202, 0 != (mPrefs.mOldPrefs.main.fOptimizations & PreferencesMain::OPTF_INTEGER_SSE));
			SetValue(203, 0 != (mPrefs.mOldPrefs.main.fOptimizations & PreferencesMain::OPTF_SSE));
			SetValue(204, 0 != (mPrefs.mOldPrefs.main.fOptimizations & PreferencesMain::OPTF_SSE2));
			SetValue(205, 0 != (mPrefs.mOldPrefs.main.fOptimizations & PreferencesMain::OPTF_3DNOW));
			SetValue(206, 0 != (mPrefs.mOldPrefs.main.fOptimizations & PreferencesMain::OPTF_3DNOW_EXT));
			pBase->ExecuteAllLinks();
			return true;
		case kEventSync:
		case kEventDetach:
			mPrefs.mOldPrefs.main.fOptimizations = 0;
			if (GetValue(100)) mPrefs.mOldPrefs.main.fOptimizations |= PreferencesMain::OPTF_FORCE;
			if (GetValue(200)) mPrefs.mOldPrefs.main.fOptimizations |= PreferencesMain::OPTF_FPU;
			if (GetValue(201)) mPrefs.mOldPrefs.main.fOptimizations |= PreferencesMain::OPTF_MMX;
			if (GetValue(202)) mPrefs.mOldPrefs.main.fOptimizations |= PreferencesMain::OPTF_INTEGER_SSE;
			if (GetValue(203)) mPrefs.mOldPrefs.main.fOptimizations |= PreferencesMain::OPTF_SSE;
			if (GetValue(204)) mPrefs.mOldPrefs.main.fOptimizations |= PreferencesMain::OPTF_SSE2;
			if (GetValue(205)) mPrefs.mOldPrefs.main.fOptimizations |= PreferencesMain::OPTF_3DNOW;
			if (GetValue(206)) mPrefs.mOldPrefs.main.fOptimizations |= PreferencesMain::OPTF_3DNOW_EXT;
			return true;
		}
		return false;
	}
};

class VDDialogPreferencesScene : public VDDialogBase {
public:
	VDPreferences2& mPrefs;
	VDDialogPreferencesScene(VDPreferences2& p) : mPrefs(p) {}

	bool HandleUIEvent(IVDUIBase *pBase, IVDUIWindow *pWin, uint32 id, eEventType type, int item) {
		switch(type) {
		case kEventAttach:
			{
				mpBase = pBase;

				IVDUIWindow *pWin = mpBase->GetControl(100);
				IVDUITrackbar *pTrackbar = vdpoly_cast<IVDUITrackbar *>(pWin);

				if (pTrackbar) {
					pTrackbar->SetRange(0, 255);
					pWin->SetValue(mPrefs.mOldPrefs.scene.iCutThreshold ? 256 - ((mPrefs.mOldPrefs.scene.iCutThreshold+8)>>4) : 0);
				}

				pWin = mpBase->GetControl(200);
				pTrackbar = vdpoly_cast<IVDUITrackbar *>(pWin);

				if (pTrackbar) {
					pTrackbar->SetRange(0, 255);
					pWin->SetValue(mPrefs.mOldPrefs.scene.iFadeThreshold);
				}

				SyncLabels();
				pBase->ExecuteAllLinks();
			}
			return true;
		case kEventSync:
		case kEventDetach:
			{
				int v = GetValue(100);
				mPrefs.mOldPrefs.scene.iCutThreshold = v ? (256-v)<<4 : 0;
				mPrefs.mOldPrefs.scene.iFadeThreshold = GetValue(200);
			}
			return true;

		case kEventSelect:
			SyncLabels();
			return true;
		}
		return false;
	}

	void SyncLabels() {
		int v = GetValue(100);

		if (!v)
			SetCaption(101, L"Off");
		else
			SetCaption(101, VDswprintf(L"%u", 1, &v).c_str());

		v = GetValue(200);
		if (!v)
			SetCaption(201, L"Off");
		else
			SetCaption(201, VDswprintf(L"%u", 1, &v).c_str());
	}
};

class VDDialogPreferencesAVI : public VDDialogBase {
public:
	VDPreferences2& mPrefs;
	VDDialogPreferencesAVI(VDPreferences2& p) : mPrefs(p) {}

	bool HandleUIEvent(IVDUIBase *pBase, IVDUIWindow *pWin, uint32 id, eEventType type, int item) {
		switch(type) {
		case kEventAttach:
			mpBase = pBase;
			SetValue(100, 0 != (mPrefs.mOldPrefs.fAVIRestrict1Gb));
			SetValue(101, 0 != (mPrefs.mOldPrefs.fNoCorrectLayer3));
			SetValue(102, mPrefs.mbAllowDirectYCbCrDecoding);
			SetValue(103, mPrefs.mbEnableAVIAlignmentThreshold);
			{
				unsigned v = mPrefs.mAVIAlignmentThreshold;
				SetCaption(200, VDswprintf(L"%u", 1, &v).c_str());
				v = mPrefs.mAVISuperindexLimit;
				SetCaption(201, VDswprintf(L"%u", 1, &v).c_str());
				v = mPrefs.mAVISubindexLimit;
				SetCaption(202, VDswprintf(L"%u", 1, &v).c_str());
			}
			SetValue(104, mPrefs.mbPreferInternalDecoders);
			pBase->ExecuteAllLinks();
			return true;
		case kEventDetach:
		case kEventSync:
			mPrefs.mOldPrefs.fAVIRestrict1Gb = 0 != GetValue(100);
			mPrefs.mOldPrefs.fNoCorrectLayer3 = 0 != GetValue(101);
			mPrefs.mbAllowDirectYCbCrDecoding = 0!=GetValue(102);
			if (mPrefs.mbEnableAVIAlignmentThreshold = (0 != GetValue(103)))
				mPrefs.mAVIAlignmentThreshold = (uint32)wcstoul(GetCaption(200).c_str(), 0, 10);
			mPrefs.mbPreferInternalDecoders = 0!=GetValue(104);
			mPrefs.mAVISubindexLimit = (uint32)wcstoul(GetCaption(201).c_str(), 0, 10);
			mPrefs.mAVISuperindexLimit = (uint32)wcstoul(GetCaption(202).c_str(), 0, 10);
			return true;
		}
		return false;
	}
};

class VDDialogPreferencesTimeline : public VDDialogBase {
public:
	VDPreferences2& mPrefs;
	VDDialogPreferencesTimeline(VDPreferences2& p) : mPrefs(p) {}

	bool HandleUIEvent(IVDUIBase *pBase, IVDUIWindow *pWin, uint32 id, eEventType type, int item) {
		switch(type) {
		case kEventAttach:
			mpBase = pBase;
			pBase->ExecuteAllLinks();
			SetCaption(200, mPrefs.mTimelineFormat.c_str());
			return true;
		case kEventDetach:
		case kEventSync:
			mPrefs.mTimelineFormat = GetCaption(200);
			return true;
		}
		return false;
	}
};

class VDDialogPreferencesDub : public VDDialogBase {
public:
	VDPreferences2& mPrefs;
	VDDialogPreferencesDub(VDPreferences2& p) : mPrefs(p) {}

	bool HandleUIEvent(IVDUIBase *pBase, IVDUIWindow *pWin, uint32 id, eEventType type, int item) {
		switch(type) {
		case kEventAttach:
			mpBase = pBase;
			pBase->ExecuteAllLinks();
			SetValue(100, mPrefs.mbConfirmRenderAbort);
			return true;
		case kEventDetach:
		case kEventSync:
			mPrefs.mbConfirmRenderAbort = 0 != GetValue(100);
			return true;
		}
		return false;
	}
};

class VDDialogPreferencesDiskIO : public VDDialogBase {
public:
	VDPreferences2& mPrefs;
	VDDialogPreferencesDiskIO(VDPreferences2& p) : mPrefs(p) {}

	bool HandleUIEvent(IVDUIBase *pBase, IVDUIWindow *pWin, uint32 id, eEventType type, int item) {
		switch(type) {
		case kEventAttach:
			mpBase = pBase;
			pBase->ExecuteAllLinks();
			SetValue(100, mPrefs.mFileAsyncDefaultMode);
			return true;
		case kEventDetach:
		case kEventSync:
			mPrefs.mFileAsyncDefaultMode = GetValue(100);
			return true;
		}
		return false;
	}
};

class VDDialogPreferences : public VDDialogBase {
public:
	VDPreferences2& mPrefs;
	VDDialogPreferences(VDPreferences2& p) : mPrefs(p) {}

	bool HandleUIEvent(IVDUIBase *pBase, IVDUIWindow *pWin, uint32 id, eEventType type, int item) {
		if (type == kEventAttach) {
			mpBase = pBase;
			SetValue(100, 0);
			pBase->ExecuteAllLinks();
		} else if (id == 101 && type == kEventSelect) {
			IVDUIBase *pSubDialog = vdpoly_cast<IVDUIBase *>(pBase->GetControl(101)->GetStartingChild());

			if (pSubDialog) {
				switch(item) {
				case 0:	pSubDialog->SetCallback(new VDDialogPreferencesGeneral(mPrefs), true); break;
				case 1:	pSubDialog->SetCallback(new VDDialogPreferencesDisplay(mPrefs), true); break;
				case 2:	pSubDialog->SetCallback(new VDDialogPreferencesScene(mPrefs), true); break;
				case 3:	pSubDialog->SetCallback(new VDDialogPreferencesCPU(mPrefs), true); break;
				case 4:	pSubDialog->SetCallback(new VDDialogPreferencesAVI(mPrefs), true); break;
				case 5:	pSubDialog->SetCallback(new VDDialogPreferencesTimeline(mPrefs), true); break;
				case 6:	pSubDialog->SetCallback(new VDDialogPreferencesDub(mPrefs), true); break;
				case 7:	pSubDialog->SetCallback(new VDDialogPreferencesDiskIO(mPrefs), true); break;
				}
			}
		} else if (type == kEventSelect) {
			if (id == 10) {
				pBase->EndModal(true);
				return true;
			} else if (id == 11) {
				pBase->EndModal(false);
				return true;
			} else if (id == 12) {
				IVDUIBase *pSubDialog = vdpoly_cast<IVDUIBase *>(pBase->GetControl(101)->GetStartingChild());

				if (pSubDialog)
					pSubDialog->DispatchEvent(vdpoly_cast<IVDUIWindow *>(mpBase), 0, IVDUICallback::kEventSync, 0);

				VDSavePreferences(mPrefs);
			}
		}
		return false;
	}
};

void VDShowPreferencesDialog(VDGUIHandle h) {
	vdrefptr<IVDUIWindow> peer(VDUICreatePeer(h));

	vdrefptr<IVDUIWindow> pWin(VDCreateDialogFromResource(1000, peer));
	VDPreferences2 temp(g_prefs2);
	VDDialogPreferences prefDlg(temp);

	IVDUIBase *pBase = vdpoly_cast<IVDUIBase *>(pWin);
	
	pBase->SetCallback(&prefDlg, false);
	int result = pBase->DoModal();

	peer->Shutdown();
	pWin->Shutdown();

	if (result) {
		g_prefs2 = temp;
		g_prefs = g_prefs2.mOldPrefs;
		VDPreferencesUpdated();
	}
}

void LoadPreferences() {
	DWORD dwSize;
	Preferences prefs_t;

	dwSize = QueryConfigBinary("", g_szMainPrefs, (char *)&prefs_t, sizeof prefs_t);

	if (dwSize) {
		if (dwSize > sizeof g_prefs) dwSize = sizeof g_prefs;

		memcpy(&g_prefs, &prefs_t, dwSize);
	}

	VDRegistryAppKey key("Preferences");

	if (!key.getString("Timeline format", g_prefs2.mTimelineFormat))
		g_prefs2.mTimelineFormat = L"Frame %f (%h:%02m:%02s.%03t) [%c]";

	if (!key.getString("Direct3D FX file", g_prefs2.mD3DFXFile))
		g_prefs2.mD3DFXFile = L"display.fx";

	g_prefs2.mbAllowDirectYCbCrDecoding = key.getBool("Allow direct YCbCr decoding", true);
	g_prefs2.mbConfirmRenderAbort = key.getBool("Confirm render abort", true);
	g_prefs2.mbEnableAVIAlignmentThreshold = key.getBool("AVI: Alignment threshold enable", false);
	g_prefs2.mAVIAlignmentThreshold = key.getInt("AVI: Alignment threshold", 524288);
	g_prefs2.mbPreferInternalDecoders = key.getBool("AVI: Prefer internal decoders", false);
	g_prefs2.mRenderOutputBufferSize = std::max<uint32>(65536, std::min<uint32>(0x10000000, key.getInt("Render: Output buffer size", 2097152)));
	g_prefs2.mRenderWaveBufferSize = std::max<uint32>(65536, std::min<uint32>(0x10000000, key.getInt("Render: Wave buffer size", 65536)));
	g_prefs2.mRenderVideoBufferCount = std::max<uint32>(1, std::min<uint32>(65536, key.getInt("Render: Video buffer count", 32)));
	g_prefs2.mRenderThrottlePercent = std::max<uint32>(10, std::min<uint32>(100, key.getInt("Render: Default throttle percent", 100)));
	g_prefs2.mFileAsyncDefaultMode = std::min<uint32>(IVDFileAsync::kModeCount-1, key.getInt("File: Async mode", IVDFileAsync::kModeAsynchronous));
	g_prefs2.mAVISuperindexLimit = key.getInt("AVI: Superindex entry limit", 256);
	g_prefs2.mAVISubindexLimit = key.getInt("AVI: Subindex entry limit", 8192);

	g_prefs2.mbDisplayAllowDirectXOverlays = key.getBool("Display: Allow DirectX overlays", true);
	g_prefs2.mbDisplayEnableDebugInfo = key.getBool("Display: Enable debug info", false);

	g_prefs2.mOldPrefs = g_prefs;

	VDPreferencesUpdated();
}

void VDSavePreferences(VDPreferences2& prefs) {
	SetConfigBinary("", g_szMainPrefs, (char *)&prefs.mOldPrefs, sizeof prefs.mOldPrefs);

	VDRegistryAppKey key("Preferences");
	key.setString("Timeline format", prefs.mTimelineFormat.c_str());
	key.setBool("Allow direct YCbCr decoding", prefs.mbAllowDirectYCbCrDecoding);
	key.setBool("Confirm render abort", prefs.mbConfirmRenderAbort);
	key.setBool("AVI: Alignment threshold enable", prefs.mbEnableAVIAlignmentThreshold);
	key.setInt("AVI: Alignment threshold", prefs.mAVIAlignmentThreshold);
	key.setBool("AVI: Prefer internal decoders", prefs.mbPreferInternalDecoders);
	key.setString("Direct3D FX file", prefs.mD3DFXFile.c_str());
	key.setInt("Render: Output buffer size", prefs.mRenderOutputBufferSize);
	key.setInt("Render: Wave buffer size", prefs.mRenderWaveBufferSize);
	key.setInt("Render: Video buffer count", prefs.mRenderVideoBufferCount);
	key.setInt("Render: Default throttle percent", prefs.mRenderThrottlePercent);
	key.setInt("File: Async mode", prefs.mFileAsyncDefaultMode);
	key.setInt("AVI: Superindex entry limit", prefs.mAVISuperindexLimit);
	key.setInt("AVI: Subindex entry limit", prefs.mAVISubindexLimit);

	key.setBool("Display: Allow DirectX overlays", prefs.mbDisplayAllowDirectXOverlays);
	key.setBool("Display: Enable debug info", g_prefs2.mbDisplayEnableDebugInfo);
}

void VDSavePreferences() {
	VDSavePreferences(g_prefs2);
}

const VDStringW& VDPreferencesGetTimelineFormat() {
	return g_prefs2.mTimelineFormat;
}

bool VDPreferencesIsDirectYCbCrInputEnabled() {
	return g_prefs2.mbAllowDirectYCbCrDecoding;
}

bool VDPreferencesIsRenderAbortConfirmEnabled() {
	return g_prefs2.mbConfirmRenderAbort;
}

uint32 VDPreferencesGetAVIAlignmentThreshold() {
	return g_prefs2.mbEnableAVIAlignmentThreshold ? g_prefs2.mAVIAlignmentThreshold : 0;
}

void VDPreferencesGetAVIIndexingLimits(uint32& superindex, uint32& subindex) {
	superindex = g_prefs2.mAVISuperindexLimit;
	subindex = g_prefs2.mAVISubindexLimit;
}

bool VDPreferencesIsPreferInternalDecodersEnabled() {
	return g_prefs2.mbPreferInternalDecoders;
}

const VDStringW& VDPreferencesGetD3DFXFile() {
	return g_prefs2.mD3DFXFile;
}

uint32& VDPreferencesGetRenderOutputBufferSize() {
	return g_prefs2.mRenderOutputBufferSize;
}

uint32& VDPreferencesGetRenderWaveBufferSize() {
	return g_prefs2.mRenderWaveBufferSize;
}

uint32& VDPreferencesGetRenderVideoBufferCount() {
	return g_prefs2.mRenderVideoBufferCount;
}

uint32 VDPreferencesGetRenderThrottlePercent() {
	return g_prefs2.mRenderThrottlePercent;
}

uint32 VDPreferencesGetFileAsyncDefaultMode() {
	return g_prefs2.mFileAsyncDefaultMode;
}

void VDPreferencesUpdated() {
	VDVideoDisplaySetFeatures(
		!(g_prefs2.mOldPrefs.fDisplay & Preferences::kDisplayDisableDX),
		g_prefs2.mbDisplayAllowDirectXOverlays,
		!!(g_prefs2.mOldPrefs.fDisplay & Preferences::kDisplayUseDXWithTS),
		!!(g_prefs2.mOldPrefs.fDisplay & Preferences::kDisplayEnableOpenGL),
		!!(g_prefs2.mOldPrefs.fDisplay & Preferences::kDisplayEnableD3D),
		!!(g_prefs2.mOldPrefs.fDisplay & Preferences::kDisplayEnableD3DFX)
		);

	VDVideoDisplaySetD3DFXFileName(g_prefs2.mD3DFXFile.c_str());
	VDVideoDisplaySetDebugInfoEnabled(g_prefs2.mbDisplayEnableDebugInfo);
}
