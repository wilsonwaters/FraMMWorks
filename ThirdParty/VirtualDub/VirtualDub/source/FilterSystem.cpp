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

#include <vd2/system/debug.h>
#include <vd2/system/error.h>
#include <vd2/system/protscope.h>
#include <vd2/Kasumi/pixmapops.h>
#include "VBitmap.h"
#include "crash.h"
#include "misc.h"

#include "filters.h"

#include "FilterSystem.h"

extern FilterFunctions g_filterFuncs;

///////////////////////////////////////////////////////////////////////////

class FilterSystemBitmap : public VFBitmapInternal {
public:
	int				buffer;
	LONG			lMapOffset;
};

///////////////////////////////////////////////////////////////////////////

FilterSystem::FilterSystem() {
	bitmap = NULL;
	dwFlags = 0;
	lpBuffer = NULL;
	hFileShared = NULL;
	hdcSrc = NULL;
	hbmSrc = NULL;
	listFilters = NULL;
	fSharedWindow = false;
}

FilterSystem::~FilterSystem() {
	DeinitFilters();
	DeallocateBuffers();
	delete[] bitmap;
}

// prepareLinearChain(): init bitmaps in a linear filtering system

void FilterSystem::prepareLinearChain(List *listFA, Pixel *src_pal, PixDim src_width, PixDim src_height, int dest_depth) {
	FilterInstance *fa;
	DWORD flags, flags_accum=0;
	int last_bufferid = 0;

	if (dwFlags & FILTERS_INITIALIZED)
		return;

	DeallocateBuffers();

	AllocateVBitmaps(4);

	bitmap[0].w					= src_width;
	bitmap[0].h					= src_height;
	bitmap[0].palette			= src_pal;
	bitmap[0].buffer			= 0;
	bitmap[0].depth				= 32;
	bitmap[0].pitch				= bitmap[0].PitchAlign8();
	bitmap[0].modulo			= bitmap[0].Modulo();
	bitmap[0].size				= bitmap[0].pitch * bitmap[0].h;
	bitmap[0].offset			= 0;
	bitmap[0].buffer			= 0;

	bmLast = &bitmap[0];

	fa = (FilterInstance *)listFA->tail.next;

//	fSharedWindow = false;		CAN'T - NEED FOR OUTPUT DISPLAY
	fSharedWindow = true;
	lAdditionalBytes = 0;
	nFrameLag = 0;

	while(fa->next) {
		fa->realSrc			= *bmLast;

		fa->origw		= fa->realSrc.w;
		fa->origh		= fa->realSrc.h;

		// Clamp the crop rect at this point to avoid going below 1x1.
		// We will throw an exception later during init.
		int realx1 = std::min<int>(fa->x1, fa->origw - 1);
		int realy2 = std::min<int>(fa->y2, fa->origh - 1);
		int realx2 = std::min<int>(fa->x2, (fa->origw - 1) - realx1);
		int realy1 = std::min<int>(fa->y1, (fa->origh - 1) - realy2);

		fa->realSrc.w		-= realx1 + realx2;
		fa->realSrc.h		-= realy1 + realy2;
		fa->realSrc.depth	= 32;
		fa->realSrc.modulo	= fa->realSrc.pitch - 4*fa->realSrc.w;
		fa->realSrc.offset	+= realy2 * fa->realSrc.pitch + realx1*4;
		fa->realSrc.size	= fa->realSrc.pitch * fa->realSrc.h;

		fa->realLast.w		= fa->realSrc.w;
		fa->realLast.h		= fa->realSrc.h;
		fa->realLast.offset	= 0;
		fa->realLast.depth	= 32;
		fa->realLast.AlignTo8();
		fa->realLast.dwFlags= 0;

		fa->realDst			= fa->realSrc;
		fa->realDst.offset	= 0;

		fa->realSrc.dwFlags	= 0;
		fa->realSrc.hdc		= NULL;

		fa->realDst.dwFlags	= 0;
		fa->realDst.hdc		= NULL;

		fa->srcbuf		= last_bufferid;

		if (fa->filter->paramProc) {
			VDCHECKPOINT;
			flags = fa->filter->paramProc(fa, &g_filterFuncs);
			VDCHECKPOINT;

			if (flags & FILTERPARAM_NEEDS_LAST) {
				lAdditionalBytes += fa->realLast.size;
			}
		} else
			flags = FILTERPARAM_SWAP_BUFFERS;

		nFrameLag += (flags>>16);

		fa->flags = flags;

		flags &= 0x0000ffff;
		flags_accum |= flags;

		fa->realDst.modulo	= fa->realDst.pitch - 4*fa->realDst.w;
		fa->realDst.size	= fa->realDst.pitch * fa->realDst.h;
		fa->dstbuf		= fa->srcbuf;

		int swapBuffer = 1;
		if (fa->srcbuf)
			swapBuffer = 3-fa->srcbuf;

		if (flags & FILTERPARAM_SWAP_BUFFERS) {
			// Alternate between buffers 1 and 2
			fa->dstbuf = swapBuffer;
		}

		// allocate destination buffer
		uint32 dstSizeRequired = fa->realDst.size+fa->realDst.offset;
		if (bitmap[fa->dstbuf].size < dstSizeRequired)
			bitmap[fa->dstbuf].size = dstSizeRequired;

		// check if we have a blend curve
		if (fa->GetAlphaParameterCurve()) {
			// if this is an in-place filter and we have a blend curve, allocate other buffer as well.
			if (!(flags & FILTERPARAM_SWAP_BUFFERS)) {
				fa->mBlendBuffer = swapBuffer;

				if (bitmap[swapBuffer].size < dstSizeRequired)
					bitmap[swapBuffer].size = dstSizeRequired;
			}
		}

		bmLast = (FilterSystemBitmap *)&fa->realDst;
		last_bufferid = fa->dstbuf;

		// Check if the filter needs a display context.  This requires us to
		// allocate a shared window instead of a private memory buffer.
		// Far more hazardous under Windows 95/98.

		if ((fa->realSrc.dwFlags | fa->realDst.dwFlags) & VFBitmap::NEEDS_HDC)
			fSharedWindow = true;

		// Next filter.

		fa = (FilterInstance *)fa->next;
	}

	// 2/3) Temp buffers
#if 0
	bitmap[1].depth			= 32;
	bitmap[1].w				= temp_width;
	bitmap[1].h				= temp_height;
	bitmap[1].AlignTo8();

	bitmap[2].depth			= 32;
	bitmap[2].w				= temp_width;
	bitmap[2].h				= temp_height;
	bitmap[2].AlignTo8();
#endif

	bitmap[3].w				= bmLast->w;
	bitmap[3].h				= bmLast->h;
	bitmap[3].depth			= dest_depth;
	bitmap[3].AlignTo4();

	fa = (FilterInstance *)listFA->tail.next;
}

