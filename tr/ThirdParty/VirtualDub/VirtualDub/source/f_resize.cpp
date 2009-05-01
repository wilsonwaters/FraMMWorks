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
#include <commctrl.h>
#include <hash_map>

#include <vd2/system/vdtypes.h>

#include "ScriptInterpreter.h"
#include "ScriptValue.h"
#include "ScriptError.h"

#include "misc.h"
#include <vd2/system/cpuaccel.h>
#include <vd2/Kasumi/pixmap.h>
#include <vd2/Kasumi/pixmaputils.h>
#include <vd2/Kasumi/resample.h>
#include "resource.h"
#include "gui.h"
#include "filter.h"
#include "vbitmap.h"

extern HINSTANCE g_hInst;
extern const char g_szError[];

///////////////////////////////////////////////////////////////////////////////

#include <vd2/system/registry.h>
#include <vd2/system/w32assist.h>

class VDDataExchangeRegistry {
public:
	VDDataExchangeRegistry(const char *path, bool write);
	~VDDataExchangeRegistry();

	bool WasChangeDetected() const { return mbChangeDetected; }

	void Exchange(const char *name, sint32& value);
	void Exchange(const char *name, uint32& value);
	void Exchange(const char *name, bool& value);
	void Exchange(const char *name, double& value);

protected:
	bool mbWrite;
	bool mbChangeDetected;
	VDRegistryAppKey mKey;
};

VDDataExchangeRegistry::VDDataExchangeRegistry(const char *path, bool write)
	: mbWrite(write)
	, mbChangeDetected(false)
	, mKey(path)
{
}

VDDataExchangeRegistry::~VDDataExchangeRegistry()
{
}

void VDDataExchangeRegistry::Exchange(const char *name, sint32& value) {
	if (mbWrite) {
		mKey.setInt(name, (int)value);
	} else {
		sint32 newValue = (sint32)mKey.getInt(name, (int)value);
		if (newValue != value) {
			value = newValue;
			mbChangeDetected = true;
		}
	}
}

void VDDataExchangeRegistry::Exchange(const char *name, uint32& value) {
	if (mbWrite) {
		mKey.setInt(name, (uint32)value);
	} else {
		uint32 newValue = (uint32)mKey.getInt(name, (int)value);
		if (newValue != value) {
			value = newValue;
			mbChangeDetected = true;
		}
	}
}

void VDDataExchangeRegistry::Exchange(const char *name, bool& value) {
	if (mbWrite) {
		mKey.setInt(name, (uint32)value);
	} else {
		bool newValue = 0 != mKey.getInt(name, (int)value);
		if (newValue != value) {
			value = newValue;
			mbChangeDetected = true;
		}
	}
}

