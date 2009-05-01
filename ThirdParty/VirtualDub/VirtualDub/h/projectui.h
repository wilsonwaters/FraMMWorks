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

#ifndef f_PROJECTUI_H
#define f_PROJECTUI_H

#include <windows.h>
#include <shellapi.h>
#include <vd2/system/event.h>
#include <vd2/Riza/display.h>

#include "project.h"
#include "MRUList.h"
#include "PositionControl.h"
#include "uiframe.h"
#include "ParameterCurveControl.h"
#include "AudioDisplay.h"

class IVDPositionControl;
class IVDUIWindow;
class IVDUIAudioDisplayControl;

class VDProjectUI : public VDProject, public vdrefcounted<IVDUIFrameClient>, protected IVDVideoDisplayCallback, public IVDPositionControlCallback, public IVDProjectUICallback, public IVDUICallback {
public:
	VDProjectUI();
	~VDProjectUI();

	bool Attach(VDGUIHandle hwnd);
	void Detach();

	bool Tick();

	void SetTitle(int nTitleString, int nArgs, ...);

	void OpenAsk();
	void AppendAsk();
	void SaveAVIAsk();
	void SaveCompatibleAVIAsk();
	void SaveStripedAVIAsk();
	void SaveStripeMasterAsk();
	void SaveImageSequenceAsk();
	void SaveSegmentedAVIAsk();
	void SaveWAVAsk();
	void SaveFilmstripAsk();
	void SaveAnimatedGIFAsk();
	void SaveRawAudioAsk();
	void SaveConfigurationAsk();
	void LoadConfigurationAsk();
	void SetVideoFiltersAsk();
	void SetVideoFramerateOptionsAsk();
	void SetVideoDepthOptionsAsk();
	void SetVideoRangeOptionsAsk();
	void SetVideoCompressionAsk();
	void SetVideoErrorModeAsk();
	void SetAudioFiltersAsk();
	void SetAudioConversionOptionsAsk();
	void SetAudioInterleaveOptionsAsk();
	void SetAudioCompressionAsk();
	void SetAudioVolumeOptionsAsk();
	void SetAudioSourceWAVAsk();
	void SetAudioErrorModeAsk();
	void JumpToFrameAsk();

protected:
	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT MainWndProc( UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT DubWndProc(UINT msg, WPARAM wParam, LPARAM lParam);
	void OnSize();
	void HandleDragDrop(HDROP hdrop);
	void OnPreferencesChanged();
	bool MenuHit(UINT id);
	void RepaintMainWindow(HWND hWnd);
	void ShowMenuHelp(WPARAM wParam);
	bool DoFrameRightClick(LPARAM lParam);
	void UpdateMainMenu(HMENU hMenu);
	void UpdateDubMenu(HMENU hMenu);
	void RepositionPanes();
	void UpdateVideoFrameLayout();

	void OpenAudioDisplay();
	void CloseAudioDisplay();
	void UpdateAudioDisplay();
	void UpdateAudioDisplayPosition();

	void OpenCurveEditor();
	void CloseCurveEditor();
	void UpdateCurveList();

	void UIRefreshInputFrame(bool bValid);
	void UIRefreshOutputFrame(bool bValid);
	void UISetDubbingMode(bool bActive, bool bIsPreview);
	void UIRunDubMessageLoop();
	void UICurrentPositionUpdated();
	void UISelectionUpdated(bool notifyUser);
	void UITimelineUpdated();
	void UIShuttleModeUpdated();
	void UISourceFileUpdated();
	void UIAudioSourceUpdated();
	void UIVideoSourceUpdated();
	void UIVideoFiltersUpdated();
	void UIDubParametersUpdated();

	void UpdateMRUList();
	void SetStatus(const wchar_t *s);

	void DisplayRequestUpdate(IVDVideoDisplay *pDisp);

	bool GetFrameString(wchar_t *buf, size_t buflen, VDPosition dstFrame);

	void LoadSettings();
	void SaveSettings();

	bool HandleUIEvent(IVDUIBase *pBase, IVDUIWindow *pWin, uint32 id, eEventType type, int item);
	void OnCurveUpdated(IVDUIParameterCurveControl *source, const int& args);
	void OnCurveStatusUpdated(IVDUIParameterCurveControl *source, const IVDUIParameterCurveControl::Status& status);
	void OnAudioDisplayUpdateRequired(IVDUIAudioDisplayControl *source, const VDPosition& pos);
	void OnAudioDisplaySetSelect(IVDUIAudioDisplayControl *source, const VDUIAudioDisplaySelectionRange& pos);
	void OnAudioDisplayTrackAudioOffset(IVDUIAudioDisplayControl *source, const sint32& offset);
	void OnAudioDisplaySetAudioOffset(IVDUIAudioDisplayControl *source, const sint32& offset);

	LRESULT (VDProjectUI::*mpWndProc)(UINT, WPARAM, LPARAM);

	HWND		mhwndPosition;
	vdrefptr<IVDPositionControl> mpPosition;
	HWND		mhwndStatus;
	HWND		mhwndInputFrame;
	HWND		mhwndOutputFrame;
	HWND		mhwndInputDisplay;
	HWND		mhwndOutputDisplay;
	IVDVideoDisplay	*mpInputDisplay;
	IVDVideoDisplay	*mpOutputDisplay;

	vdrefptr<IVDUIParameterCurveControl> mpCurveEditor;
	HWND		mhwndCurveEditor;

	vdrefptr<IVDUIAudioDisplayControl> mpAudioDisplay;
	HWND		mhwndAudioDisplay;
	VDPosition	mAudioDisplayPosNext;
	bool		mbAudioDisplayReadActive;

	HMENU		mhMenuNormal;
	HMENU		mhMenuDub;
	HMENU		mhMenuDisplay;
	int			mMRUListPosition;
	HACCEL		mhAccelDub;
	HACCEL		mhAccelMain;

	RECT		mrInputFrame;
	RECT		mrOutputFrame;
	bool		mbInputFrameValid;
	bool		mbOutputFrameValid;

	WNDPROC		mOldWndProc;
	bool		mbDubActive;
	bool		mbPositionControlVisible;
	bool		mbStatusBarVisible;

	MRUList		mMRUList;

	vdrefptr<IVDUIWindow>	mpUIPeer;
	vdrefptr<IVDUIWindow>	mpUIBase;
	vdrefptr<IVDUIWindow>	mpUIPaneSet;
	vdrefptr<IVDUIWindow>	mpUISplitSet;
	vdrefptr<IVDUIWindow>	mpUICurveSet;
	vdrefptr<IVDUIWindow>	mpUICurveSplitBar;
	vdrefptr<IVDUIWindow>	mpUICurveEditor;
	vdrefptr<IVDUIWindow>	mpUIInputFrame;
	vdrefptr<IVDUIWindow>	mpUIOutputFrame;
	vdrefptr<IVDUIWindow>	mpUICurveComboBox;

	vdrefptr<IVDUIWindow>	mpUIAudioSplitBar;
	vdrefptr<IVDUIWindow>	mpUIAudioDisplay;

	VDDelegate mCurveUpdatedDelegate;
	VDDelegate mCurveStatusUpdatedDelegate;
	VDDelegate mAudioDisplayUpdateRequiredDelegate;
	VDDelegate mAudioDisplaySetSelectStartDelegate;
	VDDelegate mAudioDisplaySetSelectTrackDelegate;
	VDDelegate mAudioDisplaySetSelectEndDelegate;
	VDDelegate mAudioDisplayTrackAudioOffsetDelegate;
	VDDelegate mAudioDisplaySetAudioOffsetDelegate;
};

#endif