// initLinearChain(): prepare for a linear filtering system

void FilterSystem::initLinearChain(List *listFA, Pixel *src_pal, PixDim src_width, PixDim src_height, int dest_depth) {
	FilterInstance *fa;
	long lRequiredSize;
	long lLastBufPtr = 0; 
	int i;

	DeinitFilters();
	DeallocateBuffers();

	listFilters = listFA;

	// buffers required:
	//
	// 1) Input buffer (8/16/24/32 bits)
	// 2) Temp buffer #1 (32 bits)
	// 3) Temp buffer #2 (32 bits)
	// 4) Output buffer (8/16/24/32 bits)
	// 5) [Optional] Last input buffer (32 bits)
	//
	// All temporary buffers must be aligned on an 8-byte boundary, and all
	// pitches must be a multiple of 8 bytes.  The exceptions are the source
	// and destination buffers, which may have pitches that are only 4-byte
	// multiples.

	prepareLinearChain(listFA, src_pal, src_width, src_height, dest_depth);

	lRequiredSize = lAdditionalBytes;

	for(i=0; i<iBitmapCount; i++) {
		bitmap[i].buffer		= i;
		bitmap[i].lMapOffset	= lRequiredSize;

		lRequiredSize		+= bitmap[i].size;

		// align lRequiredSize up to next 8

		lRequiredSize = (lRequiredSize+7) & -8;

	}

	AllocateBuffers(lRequiredSize);

	for(i=0; i<iBitmapCount; i++) {
		bitmap[i].data			= (Pixel *)(lpBuffer + bitmap[i].lMapOffset);
	}

	fa = (FilterInstance *)listFA->tail.next;

	while(fa->next) {
		if (fa->GetAlphaParameterCurve()) {
				// size check
			if (fa->src.w != fa->dst.w || fa->src.h != fa->dst.h) {
				throw MyError("Cannot start filter chain: Filter \"%s\" has a blend curve attached and has differing input and output sizes (%dx%d -> %dx%d). Input and output sizes must match."
					, fa->filter->name
					, fa->src.w
					, fa->src.h
					, fa->dst.w
					, fa->dst.h
					);
			}
		}

		if (fa->origw <= fa->x1 + fa->x2 || fa->origh <= fa->y1 + fa->y2)
			throw MyError("Cannot start filter chain: The input crop rectangle to filter \"%s\" is smaller than 1x1.", fa->filter->name);

		fa->realSrc.data		= (Pixel32 *)((char *)bitmap[fa->srcbuf].data + fa->realSrc.offset);
		fa->realDst.data		= (Pixel32 *)((char *)bitmap[fa->dstbuf].data + fa->realDst.offset);

		fa = (FilterInstance *)fa->next;
	}

	// Does the first filter require a display context?

	HDC hdcDisplay = NULL;

	fa = (FilterInstance *)listFA->tail.next;

	try {
		if (fa->next && (fa->realSrc.dwFlags & VFBitmap::NEEDS_HDC)) {
			BITMAPINFOHEADER bih;
			void *mem;

			bitmap[0].MakeBitmapHeader(&bih);

			if (!hdcDisplay) {
				hdcDisplay = CreateDC("DISPLAY", NULL, NULL, 0);

				if (!hdcDisplay)
					throw MyMemoryError();
			}

			hdcSrc = CreateCompatibleDC(hdcDisplay);

			if (!hdcSrc)
				throw MyMemoryError();

			hbmSrc = CreateDIBSection(hdcSrc, (BITMAPINFO *)&bih, DIB_RGB_COLORS, &mem, hFileShared, 0);

			if (!hbmSrc)
				throw MyMemoryError();

			hgoSrc = SelectObject(hdcSrc, hbmSrc);

			fa->realSrc.hdc = hdcSrc;
		}

		// Check all subsequent filters; copy over display contexts from destinations
		// to sources and create new destination DCs as necessary

		fa = (FilterInstance *)listFA->tail.next;

		while(fa->next) {
			FilterInstance *fa_next = (FilterInstance *)fa->next;

			if ((fa->realDst.dwFlags & VFBitmap::NEEDS_HDC) || (fa_next->next && (fa_next->realSrc.dwFlags & VFBitmap::NEEDS_HDC))) {
				BITMAPINFOHEADER bih;

				fa->realDst.MakeBitmapHeader(&bih);

				if (!hdcDisplay) {
					hdcDisplay = CreateDC("DISPLAY", NULL, NULL, 0);

					if (!hdcDisplay)
						throw MyMemoryError();
				}

				fa->realDst.hdc = CreateCompatibleDC(hdcDisplay);

				if (!fa->realDst.hdc)
					throw MyMemoryError();

				fa->hbmDst = CreateDIBSection(fa->realDst.hdc, (BITMAPINFO *)&bih, DIB_RGB_COLORS, &fa->pvDstView,
					hFileShared, bitmap[fa->dstbuf].lMapOffset + fa->realDst.offset);

				if (!fa->hbmDst)
					throw MyMemoryError();

				fa->hgoDst = SelectObject(fa->realDst.hdc, fa->hbmDst);

				if (fa_next->next && (fa_next->realSrc.dwFlags & VFBitmap::NEEDS_HDC))
					fa_next->realSrc.hdc = fa->realDst.hdc;
			}

			if (fa->flags & FILTERPARAM_NEEDS_LAST) {
				fa->last->data		 = (uint32 *)(lpBuffer + lLastBufPtr);

				if (fa->last->dwFlags & VFBitmap::NEEDS_HDC) {
					BITMAPINFOHEADER bih;

					fa->realLast.MakeBitmapHeader(&bih);

					if (!hdcDisplay) {
						hdcDisplay = CreateDC("DISPLAY", NULL, NULL, 0);

						if (!hdcDisplay)
							throw MyMemoryError();
					}

					fa->last->hdc = CreateCompatibleDC(hdcDisplay);

					if (!fa->last->hdc)
						throw MyMemoryError();

					fa->hbmLast = CreateDIBSection(fa->last->hdc, (BITMAPINFO *)&bih, DIB_RGB_COLORS, &fa->pvLastView,
						hFileShared, lLastBufPtr);

					if (!fa->hbmLast)
						throw MyMemoryError();

					fa->hgoLast = SelectObject(fa->last->hdc, fa->hbmLast);
				}
				lLastBufPtr += fa->last->size;
			}

			fa = fa_next;
		}
	} catch(const MyError&) {
		if (hdcDisplay)
			DeleteDC(hdcDisplay);

		throw;
	}

	if (hdcDisplay)
		DeleteDC(hdcDisplay);
}

