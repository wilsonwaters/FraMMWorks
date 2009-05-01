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
#include "filter.h"
#include "filters.h"


extern FilterDefinition
	filterDef_chromasmoother,
	filterDef_fieldbob,
	filterDef_fieldswap,
	filterDef_fill,
	filterDef_invert,
	filterDef_null,
	filterDef_tsoften,
	filterDef_resize,
	filterDef_flipv,
	filterDef_fliph,
	filterDef_deinterlace,
	filterDef_rotate,
	filterDef_hsv,
	filterDef_warpresize

#ifdef _M_IX86
	,
	filterDef_reduceby2,
	filterDef_convolute,
	filterDef_sharpen,
	filterDef_brightcont,
	filterDef_emboss,
	filterDef_grayscale,
	filterDef_reduce2hq,
	filterDef_threshold,
	filterDef_tv,
	filterDef_smoother,
	filterDef_rotate2,
	filterDef_levels,
	filterDef_blur,
	filterDef_blurhi,
	filterDef_box,
	filterDef_timesmooth,
	filterDef_logo,
	filterDef_perspective
#endif
	;

static FilterDefinition *const builtin_filters[]={
	&filterDef_chromasmoother,
	&filterDef_fieldbob,
	&filterDef_fieldswap,
	&filterDef_fill,
	&filterDef_invert,
	&filterDef_null,
	&filterDef_tsoften,
	&filterDef_resize,
	&filterDef_flipv,
	&filterDef_fliph,
	&filterDef_deinterlace,
	&filterDef_rotate,
	&filterDef_hsv,
	&filterDef_warpresize,
#ifdef _M_IX86
	&filterDef_reduceby2,
	&filterDef_convolute,
	&filterDef_sharpen,
	&filterDef_brightcont,
	&filterDef_emboss,
	&filterDef_grayscale,
	&filterDef_reduce2hq,
	&filterDef_threshold,
	&filterDef_tv,
	&filterDef_smoother,
	&filterDef_rotate2,
	&filterDef_levels,
	&filterDef_blur,
	&filterDef_blurhi,
	&filterDef_box,
	&filterDef_timesmooth,
	&filterDef_logo,
	&filterDef_perspective,
#endif
	NULL
};

void InitBuiltinFilters() {
	FilterDefinition *cur, *const *cpp;

	cpp = builtin_filters;
	while(cur = *cpp++)
		FilterAddBuiltin(cur);
}
