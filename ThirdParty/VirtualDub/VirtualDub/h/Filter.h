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

#ifndef f_FILTER_H
#define f_FILTER_H

#include <windows.h>

#include <vd2/plugin/vdvideofiltold.h>

#include "VBitmap.h"

//////////////////

struct CScriptObject;
///////////////////

class VFBitmap;
class FilterActivation;
struct FilterFunctions;

typedef int  (*FilterInitProc     )(FilterActivation *fa, const FilterFunctions *ff);
typedef void (*FilterDeinitProc   )(FilterActivation *fa, const FilterFunctions *ff);
typedef int  (*FilterRunProc      )(const FilterActivation *fa, const FilterFunctions *ff);
typedef long (*FilterParamProc    )(FilterActivation *fa, const FilterFunctions *ff);
typedef int  (*FilterConfigProc   )(FilterActivation *fa, const FilterFunctions *ff, HWND hWnd);
typedef void (*FilterStringProc   )(const FilterActivation *fa, const FilterFunctions *ff, char *buf);
typedef int  (*FilterStartProc    )(FilterActivation *fa, const FilterFunctions *ff);
typedef int  (*FilterEndProc      )(FilterActivation *fa, const FilterFunctions *ff);
typedef bool (*FilterScriptStrProc)(FilterActivation *fa, const FilterFunctions *, char *, int);
typedef void (*FilterStringProc2  )(const FilterActivation *fa, const FilterFunctions *ff, char *buf, int maxlen);
typedef int  (*FilterSerialize    )(FilterActivation *fa, const FilterFunctions *ff, char *buf, int maxbuf);
typedef void (*FilterDeserialize  )(FilterActivation *fa, const FilterFunctions *ff, const char *buf, int maxbuf);
typedef void (*FilterCopy         )(FilterActivation *fa, const FilterFunctions *ff, void *dst);

typedef int (__cdecl *FilterModuleInitProc)(struct FilterModule *fm, const FilterFunctions *ff, int& vdfd_ver, int& vdfd_compat);
typedef void (__cdecl *FilterModuleDeinitProc)(struct FilterModule *fm, const FilterFunctions *ff);

//////////

typedef void (__cdecl *FilterPreviewButtonCallback)(bool fNewState, void *pData);
typedef void (__cdecl *FilterPreviewSampleCallback)(VFBitmap *, long lFrame, long lCount, void *pData);

//////////

// These flags must match those in cpuaccel.h!

#ifndef f_VIRTUALDUB_CPUACCEL_H
#define CPUF_SUPPORTS_CPUID			(0x00000001L)
#define CPUF_SUPPORTS_FPU			(0x00000002L)
#define CPUF_SUPPORTS_MMX			(0x00000004L)
#define CPUF_SUPPORTS_INTEGER_SSE	(0x00000008L)
#define CPUF_SUPPORTS_SSE			(0x00000010L)
#define CPUF_SUPPORTS_SSE2			(0x00000020L)
#define CPUF_SUPPORTS_3DNOW			(0x00000040L)
#define CPUF_SUPPORTS_3DNOW_EXT		(0x00000080L)
#endif

#endif