int FilterSystem::ReadyFilters(const FilterStateInfo& fsi) {
	FilterInstance *fa = (FilterInstance *)listFilters->tail.next;
	int rcode = 0;

	if (dwFlags & FILTERS_INITIALIZED)
		return 0;

	dwFlags &= ~FILTERS_ERROR;

	mfsi = fsi;
	FilterStateInfo *pfsiPrev = &mfsi;

	try {
		while(fa->next) {
			int nDelay = fa->flags >> 16;

			if (nDelay) {
				fa->nDelayRingSize = nDelay;
				fa->nDelayRingPos = 0;
				fa->pfsiDelayRing = new FilterStateInfo[nDelay];

				fa->pfsiDelayInput = pfsiPrev;
				fa->pfsi = &fa->fsiDelay;
				pfsiPrev = &fa->fsiDelayOutput;
			} else
				fa->pfsi = pfsiPrev;

			VDCHECKPOINT;

			if (fa->filter->startProc) {
				try {
					VDExternalCodeBracket bracket(fa->mFilterName.c_str(), __FILE__, __LINE__);

					vdprotected1("starting filter \"%s\"", const char *, fa->filter->name) {
						rcode = fa->filter->startProc(fa, &g_filterFuncs);
					}

				} catch(const MyError& e) {
					throw MyError("Cannot start filter '%s': %s", fa->filter->name, e.gets());
				}
				if (rcode)
					break;
			}

			fa = (FilterInstance *)fa->next;
		}

		VDCHECKPOINT;
	} catch(const MyError&) {
		// roll back previously initialized filters (similar to deinit)
		while(fa->prev) {
			if (fa->filter->endProc) {
				VDExternalCodeBracket bracket(fa->mFilterName.c_str(), __FILE__, __LINE__);
				vdprotected1("stopping filter \"%s\"", const char *, fa->filter->name) {
					fa->filter->endProc(fa, &g_filterFuncs);
				}
			}

			delete[] fa->pfsiDelayRing;
			fa->pfsiDelayRing = NULL;

			fa = (FilterInstance *)fa->prev;
		}

		delete[] bitmap;
		bitmap = NULL;

		throw;
	}

	dwFlags |= FILTERS_INITIALIZED;

	if (rcode)
		DeinitFilters();

	RestartFilters();
	return rcode;
}