void VDDataExchangeRegistry::Exchange(const char *name, double& value) {
	if (mbWrite) {
		mKey.setString(name, VDswprintf(L"%g", 1, &value).c_str());
	} else {
		VDStringW s;

		if (mKey.getString(name, s)) {
			double newValue;
			char dummy;

			if (1 == swscanf(s.c_str(), L" %lg%C", &newValue, &dummy)) {
				if (newValue != value) {
					value = newValue;
					mbChangeDetected = true;
				}
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

class VDDataExchangeDialogW32 {
public:
	VDDataExchangeDialogW32(HWND hdlg, bool write);
	~VDDataExchangeDialogW32();

	bool WasChangeDetected() const { return mbChangeDetected; }
	uint32 GetFirstErrorPos() const { return mErrorPos; }

	void ExchangeEdit(uint32 id, sint32& value);
	void ExchangeEdit(uint32 id, uint32& value);
	void ExchangeEdit(uint32 id, double& value);
	void ExchangeOption(uint32 id, bool& value, bool optionValue);
	void ExchangeOption(uint32 id, uint32& value, uint32 optionValue);
	void ExchangeCombo(uint32 id, uint32& value);
	void ExchangeButton(uint32 id, bool& value);

protected:
	bool mbWrite;
	bool mbChangeDetected;
	uint32 mErrorPos;
	HWND mhdlg;
};

VDDataExchangeDialogW32::VDDataExchangeDialogW32(HWND hdlg, bool write)
	: mbWrite(write)
	, mbChangeDetected(false)
	, mErrorPos(0)
	, mhdlg(hdlg)
{
}

VDDataExchangeDialogW32::~VDDataExchangeDialogW32()
{
}

void VDDataExchangeDialogW32::ExchangeEdit(uint32 id, sint32& value) {
	HWND hwnd = GetDlgItem(mhdlg, id);
	if (!hwnd) {
		if (!mErrorPos)
			mErrorPos = id;
		return;
	}

	if (mbWrite) {
		VDSetWindowTextW32(hwnd, VDswprintf(L"%d", 1, &value).c_str());
	} else {
		VDStringW s(VDGetWindowTextW32(hwnd));
		int value;
		char dummy;

		if (1 == swscanf(s.c_str(), L" %d%C", &value, &dummy)) {
			value = (sint32)value;
		} else {
			if (!mErrorPos && !(GetWindowLong(hwnd, GWL_STYLE) & WS_DISABLED))
				mErrorPos = id;
		}
	}
}

void VDDataExchangeDialogW32::ExchangeEdit(uint32 id, uint32& value) {
	HWND hwnd = GetDlgItem(mhdlg, id);
	if (!hwnd) {
		if (!mErrorPos)
			mErrorPos = id;
		return;
	}

	if (mbWrite) {
		VDSetWindowTextW32(hwnd, VDswprintf(L"%u", 1, &value).c_str());
	} else {
		VDStringW s(VDGetWindowTextW32(hwnd));
		unsigned newValue;
		char dummy;

		if (1 == swscanf(s.c_str(), L" %u%C", &newValue, &dummy)) {
			if (newValue != value) {
				value = (uint32)newValue;
				mbChangeDetected = true;
			}
		} else {
			if (!mErrorPos && !(GetWindowLong(hwnd, GWL_STYLE) & WS_DISABLED))
				mErrorPos = id;
		}
	}
}

void VDDataExchangeDialogW32::ExchangeEdit(uint32 id, double& value) {
	HWND hwnd = GetDlgItem(mhdlg, id);
	if (!hwnd) {
		if (!mErrorPos)
			mErrorPos = id;
		return;
	}

	if (mbWrite) {
		VDSetWindowTextW32(hwnd, VDswprintf(L"%g", 1, &value).c_str());
	} else {
		VDStringW s(VDGetWindowTextW32(hwnd));
		double newValue;
		char dummy;

		if (1 == swscanf(s.c_str(), L" %lg%C", &newValue, &dummy)) {
			if (newValue != value) {
				value = newValue;
				mbChangeDetected = true;
			}
		} else {
			if (!mErrorPos && !(GetWindowLong(hwnd, GWL_STYLE) & WS_DISABLED))
				mErrorPos = id;
		}
	}
}

void VDDataExchangeDialogW32::ExchangeOption(uint32 id, bool& value, bool optionValue) {
	uint32 v = value;

	ExchangeOption(id, v, optionValue);

	value = (v != 0);
}

void VDDataExchangeDialogW32::ExchangeOption(uint32 id, uint32& value, uint32 optionValue) {
	HWND hwnd = GetDlgItem(mhdlg, id);
	if (!hwnd) {
		if (!mErrorPos)
			mErrorPos = id;
		return;
	}

	if (mbWrite) {
		SendMessage(hwnd, BM_SETCHECK, (value == optionValue) ? BST_CHECKED : BST_UNCHECKED, 0);
	} else {
		if (value != optionValue) {
			if (SendMessage(hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED) {
				mbChangeDetected = true;
				value = optionValue;
			}
		}
	}
}

void VDDataExchangeDialogW32::ExchangeCombo(uint32 id, uint32& value) {
	HWND hwnd = GetDlgItem(mhdlg, id);
	if (!hwnd) {
		if (!mErrorPos)
			mErrorPos = id;
		return;
	}

	if (mbWrite) {
		SendMessage(hwnd, CB_SETCURSEL, value, 0);
	} else {
		int result = (int)SendMessage(hwnd, CB_GETCURSEL, 0, 0);

		if (result != CB_ERR && (uint32)result != value) {
			mbChangeDetected = true;
			value = (uint32)result;
		}
	}
}

void VDDataExchangeDialogW32::ExchangeButton(uint32 id, bool& value) {
	HWND hwnd = GetDlgItem(mhdlg, id);
	if (!hwnd) {
		if (!mErrorPos)
			mErrorPos = id;
		return;
	}

	if (mbWrite) {
		SendMessage(hwnd, BM_SETCHECK, value ? BST_CHECKED : BST_UNCHECKED, 0);
	} else {
		bool result = (SendMessage(hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED);

		if (result != value) {
			mbChangeDetected = true;
			value = result;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

enum {
	FILTER_NONE				= 0,
	FILTER_BILINEAR			= 1,
	FILTER_BICUBIC			= 2,
	FILTER_TABLEBILINEAR	= 3,
	FILTER_TABLEBICUBIC075	= 4,
	FILTER_TABLEBICUBIC060	= 5,
	FILTER_TABLEBICUBIC100	= 6,
	FILTER_LANCZOS3			= 7
};

static char *filter_names[]={
	"Nearest neighbor",
	"Bilinear (interpolation only)",
	"Bicubic (interpolation only)",
	"Precise bilinear",
	"Precise bicubic (A=-0.75)",
	"Precise bicubic (A=-0.60)",
	"Precise bicubic (A=-1.00)",
	"Lanczos3"
};

struct VDResizeFilterData {
	enum {
		kFrameModeNone,
		kFrameModeToSize,
		kFrameModeARCrop,
		kFrameModeARLetterbox,
		kFrameModeCount
	};

	enum {
		kImageAspectNone,
		kImageAspectUseSource,
		kImageAspectCustom,
		kImageAspectModeCount
	};

	double	mImageW;
	double	mImageH;
	double	mImageRelW;
	double	mImageRelH;
	bool	mbUseRelative;

	double	mImageAspectNumerator;
	double	mImageAspectDenominator;
	uint32	mImageAspectMode;
	uint32	mFrameW;
	uint32	mFrameH;
	double	mFrameAspectNumerator;
	double	mFrameAspectDenominator;
	uint32	mFrameMode;
	uint32	mFilterMode;
	uint32	mAlignment;
	COLORREF	rgbColor;

	IVDPixmapResampler *resampler;

	bool	mbInterlaced;

	VDResizeFilterData()
		: mImageW(320)
		, mImageH(240)
		, mImageRelW(100)
		, mImageRelH(100)
		, mbUseRelative(true)
		, mImageAspectNumerator(4.0f)
		, mImageAspectDenominator(3.0f)
		, mImageAspectMode(kImageAspectUseSource)
		, mFrameW(320)
		, mFrameH(240)
		, mFrameAspectNumerator(4.0f)
		, mFrameAspectDenominator(3.0f)
		, mFrameMode(kFrameModeNone)
		, mFilterMode(FILTER_TABLEBICUBIC075)
		, mAlignment(1)
		, rgbColor(0)
		, mbInterlaced(false)
	{
	}

	const char *Validate() const {
		if (mbUseRelative) {
			if (!(mImageW > 0 && mImageW <= 1048576))
				return "The target image width is invalid (not within 1-1048576).";
		} else {
			if (!(mImageRelW > 0.0 && mImageRelW <= 1000000.0))
				return "The target image width is invalid (not within 1-1000000%).";
		}

		switch(mImageAspectMode) {
		case kImageAspectNone:
			if (mbUseRelative) {
				if (!(mImageRelH > 0.0 && mImageRelH <= 1000000.0))
					return "The target image height is invalid (not within 0-1000000%).";
			} else {
				if (!(mImageH > 0 && mImageH <= 1048576))
					return "The target image height is invalid (not within 1-1048576).";
			}
			break;

		case kImageAspectUseSource:
			break;

		case kImageAspectCustom:
			if (!(mImageAspectDenominator >= 0.001 && mImageAspectDenominator < 1000.0)
				|| !(mImageAspectNumerator >= 0.001 && mImageAspectNumerator < 1000.0))
				return "The target image aspect ratio is invalid (values must be within 0.001 to 1000.0).";
			break;

		default:
			return "The target image size mode is invalid.";
		}

		switch(mFrameMode) {
			case kFrameModeNone:
				break;

			case kFrameModeToSize:
				if (!(mFrameW >= 0 && mFrameW < 1048576) || !(mFrameH >= 0 && mFrameH < 1048576))
					return "The target frame size is invalid.";
				break;

			case kFrameModeARCrop:
			case kFrameModeARLetterbox:
				if (!(mFrameAspectDenominator >= 0.001 && mFrameAspectDenominator < 1000.0)
					|| !(mFrameAspectNumerator >= 0.001 && mFrameAspectNumerator < 1000.0))
					return "The target image aspect ratio is invalid (values must be within 0.1 to 10.0).";
				break;

			default:
				return "The target image size mode is invalid.";
		}

		return NULL;
	}

	void ComputeSizes(uint32 srcw, uint32 srch, double& imgw, double& imgh, uint32& framew, uint32& frameh, bool useAlignment, bool widthHasPriority) {
		if (mbUseRelative) {
			imgw = srcw * (mImageRelW * (1.0 / 100.0));
			imgh = srch * (mImageRelH * (1.0 / 100.0));
		} else {
			imgw = mImageW;
			imgh = mImageH;
		}

		switch(mImageAspectMode) {
			case kImageAspectNone:
				break;

			case kImageAspectUseSource:
				if (widthHasPriority)
					imgh = imgw * ((double)srch / (double)srcw);
				else
					imgw = imgh * ((double)srcw / (double)srch);
				break;

			case kImageAspectCustom:
				if (widthHasPriority)
					imgh = imgw * (mImageAspectDenominator / mImageAspectNumerator);
				else
					imgw = imgh * (mImageAspectNumerator / mImageAspectDenominator);
				break;
		}

		double framewf;
		double framehf;

		switch(mFrameMode) {
			case kFrameModeNone:
				framewf = imgw;
				framehf = imgh;
				break;

			case kFrameModeToSize:
				framewf = mFrameW;
				framehf = mFrameH;
				break;

			case kFrameModeARCrop:
				framewf = imgw;
				framehf = imgw * (mFrameAspectDenominator / mFrameAspectNumerator);
				if (framehf > imgh) {
					framewf = imgw * (mFrameAspectNumerator / mFrameAspectDenominator);
					framehf = imgh;
				}
				break;
			case kFrameModeARLetterbox:
				framewf = imgw;
				framehf = imgw * (mFrameAspectDenominator / mFrameAspectNumerator);
				if (framehf < imgh) {
					framewf = imgw * (mFrameAspectNumerator / mFrameAspectDenominator);
					framehf = imgh;
				}
				break;
		}

		framew = (uint32)VDRoundToInt(framewf);
		frameh = (uint32)VDRoundToInt(framehf);

		if (framew < 1)
			framew = 1;
		if (frameh < 1)
			frameh = 1;

		// if alignment is present, round frame sizes down to a multiple of alignment.
		if (mAlignment > 1 && useAlignment) {
			framew -= framew % mAlignment;
			frameh -= frameh % mAlignment;

			if (!framew)
				framew = mAlignment;

			if (!frameh)
				frameh = mAlignment;

			// Detect if letterboxing is occurring. If so, we should enlarge the image
			// to fit alignment requirements. We have to enlarge because it will often
			// be impossible to meet alignment requirements in both axes if we shrink.

			if (imgw > 1e-5 && imgh > 1e-5) {
				if (imgw < framew) {
					if (imgh < frameh) {			// Bars all around.

						// If we have bars on all sides, we're basically screwed
						// as there is no way we can expand the image and maintain
						// aspect ratio. So we do nothing.

					} else {						// Bars on left and right only -- expand to horizontal alignment.

						double imginvaspect = imgh / imgw;

						uint32 tmpw = ((uint32)VDRoundToInt(imgw) + mAlignment - 1);
						tmpw -= tmpw % mAlignment;

						imgw = tmpw;
						imgh = imgw * imginvaspect;

					}
				} else if (imgh < frameh) {			// Bars on top and bottom only -- expand to vertical alignment.

						double imgaspect = imgw / imgh;

						uint32 tmph = ((uint32)VDRoundToInt(imgh) + mAlignment - 1);
						tmph -= tmph % mAlignment;

						imgh = tmph;
						imgw = imgh * imgaspect;

				}
			}
		}
	}

	void UpdateInformativeFields(uint32 srcw, uint32 srch, bool widthHasPriority) {
		double imgw, imgh;
		uint32 framew, frameh;
		ComputeSizes(srcw, srch, imgw, imgh, framew, frameh, false, widthHasPriority);

		if (mbUseRelative) {
			mImageW = imgw;
			mImageH = imgh;
			if (mImageAspectMode != VDResizeFilterData::kImageAspectNone) {
				if (widthHasPriority)
					mImageRelH = 100.0 * imgh / (double)srch;
				else
					mImageRelW = 100.0 * imgw / (double)srcw;
			}
		} else {
			mImageRelW = 100.0 * imgw / (double)srcw;
			mImageRelH = 100.0 * imgh / (double)srch;
			if (mImageAspectMode != VDResizeFilterData::kImageAspectNone) {
				if (widthHasPriority)
					mImageH = imgh;
				else
					mImageW = imgw;
			}
		}
	}

	bool ExchangeWithRegistry(bool write) {
		VDDataExchangeRegistry ex("Video filters\\resize", write);

		ex.Exchange("Image width", mImageW);
		ex.Exchange("Image height", mImageH);
		ex.Exchange("Image relative width", mImageRelW);
		ex.Exchange("Image relative height", mImageRelH);
		ex.Exchange("Use relative image size", mbUseRelative);
		ex.Exchange("Image aspect N", mImageAspectNumerator);
		ex.Exchange("Image aspect D", mImageAspectDenominator);
		ex.Exchange("Frame width", mFrameW);
		ex.Exchange("Frame height", mFrameH);
		ex.Exchange("Frame aspect N", mFrameAspectNumerator);
		ex.Exchange("Frame aspect D", mFrameAspectDenominator);
		ex.Exchange("Filter mode", mFilterMode);
		ex.Exchange("Interlaced", mbInterlaced);
		ex.Exchange("Image aspect mode", mImageAspectMode);
		ex.Exchange("Frame mode", mFrameMode);
		ex.Exchange("Codec-friendly alignment", mAlignment);

		return ex.WasChangeDetected();
	}
};

////////////////////

int revcolor(int c) {
	return ((c>>16)&0xff) | (c&0xff00) | ((c&0xff)<<16);
}

////////////////////

static int resize_init(FilterActivation *fa, const FilterFunctions *ff) {
	VDResizeFilterData *mfd = (VDResizeFilterData *)fa->filter_data;

	new(mfd) VDResizeFilterData();

	mfd->ExchangeWithRegistry(false);

	return 0;
}

static int resize_run(const FilterActivation *fa, const FilterFunctions *ff) {
	VDResizeFilterData *mfd = (VDResizeFilterData *)fa->filter_data;
	Pixel *dst, *src;

	dst = fa->dst.data;
	src = fa->src.data;

	double dstw, dsth;
	uint32 framew, frameh;
	mfd->ComputeSizes(fa->src.w, fa->src.h, dstw, dsth, framew, frameh, true, true);

	double dx = (fa->dst.w - dstw) * 0.5;
	double dy = (fa->dst.h - dsth) * 0.5;

	int x1 = std::max<int>(0, (int)ceil(dx - 0.5));
	int y1 = std::max<int>(0, (int)ceil(dy - 0.5));

	if (mfd->mAlignment > 1) {
		x1 -= x1 % mfd->mAlignment;
		y1 -= y1 % mfd->mAlignment;
		dx = x1;
		dy = y1;
	}

	int x2 = std::min<int>(fa->dst.w, (int)ceil(dx + dstw - 0.5));
	int y2 = std::min<int>(fa->dst.h, (int)ceil(dy + dsth - 0.5));

	// Draw letterbox bound

	if (x1 > 0 || y1 > 0 || x2 < fa->dst.w || y2 < fa->dst.h) {
		Pixel fill = revcolor(mfd->rgbColor);

		char *dstp = (char *)dst;
		ptrdiff_t dstpitch = fa->dst.pitch;

		// fill bottom
		if (y2 < fa->dst.h)
			VDMemset32Rect(dstp, dstpitch, fill, fa->dst.w, fa->dst.h - y2);


		// fill left/right
		if (y2 > y1) {
			char *dst2 = dstp + dstpitch * (fa->dst.h - y2);

			for(int y = y1; y < y2; ++y) {
				// fill left
				if (x1 > 0)
					VDMemset32(dst2, fill, x1);

				// fill right
				if (x2 < fa->dst.w)
					VDMemset32(dst2 + x2*4, fill, fa->dst.w - x2);

				dst2 += dstpitch;
			}
		}

		// fill bottom
		if (y1 > 0)
			VDMemset32Rect(dstp + dstpitch * (fa->dst.h - y1), dstpitch, fill, fa->dst.w, y1);
	}

	if (mfd->mbInterlaced) {
		VDPixmap vbHalfSrc, vbHalfDst;

		// Top field

		vbHalfSrc		= VDAsPixmap((VBitmap&)fa->src);
		vbHalfSrc.pitch *= 2;
		vbHalfSrc.h		= (fa->src.h + 1) >> 1;

		vbHalfDst		= VDAsPixmap((VBitmap&)fa->dst);
		vbHalfDst.pitch *= 2;
		vbHalfDst.h		= (fa->dst.h + 1) >> 1;

		dy = dy * 0.5 + 0.25;

		mfd->resampler->Process(vbHalfDst, dx, dy, dx + dstw, dy + dsth*0.5, vbHalfSrc, 0, 0.25);

		// Bottom field

		vdptrstep(vbHalfSrc.data, -fa->src.pitch);
		vbHalfSrc.h		= fa->src.h >> 1;
		vdptrstep(vbHalfDst.data, -fa->dst.pitch);
		vbHalfDst.h		= fa->dst.h >> 1;

		dy -= 0.5;
		mfd->resampler->Process(vbHalfDst, dx, dy, dx + dstw, dy + dsth*0.5, vbHalfSrc, 0, -0.25);
	} else {
		VDPixmap pxdst(VDAsPixmap(*(VBitmap *)&fa->dst));
		VDPixmap pxsrc(VDAsPixmap(*(VBitmap *)&fa->src));

		mfd->resampler->Process(pxdst, dx, dy, dx + dstw, dy + dsth, pxsrc, 0, 0);
	}

	return 0;
}

static long resize_param(FilterActivation *fa, const FilterFunctions *ff) {
	VDResizeFilterData *mfd = (VDResizeFilterData *)fa->filter_data;

	if (mfd->Validate()) {
		// uh oh.
		return 0;
	}

	double imgw, imgh;
	uint32 framew, frameh;
	mfd->ComputeSizes(fa->src.w, fa->src.h, imgw, imgh, framew, frameh, true, true);

	fa->dst.w = framew;
	fa->dst.h = frameh;

	fa->dst.AlignTo8();

	return FILTERPARAM_SWAP_BUFFERS;
}

namespace {
	void VDSetDlgItemFloatW32(HWND hdlg, UINT id, double v) {
		char buf[512];

		sprintf(buf, "%g", v);
		SetDlgItemText(hdlg, id, buf);
	}

	double VDGetDlgItemFloatW32(HWND hdlg, UINT id, BOOL *success) {
		char buf[512];

		*success = FALSE;

		if (GetDlgItemText(hdlg, id, buf, sizeof buf)) {
			double v;
			if (1 == sscanf(buf, " %lg", &v)) {
				*success = TRUE;
				return v;
			}
		}

		return 0;
	}
};

class VDVF1ResizeDlg : public VDDialogBaseW32 {
public:
	VDVF1ResizeDlg(VDResizeFilterData& config, IVDFilterPreview2 *ifp, uint32 w, uint32 h);

protected:
	INT_PTR DlgProc(UINT msg, WPARAM wParam, LPARAM lParam);

	void InitDialog();
	uint32 ExchangeWithDialog(bool write);
	void UpdateDialogAbsoluteSize();
	void UpdateDialogRelativeSize();
	void UpdateEnables();
	void UpdatePreview();
	bool ApplyChanges();
	void MarkDirty();

	VDResizeFilterData& mConfig;
	VDResizeFilterData mNewConfig;
	bool		mbConfigDirty;
	bool		mbWidthPriority;
	HBRUSH		mhbrColor;
	IVDFilterPreview2 *mifp;
	uint32		mWidth;
	uint32		mHeight;
	int			mRecursionLock;
};

VDVF1ResizeDlg::VDVF1ResizeDlg(VDResizeFilterData& config, IVDFilterPreview2 *ifp, uint32 w, uint32 h)
	: VDDialogBaseW32(IDD_FILTER_RESIZE)
	, mConfig(config)
	, mbConfigDirty(false)
	, mbWidthPriority(true)
	, mhbrColor(NULL)
	, mifp(ifp)
	, mWidth(w)
	, mHeight(h)
	, mRecursionLock(0)
{
}

INT_PTR VDVF1ResizeDlg::DlgProc(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG:
			InitDialog();
            return FALSE;

        case WM_COMMAND:                      
			switch(LOWORD(wParam)) {
			case IDC_APPLY:
				ApplyChanges();
				return TRUE;

			case IDC_PREVIEW:
				if (mifp->IsPreviewDisplayed()) {
					EnableWindow(GetDlgItem(mhdlg, IDC_APPLY), FALSE);
				} else {
					if (mbConfigDirty) {
						if (!ApplyChanges())
							return TRUE;
					}
				}
				mifp->Toggle(mhdlg);
				return TRUE;

			case IDOK:
				if (uint32 badid = ExchangeWithDialog(false)) {
					SetFocus(GetDlgItem(mhdlg, badid));
					MessageBeep(MB_ICONERROR);
				} else if (const char *err = mNewConfig.Validate()) {
					MessageBox(mhdlg, err, g_szError, MB_ICONERROR | MB_OK);
				} else {
					mifp->Close();
					mConfig = mNewConfig;
					End(true);
				}
				return TRUE;

			case IDCANCEL:
				mifp->Close();
				End(false);
                return TRUE;

			case IDC_SAVE_AS_DEFAULT:
				if (const char *err = mConfig.Validate())
					MessageBox(mhdlg, err, g_szError, MB_ICONERROR | MB_OK);
				else
					mConfig.ExchangeWithRegistry(true);
				return TRUE;

			case IDC_WIDTH:
			case IDC_HEIGHT:
				if (!mRecursionLock && (HIWORD(wParam) == EN_KILLFOCUS || HIWORD(wParam) == EN_CHANGE)) {
					if (HIWORD(wParam) == EN_CHANGE) {
						mbWidthPriority = (LOWORD(wParam) == IDC_WIDTH);
						++mRecursionLock;
						CheckDlgButton(mhdlg, IDC_SIZE_ABSOLUTE, BST_CHECKED);
						CheckDlgButton(mhdlg, IDC_SIZE_RELATIVE, BST_UNCHECKED);
						--mRecursionLock;
					}
					ExchangeWithDialog(false);
				}
				break;

			case IDC_RELWIDTH:
			case IDC_RELHEIGHT:
				if (!mRecursionLock && (HIWORD(wParam) == EN_KILLFOCUS || HIWORD(wParam) == EN_CHANGE)) {
					if (HIWORD(wParam) == EN_CHANGE) {
						mbWidthPriority = (LOWORD(wParam) == IDC_RELWIDTH);
						++mRecursionLock;
						CheckDlgButton(mhdlg, IDC_SIZE_ABSOLUTE, BST_UNCHECKED);
						CheckDlgButton(mhdlg, IDC_SIZE_RELATIVE, BST_CHECKED);
						--mRecursionLock;
					}
					ExchangeWithDialog(false);
				}
				break;

			case IDC_FRAMEWIDTH:
			case IDC_FRAMEHEIGHT:
			case IDC_ASPECT_RATIO1:
			case IDC_ASPECT_RATIO2:
			case IDC_FRAME_ASPECT1:
			case IDC_FRAME_ASPECT2:
				if (!mRecursionLock && (HIWORD(wParam) == EN_KILLFOCUS || HIWORD(wParam) == EN_CHANGE)) {
					ExchangeWithDialog(false);
				}
				return TRUE;

			case IDC_FILTER_MODE:
				if (!mRecursionLock && HIWORD(wParam) == CBN_SELCHANGE) {
					ExchangeWithDialog(false);
				}
				return TRUE;

			case IDC_SIZE_ABSOLUTE:
			case IDC_SIZE_RELATIVE:
			case IDC_INTERLACED:
			case IDC_AR_NONE:
			case IDC_AR_USE_SOURCE:
			case IDC_AR_USE_RATIO:
			case IDC_FRAME_NONE:
			case IDC_FRAME_TO_SIZE:
			case IDC_FRAME_AR_CROP:
			case IDC_FRAME_AR_LETTERBOX:
			case IDC_LETTERBOX:
			case IDC_ALIGNMENT_1:
			case IDC_ALIGNMENT_4:
			case IDC_ALIGNMENT_8:
			case IDC_ALIGNMENT_16:
				if (!mRecursionLock && HIWORD(wParam) == BN_CLICKED) {
					ExchangeWithDialog(false);
				}
				return TRUE;

			case IDC_PICKCOLOR:
				if (guiChooseColor(mhdlg, mNewConfig.rgbColor)) {
					DeleteObject(mhbrColor);
					mhbrColor = CreateSolidBrush(mNewConfig.rgbColor);
					RedrawWindow(GetDlgItem(mhdlg, IDC_COLOR), NULL, NULL, RDW_ERASE|RDW_INVALIDATE|RDW_UPDATENOW);
					MarkDirty();
				}
				break;
            }
            break;

		case WM_CTLCOLORSTATIC:
			if (GetWindowLong((HWND)lParam, GWL_ID) == IDC_COLOR)
				return (INT_PTR)mhbrColor;
			break;

		case WM_DESTROY:
			if (mhbrColor) {
				DeleteObject(mhbrColor);
				mhbrColor = NULL;
			}

			break;
    }
    return FALSE;
}

void VDVF1ResizeDlg::InitDialog() {
	HWND hwndItem = GetDlgItem(mhdlg, IDC_FILTER_MODE);
	for(int i=0; i<(sizeof filter_names/sizeof filter_names[0]); i++)
		SendMessage(hwndItem, CB_ADDSTRING, 0, (LPARAM)filter_names[i]);

	if (mWidth && mHeight)
		mConfig.UpdateInformativeFields(mWidth, mHeight, true);

	mNewConfig = mConfig;
	ExchangeWithDialog(true);

	mbConfigDirty = false;

	UpdateEnables();

	mhbrColor = CreateSolidBrush(mConfig.rgbColor);
	mifp->InitButton(GetDlgItem(mhdlg, IDC_PREVIEW));

	UINT id = IDC_WIDTH;
	if (mConfig.mbUseRelative)
		id = IDC_RELWIDTH;

	hwndItem = GetDlgItem(mhdlg, id);
	if (hwndItem) {
		SetFocus(hwndItem);
		SendMessage(hwndItem, EM_SETSEL, 0, -1);
	}
}

uint32 VDVF1ResizeDlg::ExchangeWithDialog(bool write) {
	VDDataExchangeDialogW32 ex(mhdlg, write);

	++mRecursionLock;
	ex.ExchangeOption(IDC_SIZE_ABSOLUTE, mNewConfig.mbUseRelative, false);
	ex.ExchangeOption(IDC_SIZE_RELATIVE, mNewConfig.mbUseRelative, true);

	bool doboth = mNewConfig.mImageAspectMode == VDResizeFilterData::kImageAspectNone || write;
	bool dowidth = doboth || mbWidthPriority;
	bool doheight = doboth || !mbWidthPriority;

	if (write || mNewConfig.mbUseRelative) {
		if (dowidth)
			ex.ExchangeEdit(IDC_RELWIDTH, mNewConfig.mImageRelW);
		if (doheight)
			ex.ExchangeEdit(IDC_RELHEIGHT, mNewConfig.mImageRelH);
	}
	
	if (write || !mNewConfig.mbUseRelative) {
		if (dowidth)
			ex.ExchangeEdit(IDC_WIDTH, mNewConfig.mImageW);
		if (doheight)
			ex.ExchangeEdit(IDC_HEIGHT, mNewConfig.mImageH);
	}

	ex.ExchangeEdit(IDC_ASPECT_RATIO1, mNewConfig.mImageAspectNumerator);
	ex.ExchangeEdit(IDC_ASPECT_RATIO2, mNewConfig.mImageAspectDenominator);
	ex.ExchangeEdit(IDC_FRAMEWIDTH, mNewConfig.mFrameW);
	ex.ExchangeEdit(IDC_FRAMEHEIGHT, mNewConfig.mFrameH);
	ex.ExchangeEdit(IDC_FRAME_ASPECT1, mNewConfig.mFrameAspectNumerator);
	ex.ExchangeEdit(IDC_FRAME_ASPECT2, mNewConfig.mFrameAspectDenominator);
	ex.ExchangeCombo(IDC_FILTER_MODE, mNewConfig.mFilterMode);
	ex.ExchangeButton(IDC_INTERLACED, mNewConfig.mbInterlaced);
	ex.ExchangeOption(IDC_AR_NONE, mNewConfig.mImageAspectMode, VDResizeFilterData::kImageAspectNone);
	ex.ExchangeOption(IDC_AR_USE_SOURCE, mNewConfig.mImageAspectMode, VDResizeFilterData::kImageAspectUseSource);
	ex.ExchangeOption(IDC_AR_USE_RATIO, mNewConfig.mImageAspectMode, VDResizeFilterData::kImageAspectCustom);
	ex.ExchangeOption(IDC_FRAME_NONE, mNewConfig.mFrameMode, VDResizeFilterData::kFrameModeNone);
	ex.ExchangeOption(IDC_FRAME_TO_SIZE, mNewConfig.mFrameMode, VDResizeFilterData::kFrameModeToSize);
	ex.ExchangeOption(IDC_FRAME_AR_CROP, mNewConfig.mFrameMode, VDResizeFilterData::kFrameModeARCrop);
	ex.ExchangeOption(IDC_FRAME_AR_LETTERBOX, mNewConfig.mFrameMode, VDResizeFilterData::kFrameModeARLetterbox);
	ex.ExchangeOption(IDC_ALIGNMENT_1, mNewConfig.mAlignment, 1);
	ex.ExchangeOption(IDC_ALIGNMENT_4, mNewConfig.mAlignment, 4);
	ex.ExchangeOption(IDC_ALIGNMENT_8, mNewConfig.mAlignment, 8);
	ex.ExchangeOption(IDC_ALIGNMENT_16, mNewConfig.mAlignment, 16);
	--mRecursionLock;

	uint32 error = ex.GetFirstErrorPos();
	bool changeDetected = ex.WasChangeDetected();

	if (changeDetected) {
		UpdateEnables();

		if (!write && !error && mWidth && mHeight) {
			VDDataExchangeDialogW32 wex(mhdlg, true);

			mNewConfig.UpdateInformativeFields(mWidth, mHeight, mbWidthPriority);

			++mRecursionLock;
			if (mNewConfig.mbUseRelative) {
				wex.ExchangeEdit(IDC_WIDTH, mNewConfig.mImageW);
				wex.ExchangeEdit(IDC_HEIGHT, mNewConfig.mImageH);
				if (mNewConfig.mImageAspectMode != VDResizeFilterData::kImageAspectNone) {
					if (mbWidthPriority)
						wex.ExchangeEdit(IDC_RELHEIGHT, mNewConfig.mImageRelH);
					else
						wex.ExchangeEdit(IDC_RELWIDTH, mNewConfig.mImageRelW);
				}
			} else {
				wex.ExchangeEdit(IDC_RELWIDTH, mNewConfig.mImageRelW);
				wex.ExchangeEdit(IDC_RELHEIGHT, mNewConfig.mImageRelH);
				if (mNewConfig.mImageAspectMode != VDResizeFilterData::kImageAspectNone) {
					if (mbWidthPriority)
						wex.ExchangeEdit(IDC_HEIGHT, mNewConfig.mImageH);
					else
						wex.ExchangeEdit(IDC_WIDTH, mNewConfig.mImageW);
				}
			}
			--mRecursionLock;
		}
	}

	if (changeDetected || error) {
		MarkDirty();
	}

	return error;
}

void VDVF1ResizeDlg::UpdateEnables() {
	EnableWindow(GetDlgItem(mhdlg, IDC_ASPECT_RATIO1), mNewConfig.mImageAspectMode == VDResizeFilterData::kImageAspectCustom);
	EnableWindow(GetDlgItem(mhdlg, IDC_ASPECT_RATIO2), mNewConfig.mImageAspectMode == VDResizeFilterData::kImageAspectCustom);
	EnableWindow(GetDlgItem(mhdlg, IDC_FRAMEWIDTH), mNewConfig.mFrameMode == VDResizeFilterData::kFrameModeToSize);
	EnableWindow(GetDlgItem(mhdlg, IDC_FRAMEHEIGHT), mNewConfig.mFrameMode == VDResizeFilterData::kFrameModeToSize);

	bool frameToAspect = (mNewConfig.mFrameMode == VDResizeFilterData::kFrameModeARCrop) || (mNewConfig.mFrameMode == VDResizeFilterData::kFrameModeARLetterbox);
	EnableWindow(GetDlgItem(mhdlg, IDC_FRAME_ASPECT1), frameToAspect);
	EnableWindow(GetDlgItem(mhdlg, IDC_FRAME_ASPECT2), frameToAspect);

	bool letterbox = (mNewConfig.mFrameMode == VDResizeFilterData::kFrameModeToSize) || (mNewConfig.mFrameMode == VDResizeFilterData::kFrameModeARLetterbox);
	EnableWindow(GetDlgItem(mhdlg, IDC_STATIC_FILLCOLOR), letterbox);
	EnableWindow(GetDlgItem(mhdlg, IDC_COLOR), letterbox);
	EnableWindow(GetDlgItem(mhdlg, IDC_PICKCOLOR), letterbox);
}

void VDVF1ResizeDlg::UpdatePreview() {
	mifp->RedoSystem();
}

bool VDVF1ResizeDlg::ApplyChanges() {
	if (!mbConfigDirty)
		return false;

	if (uint32 badid = ExchangeWithDialog(false)) {
		SetFocus(GetDlgItem(mhdlg, badid));
		MessageBeep(MB_ICONERROR);
		return false;
	}

	mbConfigDirty = false;
	mifp->UndoSystem();

	mConfig = mNewConfig;
	UpdatePreview();
	EnableWindow(GetDlgItem(mhdlg, IDC_APPLY), FALSE);
	return true;
}

void VDVF1ResizeDlg::MarkDirty() {
	if (!mbConfigDirty) {
		mbConfigDirty = true;

		if (mifp->IsPreviewDisplayed())
			EnableWindow(GetDlgItem(mhdlg, IDC_APPLY), TRUE);
	}
}

static int resize_config(FilterActivation *fa, const FilterFunctions *ff, HWND hwnd) {
	VDResizeFilterData *mfd = (VDResizeFilterData *)fa->filter_data;
	VDResizeFilterData mfd2 = *mfd;

	VDVF1ResizeDlg dlg(*mfd, fa->ifp2, fa->src.w, fa->src.h);

	if (!dlg.ActivateDialogDual((VDGUIHandle)hwnd)) {
		*mfd = mfd2;
		return true;
	}

	return false;
}

static void resize_string(const FilterActivation *fa, const FilterFunctions *ff, char *buf) {
	VDResizeFilterData *mfd = (VDResizeFilterData *)fa->filter_data;

	sprintf(buf, " (%s)", filter_names[mfd->mFilterMode]);
}

static int resize_start(FilterActivation *fa, const FilterFunctions *ff) {
	VDResizeFilterData *mfd = (VDResizeFilterData *)fa->filter_data;

	if (const char *error = mfd->Validate())
		ff->Except("%s", error);

	double dstw, dsth;
	uint32 framew, frameh;
	mfd->ComputeSizes(fa->src.w, fa->src.h, dstw, dsth, framew, frameh, true, true);

	IVDPixmapResampler::FilterMode fmode;
	bool bInterpolationOnly = true;

	mfd->resampler = VDCreatePixmapResampler();

	switch(mfd->mFilterMode) {
	case FILTER_NONE:
		fmode = IVDPixmapResampler::kFilterPoint;
		break;
	case FILTER_TABLEBILINEAR:
		bInterpolationOnly = false;
	case FILTER_BILINEAR:
		fmode = IVDPixmapResampler::kFilterLinear;
		break;
	case FILTER_TABLEBICUBIC060:
		mfd->resampler->SetSplineFactor(-0.60);
		fmode = IVDPixmapResampler::kFilterCubic;
		bInterpolationOnly = false;
		break;
	case FILTER_TABLEBICUBIC075:
		bInterpolationOnly = false;
	case FILTER_BICUBIC:
		mfd->resampler->SetSplineFactor(-0.75);
		fmode = IVDPixmapResampler::kFilterCubic;
		break;
	case FILTER_TABLEBICUBIC100:
		bInterpolationOnly = false;
		mfd->resampler->SetSplineFactor(-1.0);
		fmode = IVDPixmapResampler::kFilterCubic;
		break;
	case FILTER_LANCZOS3:
		bInterpolationOnly = false;
		fmode = IVDPixmapResampler::kFilterLanczos3;
		break;
	}

	if (mfd->mbInterlaced)
		mfd->resampler->Init(dstw, dsth * 0.5f, nsVDPixmap::kPixFormat_XRGB8888, fa->src.w, fa->src.h * 0.5f, nsVDPixmap::kPixFormat_XRGB8888, fmode, fmode, bInterpolationOnly);
	else
		mfd->resampler->Init(dstw, dsth, nsVDPixmap::kPixFormat_XRGB8888, fa->src.w, fa->src.h, nsVDPixmap::kPixFormat_XRGB8888, fmode, fmode, bInterpolationOnly);

	return 0;
}

static int resize_stop(FilterActivation *fa, const FilterFunctions *ff) {
	VDResizeFilterData *mfd = (VDResizeFilterData *)fa->filter_data;

	delete mfd->resampler;	mfd->resampler = NULL;

	return 0;
}

static void resize_script_config(IScriptInterpreter *isi, void *lpVoid, CScriptValue *argv, int argc) {
	FilterActivation *fa = (FilterActivation *)lpVoid;

	VDResizeFilterData *mfd = (VDResizeFilterData *)fa->filter_data;

	mfd->mImageW	= argv[0].asDouble();
	mfd->mImageH	= argv[1].asDouble();
	mfd->mbUseRelative = false;
	mfd->mImageAspectMode = VDResizeFilterData::kImageAspectNone;

	if (argv[2].isInt())
		mfd->mFilterMode = argv[2].asInt();
	else {
		char *s = *argv[2].asString();

		if (!_stricmp(s, "point") || !_stricmp(s, "nearest"))
			mfd->mFilterMode = FILTER_NONE;
		else if (!_stricmp(s, "bilinear"))
			mfd->mFilterMode = FILTER_BILINEAR;
		else if (!_stricmp(s, "bicubic"))
			mfd->mFilterMode = FILTER_BICUBIC;
		else
			VDSCRIPT_EXT_ERROR(FCALL_UNKNOWN_STR);
	}

	mfd->mbInterlaced = false;

	if (mfd->mFilterMode & 128) {
		mfd->mbInterlaced = true;
		mfd->mFilterMode &= 127;
	}

	mfd->mFrameMode = VDResizeFilterData::kFrameModeNone;
	if (argc > 3) {
		mfd->mFrameMode = VDResizeFilterData::kFrameModeToSize;
		mfd->mFrameW = argv[3].asInt();
		mfd->mFrameH = argv[4].asInt();
		mfd->rgbColor = revcolor(argv[5].asInt());
	}

	// make the sizes somewhat sane
	if (mfd->mImageW < 1.0f)
		mfd->mImageW = 1.0f;
	if (mfd->mImageH < 1.0f)
		mfd->mImageH = 1.0f;

	if (mfd->mFrameMode) {
		if (mfd->mFrameW < mfd->mImageW)
			mfd->mFrameW = (int)ceil(mfd->mImageW);

		if (mfd->mFrameH < mfd->mImageH)
			mfd->mFrameH = (int)ceil(mfd->mImageH);
	}
}

static void resize_script_config2(IScriptInterpreter *isi, void *lpVoid, CScriptValue *argv, int argc) {
	FilterActivation *fa = (FilterActivation *)lpVoid;

	VDResizeFilterData *mfd = (VDResizeFilterData *)fa->filter_data;

	mfd->mbUseRelative	= (argv[2].asInt() & 1) != 0;

	if (mfd->mbUseRelative) {
		mfd->mImageRelW	= argv[0].asDouble();
		mfd->mImageRelH	= argv[1].asDouble();
	} else {
		mfd->mImageW	= argv[0].asDouble();
		mfd->mImageH	= argv[1].asDouble();
	}

	mfd->mImageAspectNumerator		= argv[3].asDouble();
	mfd->mImageAspectDenominator	= argv[4].asDouble();
	mfd->mImageAspectMode			= argv[5].asInt();
	if (mfd->mImageAspectMode >= VDResizeFilterData::kImageAspectModeCount)
		mfd->mImageAspectMode = 0;

	mfd->mFrameW = argv[6].asInt();
	mfd->mFrameH = argv[7].asInt();

	mfd->mFrameAspectNumerator		= argv[8].asDouble();
	mfd->mFrameAspectDenominator	= argv[9].asDouble();
	mfd->mFrameMode			= argv[10].asInt();
	if (mfd->mFrameMode >= VDResizeFilterData::kFrameModeCount)
		mfd->mFrameMode = 0;

	mfd->mFilterMode = argv[11].asInt();
	mfd->mbInterlaced = false;

	if (mfd->mFilterMode & 128) {
		mfd->mbInterlaced = true;
		mfd->mFilterMode &= 127;
	}

	mfd->mAlignment = argv[12].asInt();
	switch(mfd->mAlignment) {
		case 1:
		case 4:
		case 8:
		case 16:
			break;
		default:
			mfd->mAlignment = 1;
			break;
	}

	mfd->rgbColor = revcolor(argv[13].asInt());
}

static ScriptFunctionDef resize_func_defs[]={
	{ (ScriptFunctionPtr)resize_script_config, "Config", "0ddi" },
	{ (ScriptFunctionPtr)resize_script_config, NULL, "0dds" },
	{ (ScriptFunctionPtr)resize_script_config, NULL, "0ddiiii" },
	{ (ScriptFunctionPtr)resize_script_config, NULL, "0ddsiii" },
	{ (ScriptFunctionPtr)resize_script_config2, NULL, "0ddiddiiiddiiii" },
	{ NULL },
};

static CScriptObject resize_obj={
	NULL, resize_func_defs
};

static bool resize_script_line(FilterActivation *fa, const FilterFunctions *ff, char *buf, int buflen) {
	VDResizeFilterData *mfd = (VDResizeFilterData *)fa->filter_data;
	int filtmode = mfd->mFilterMode + (mfd->mbInterlaced ? 128 : 0);

	_snprintf(buf, buflen, "Config(%g,%g,%d,%g,%g,%d,%d,%d,%g,%g,%d,%d,%d,0x%06x)"
		, mfd->mbUseRelative ? mfd->mImageRelW : mfd->mImageW
		, mfd->mbUseRelative ? mfd->mImageRelH : mfd->mImageH
		, mfd->mbUseRelative
		, mfd->mImageAspectNumerator
		, mfd->mImageAspectDenominator
		, mfd->mImageAspectMode
		, mfd->mFrameW
		, mfd->mFrameH
		, mfd->mFrameAspectNumerator
		, mfd->mFrameAspectDenominator
		, mfd->mFrameMode
		, filtmode
		, mfd->mAlignment
		, revcolor(mfd->rgbColor));

	return true;
}

FilterDefinition filterDef_resize={
	0,0,NULL,
	"resize",
	"Resizes the image to a new size."
#ifdef USE_ASM
			"\n\n[Assembly optimized] [FPU optimized] [MMX optimized]"
#endif
			,
	NULL,NULL,
	sizeof(VDResizeFilterData),
	resize_init,
	NULL,
	resize_run,
	resize_param,
	resize_config,
	resize_string,
	resize_start,
	resize_stop,

	&resize_obj,
	resize_script_line,
};