void FilterSystem::RestartFilters() {
	mbFirstFrame = true;
	mFrameDelayLeft = nFrameLag;
}

bool FilterSystem::RunFilters(const FilterStateInfo& fsi, FilterInstance *pfiStopPoint) {
	mfsi = fsi;

	if (listFilters->IsEmpty())
		return true;

	if (dwFlags & FILTERS_ERROR)
		return false;

	FilterInstance *fa = (FilterInstance *)listFilters->tail.next;

	if (!(dwFlags & FILTERS_INITIALIZED))
		return false;

	if (fa->next && fa->srcbuf != 0)
		fa->realSrc.BitBlt(0, 0, &bitmap[0], fa->x1, fa->y1, -1, -1);

	while(fa->next && fa != pfiStopPoint) {

		if (fa->realSrc.dwFlags & VFBitmap::NEEDS_HDC) {
			LONG comp;
			RECT r;

			r.left		=			  fa->x1; comp  = fa->x1;
			r.right		= fa->realSrc.w	- fa->x2; comp |= fa->x2;
			r.top		=			  fa->y1; comp |= fa->y1;
			r.bottom	= fa->realSrc.h	- fa->y2; comp |= fa->y2;

			if (comp) {
				IntersectClipRect(fa->realSrc.hdc, r.left, r.top, r.right, r.bottom);
				SetWindowOrgEx(fa->realSrc.hdc, fa->x1, fa->y1, NULL);
			}
		}

		if (fa->realDst.dwFlags & VFBitmap::NEEDS_HDC) {
			SetViewportOrgEx(fa->realDst.hdc, 0, 0, NULL);
			SelectClipRgn(fa->realDst.hdc, NULL);
			IntersectClipRect(fa->realDst.hdc, 0, 0, fa->realDst.w, fa->realDst.h);
		}

		// If the filter has a delay ring...

		if (fa->pfsiDelayRing) {
			if (mbFirstFrame) {
				for(int i=0; i<fa->nDelayRingSize; ++i)
					fa->pfsiDelayRing[i] = *fa->pfsiDelayInput;
			}

			// Create composite FilterStateInfo structure for lagged filter.

			const FilterStateInfo& fsiIn = *fa->pfsiDelayInput;
			FilterStateInfo& fsiOut = fa->pfsiDelayRing[fa->nDelayRingPos];

			fa->fsiDelay = fsiIn;
			fa->fsiDelay.lCurrentFrame = fsiOut.lCurrentFrame;
			fa->fsiDelay.lDestFrameMS = fsiOut.lDestFrameMS;

			// Send out old value, read in new value, and advance ring.

			fa->fsiDelayOutput = fsiOut;
			fsiOut = fsiIn;

			if (++fa->nDelayRingPos >= fa->nDelayRingSize)
				fa->nDelayRingPos = 0;
		}

		// Compute alpha blending value.
		float alpha = 1.0f;

		VDParameterCurve *pAlphaCurve = fa->GetAlphaParameterCurve();
		if (pAlphaCurve)
			alpha = (float)(*pAlphaCurve)(fsi.lCurrentFrame).mY;

		// If this is an in-place filter with an alpha curve, save off the old image.
		VDPixmap oldImage;
		bool skipFilter = false;
		bool skipBlit = false;

		if (alpha < 254.5f / 255.0f) {
			if (fa->flags & FILTERPARAM_SWAP_BUFFERS) {
				oldImage = VDAsPixmap(fa->realSrc);
				if (alpha < 0.5f / 255.0f)
					skipFilter = true;
			} else {
				oldImage = VDAsPixmap(fa->realDst);
				oldImage.data = (char *)bitmap[fa->mBlendBuffer].data + fa->realDst.offset - oldImage.pitch*(oldImage.h-1);

				if (alpha < 0.5f / 255.0f) {
					skipFilter = true;

					if (fa->realSrc.data == fa->realDst.data && fa->realSrc.pitch == fa->realDst.pitch)
						skipBlit = true;
				}

				if (!skipBlit)
					VDPixmapBlt(oldImage, VDAsPixmap(fa->realSrc));
			}
		}

		if (!skipFilter) {
			// Run the filter.

			VDCHECKPOINT;

			try {
				vdprotected1("running filter \"%s\"", const char *, fa->filter->name) {
					VDExternalCodeBracket bracket(fa->mFilterName.c_str(), __FILE__, __LINE__);

					// Deliberately ignore the return code. It was supposed to be an error value,
					// but earlier versions didn't check it and logoaway returns true in some cases.
					fa->filter->runProc(fa, &g_filterFuncs);
				}
			} catch(const MyError& e) {
				dwFlags |= FILTERS_ERROR;
				throw MyError("Error running filter '%s': %s", fa->filter->name, e.gets());
			}
			VDCHECKPOINT;

			if (fa->realDst.dwFlags & VFBitmap::NEEDS_HDC)
				::GdiFlush();
		}

		if (!skipBlit && alpha < 254.5f / 255.0f) {
			if (alpha > 0.5f / 255.0f)
				VDPixmapBltAlphaConst(VDAsPixmap(fa->realDst), oldImage, 1.0f - alpha);
			else
				VDPixmapBlt(VDAsPixmap(fa->realDst), oldImage);
		}

		if (fa->flags & FILTERPARAM_NEEDS_LAST)
			fa->realLast.BitBlt(0, 0, &fa->realSrc, 0, 0, -1, -1);


		fa = (FilterInstance *)fa->next;
	}

	mbFirstFrame = false;

	if (mFrameDelayLeft) {
		--mFrameDelayLeft;
		return false;
	}

	return true;
}

void FilterSystem::DeinitFilters() {
	if (!listFilters)
		return;

	FilterInstance *fa = (FilterInstance *)listFilters->tail.next;

	if (!(dwFlags & FILTERS_INITIALIZED))
		return;

	// send all filters a 'stop'

	while(fa->next) {
		VDCHECKPOINT;

		if (fa->filter->endProc) {
			VDExternalCodeBracket bracket(fa->mFilterName.c_str(), __FILE__, __LINE__);
			vdprotected1("stopping filter \"%s\"", const char *, fa->filter->name) {
					fa->filter->endProc(fa, &g_filterFuncs);
			}
		}

		delete[] fa->pfsiDelayRing;
		fa->pfsiDelayRing = NULL;

		fa = (FilterInstance *)fa->next;
	}
	VDCHECKPOINT;

	dwFlags &= ~FILTERS_INITIALIZED;

	delete[] bitmap;
	bitmap = NULL;
}

VBitmap *FilterSystem::InputBitmap() {
	return &bitmap[0];
}

VBitmap *FilterSystem::OutputBitmap() {
	return &bitmap[3];
}

VBitmap *FilterSystem::LastBitmap() {
	return bmLast;
}

bool FilterSystem::isRunning() {
	return !!(dwFlags & FILTERS_INITIALIZED);
}

int FilterSystem::getFrameLag() {
	return nFrameLag;
}

bool FilterSystem::getOutputMappingParams(HANDLE& hr, LONG& lr) {
	hr = hFileShared;
	lr = bitmap[3].lMapOffset + bitmap[3].offset;

	return true;
}

bool FilterSystem::IsFiltered(VDPosition frame) const {
	if (listFilters->IsEmpty())
		return false;

	if (dwFlags & FILTERS_ERROR)
		return false;

	FilterInstance *fa = (FilterInstance *)listFilters->tail.next;

	if (!(dwFlags & FILTERS_INITIALIZED))
		return false;

	while(fa->next) {
		VDParameterCurve *pAlphaCurve = fa->GetAlphaParameterCurve();
		if (!pAlphaCurve)
			return true;

		float alpha = (float)(*pAlphaCurve)((double)frame).mY;

		if (alpha >= (0.5f / 255.0f))
			return true;

		fa = (FilterInstance *)fa->next;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////
//
//	FilterSystem::private_methods
//
/////////////////////////////////////////////////////////////////////////////

void FilterSystem::AllocateVBitmaps(int count) {
	delete[] bitmap;

	if (!(bitmap = new FilterSystemBitmap[count])) throw MyMemoryError();
//	memset(bitmap, 0, sizeof(VBitmap) * count);

	for(int i=0; i<count; i++) {
		bitmap[i].data		= NULL;
		bitmap[i].palette	= NULL;
		bitmap[i].buffer	= 0;
		bitmap[i].depth		= 0;
		bitmap[i].w			= 0;
		bitmap[i].h			= 0;
		bitmap[i].pitch		= 0;
		bitmap[i].modulo	= 0;
		bitmap[i].size		= 0;
		bitmap[i].offset	= 0;
	}

	iBitmapCount = count;
}

void FilterSystem::AllocateBuffers(LONG lTotalBufferNeeded) {
	DeallocateBuffers();

	if (fSharedWindow) {

		if (!(hFileShared = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, lTotalBufferNeeded+8, NULL)))
			throw MyError("Could not allocate shared memory.");

		if (!(lpBuffer = (unsigned char *)MapViewOfFile(hFileShared, FILE_MAP_ALL_ACCESS, 0, 0, lTotalBufferNeeded+8)))
			throw MyError("Could not map shared memory into local space.");

	} else {

		if (!(lpBuffer = (unsigned char *)VirtualAlloc(NULL, lTotalBufferNeeded+8, MEM_COMMIT, PAGE_READWRITE)))
			throw MyMemoryError();

	}

	memset(lpBuffer, 0, lTotalBufferNeeded+8);
}

void FilterSystem::DeallocateBuffers() {
	if (listFilters) {
		// delete hdcs

		FilterInstance *fa = (FilterInstance *)listFilters->tail.next;

		while(fa->next) {
			if (fa->realDst.hdc) {
				_RPT2(0,"Deleting realDst display context from %p (%s)\n", fa, fa->filter->name);
				DeleteObject(SelectObject(fa->realDst.hdc, fa->hgoDst));
				UnmapViewOfFile(fa->pvDstView);		// avoid NT4 GDI 64K memory leak bug
				DeleteDC(fa->realDst.hdc);
				fa->realDst.hdc = NULL;
			}
			if (fa->last->hdc) {
				_RPT2(0,"Deleting last display context from %p (%s)\n", fa, fa->filter->name);
				DeleteObject(SelectObject(fa->last->hdc, fa->hgoLast));
				UnmapViewOfFile(fa->pvLastView);		// avoid NT4 GDI 64K memory leak bug
				DeleteDC(fa->last->hdc);
				fa->last->hdc = NULL;
			}

			fa = (FilterInstance *)fa->next;
		}
	}

	if (hdcSrc) {
		DeleteObject(SelectObject(hdcSrc, hgoSrc));
		DeleteDC(hdcSrc);
		hdcSrc = NULL;
	}

	if (lpBuffer) {
		if (fSharedWindow)
			UnmapViewOfFile(lpBuffer);
		else
			VirtualFree(lpBuffer, 0, MEM_RELEASE);

		lpBuffer = NULL;
	}
	if (hFileShared) { CloseHandle(hFileShared); hFileShared = NULL; }
}
