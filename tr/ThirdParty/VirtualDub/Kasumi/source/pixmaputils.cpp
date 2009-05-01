#include <vd2/Kasumi/pixmaputils.h>
#include <vd2/system/memory.h>

extern VDPixmapFormatInfo g_vdPixmapFormats[] = {
	/* Null */			{ "null",		0, 0, 0, 0, 0, 0, 0, 0,   0 },
	/* Pal1 */			{ "Pal1",		8, 1, 3, 0, 1, 0, 0, 0,   2 },
	/* Pal2 */			{ "Pal2",		4, 1, 2, 0, 1, 0, 0, 0,   4 },
	/* Pal4 */			{ "Pal4",		2, 1, 1, 0, 1, 0, 0, 0,  16 },
	/* Pal8 */			{ "Pal8",		1, 1, 0, 0, 1, 0, 0, 0, 256 },
	/* RGB16_555 */		{ "XRGB1555",	1, 1, 0, 0, 2, 0, 0, 0,   0 },
	/* RGB16_565 */		{ "RGB565",		1, 1, 0, 0, 2, 0, 0, 0,   0 },
	/* RGB24 */			{ "RGB888",		1, 1, 0, 0, 3, 0, 0, 0,   0 },
	/* RGB32 */			{ "XRGB8888",	1, 1, 0, 0, 4, 0, 0, 0,   0 },
	/* Y8 */			{ "Y8",			1, 1, 0, 0, 1, 0, 0, 0,   0 },
	/* YUV422_UYVY */	{ "UYVY",		2, 1, 1, 0, 4, 0, 0, 0,   0 },
	/* YUV422_YUYV */	{ "YUYV",		2, 1, 1, 0, 4, 0, 0, 0,   0 },
	/* YUV444_XVYU */	{ "XVYU",		1, 1, 0, 0, 4, 0, 0, 0,   0 },
	/* YUV444_Planar */	{ "YUV444",		1, 1, 0, 0, 1, 2, 0, 0,   0 },
	/* YUV422_Planar */	{ "YUV422",		1, 1, 0, 0, 1, 2, 1, 0,   0 },
	/* YUV420_Planar */	{ "YUV420",		1, 1, 0, 0, 1, 2, 1, 1,   0 },
	/* YUV411_Planar */	{ "YUV411",		1, 1, 0, 0, 1, 2, 2, 0,   0 },
	/* YUV410_Planar */	{ "YUV410",		1, 1, 0, 0, 1, 2, 2, 2,   0 },
};

#ifdef _DEBUG
	bool VDIsValidPixmapPlane(const void *p, ptrdiff_t pitch, vdpixsize w, vdpixsize h) {
		bool isvalid;

		if (pitch < 0)
			isvalid = VDIsValidReadRegion((const char *)p + pitch*(h-1), (-pitch)*(h-1)+w);
		else
			isvalid = VDIsValidReadRegion(p, pitch*(h-1)+w);

		if (!isvalid) {
			VDDEBUG("Kasumi: Invalid pixmap plane detected.\n"
					"        Base=%p, pitch=%d, size=%dx%d (bytes)\n", p, (int)pitch, w, h);
		}

		return isvalid;
	}

	bool VDAssertValidPixmap(const VDPixmap& px) {
		const VDPixmapFormatInfo& info = VDPixmapGetInfo(px.format);

		if (px.format) {
			if (!VDIsValidPixmapPlane(px.data, px.pitch, -(-px.w >> info.qwbits)*info.qsize, -(-px.h >> info.qhbits))) {
				VDDEBUG("Kasumi: Invalid primary plane detected in pixmap.\n"
						"        Pixmap info: format=%d, dimensions=%dx%d\n", px.format, px.w, px.h);
				VDASSERT(!"Kasumi: Invalid primary plane detected in pixmap.\n");
				return false;
			}

			if (info.palsize)
				if (!VDIsValidReadRegion(px.palette, sizeof(uint32) * info.palsize)) {
					VDDEBUG("Kasumi: Invalid palette detected in pixmap.\n"
							"        Pixmap info: format=%d, dimensions=%dx%d\n", px.format, px.w, px.h);
					VDASSERT(!"Kasumi: Invalid palette detected in pixmap.\n");
					return false;
				}

			if (info.auxbufs) {
				const vdpixsize auxw = -(-px.w >> info.auxwbits);
				const vdpixsize auxh = -(-px.h >> info.auxhbits);

				if (!VDIsValidPixmapPlane(px.data2, px.pitch2, auxw, auxh)) {
					VDDEBUG("Kasumi: Invalid Cb plane detected in pixmap.\n"
							"        Pixmap info: format=%d, dimensions=%dx%d\n", px.format, px.w, px.h);
					VDASSERT(!"Kasumi: Invalid Cb plane detected in pixmap.\n");
					return false;
				}

				if (info.auxbufs > 2) {
					if (!VDIsValidPixmapPlane(px.data3, px.pitch3, auxw, auxh)) {
						VDDEBUG("Kasumi: Invalid Cr plane detected in pixmap.\n"
								"        Pixmap info: format=%d, dimensions=%dx%d\n", px.format, px.w, px.h);
						VDASSERT(!"Kasumi: Invalid Cr plane detected in pixmap.\n");
						return false;
					}
				}
			}
		}

		return true;
	}
#endif

VDPixmap VDPixmapOffset(const VDPixmap& src, vdpixpos x, vdpixpos y) {
	VDPixmap temp(src);
	const VDPixmapFormatInfo& info = VDPixmapGetInfo(temp.format);

	x >>= info.qwbits;
	y >>= info.qhbits;

	switch(info.auxbufs) {
	case 2:
		temp.data3 = (char *)temp.data3 + (x >> info.auxwbits) + (y >> info.auxhbits)*temp.pitch3;
	case 1:
		temp.data2 = (char *)temp.data2 + (x >> info.auxwbits) + (y >> info.auxhbits)*temp.pitch2;
	case 0:
		temp.data = (char *)temp.data + x*info.qsize + y*temp.pitch;
	}

	return temp;
}

VDPixmapLayout VDPixmapLayoutOffset(const VDPixmapLayout& src, vdpixpos x, vdpixpos y) {
	VDPixmapLayout temp(src);
	const VDPixmapFormatInfo& info = VDPixmapGetInfo(temp.format);

	x = -(-x >> info.qwbits);
	y = -(-y >> info.qhbits);

	switch(info.auxbufs) {
	case 2:
		temp.data3 += -(-x >> info.auxwbits) + -(-y >> info.auxhbits)*temp.pitch3;
	case 1:
		temp.data2 += -(-x >> info.auxwbits) + -(-y >> info.auxhbits)*temp.pitch2;
	case 0:
		temp.data += x*info.qsize + y*temp.pitch;
	}

	return temp;
}

uint32 VDPixmapCreateLinearLayout(VDPixmapLayout& layout, int format, vdpixsize w, vdpixsize h, int alignment) {
	const ptrdiff_t alignmask = alignment - 1;

	const VDPixmapFormatInfo& srcinfo = VDPixmapGetInfo(format);
	sint32		qw			= -(-w >> srcinfo.qwbits);
	sint32		qh			= -(-h >> srcinfo.qhbits);
	sint32		subw		= -(-w >> srcinfo.auxwbits);
	sint32		subh		= -(-h >> srcinfo.auxhbits);

	ptrdiff_t	mainpitch	= (srcinfo.qsize * qw + alignmask) & ~alignmask;
	size_t		mainsize	= mainpitch * qh;

	layout.data		= 0;
	layout.pitch	= mainpitch;
	layout.palette	= NULL;
	layout.data2	= 0;
	layout.pitch2	= 0;
	layout.data3	= 0;
	layout.pitch3	= 0;
	layout.w		= w;
	layout.h		= h;
	layout.format	= format;

	if (srcinfo.auxbufs >= 1) {
		ptrdiff_t	subpitch	= (subw + alignmask) & ~alignmask;
		size_t		subsize		= subpitch * subh;

		layout.data2	= mainsize;
		layout.pitch2	= subpitch;
		mainsize += subsize;

		if (srcinfo.auxbufs >= 2) {
			layout.data3	= mainsize;
			layout.pitch3	= subpitch;
			mainsize += subsize;
		}
	}

	return mainsize;
}

void VDPixmapFlipV(VDPixmap& px) {
	const VDPixmapFormatInfo& srcinfo = VDPixmapGetInfo(px.format);
	sint32		w			= px.w;
	sint32		h			= px.h;
	sint32		qw			= -(-w >> srcinfo.qwbits);
	sint32		qh			= -(-h >> srcinfo.qhbits);
	sint32		subw		= -(-w >> srcinfo.auxwbits);
	sint32		subh		= -(-h >> srcinfo.auxhbits);

	vdptrstep(px.data, px.pitch * (qh - 1));
	px.pitch = -px.pitch;

	if (srcinfo.auxbufs >= 1) {
		vdptrstep(px.data2, px.pitch2 * (subh - 1));
		px.pitch2 = -px.pitch2;

		if (srcinfo.auxbufs >= 2) {
			vdptrstep(px.data3, px.pitch3 * (subh - 1));
			px.pitch3 = -px.pitch3;
		}
	}
}

void VDPixmapLayoutFlipV(VDPixmapLayout& layout) {
	const VDPixmapFormatInfo& srcinfo = VDPixmapGetInfo(layout.format);
	sint32		w			= layout.w;
	sint32		h			= layout.h;
	sint32		qw			= -(-w >> srcinfo.qwbits);
	sint32		qh			= -(-h >> srcinfo.qhbits);
	sint32		subw		= -(-w >> srcinfo.auxwbits);
	sint32		subh		= -(-h >> srcinfo.auxhbits);

	layout.data += layout.pitch * (qh - 1);
	layout.pitch = -layout.pitch;

	if (srcinfo.auxbufs >= 1) {
		layout.data2 += layout.pitch2 * (subh - 1);
		layout.pitch2 = -layout.pitch2;

		if (srcinfo.auxbufs >= 2) {
			layout.data3 += layout.pitch3 * (subh - 1);
			layout.pitch3 = -layout.pitch3;
		}
	}
}

///////////////////////////////////////////////////////////////////////////

VDPixmapBuffer::VDPixmapBuffer(const VDPixmap& src)
	: mpBuffer(NULL)
	, mLinearSize(0)
{
	assign(src);
}

VDPixmapBuffer::VDPixmapBuffer(const VDPixmapBuffer& src)
	: mpBuffer(NULL)
	, mLinearSize(0)
{
	assign(src);
}

VDPixmapBuffer::VDPixmapBuffer(const VDPixmapLayout& layout) {
	init(layout);
}

VDPixmapBuffer::~VDPixmapBuffer() {
#ifdef _DEBUG
	validate();
#endif

	delete[] mpBuffer;
}

void VDPixmapBuffer::init(sint32 width, sint32 height, int f) {
	const VDPixmapFormatInfo& srcinfo = VDPixmapGetInfo(f);
	sint32		qw			= -(-width >> srcinfo.qwbits);
	sint32		qh			= -(-height >> srcinfo.qhbits);
	sint32		subw		= -(-width >> srcinfo.auxwbits);
	sint32		subh		= -(-height >> srcinfo.auxhbits);
	ptrdiff_t	mainpitch	= (srcinfo.qsize * qw + 15) & ~15;
	ptrdiff_t	subpitch	= (subw + 15) & ~15;
	size_t		mainsize	= mainpitch * qh;
	size_t		subsize		= subpitch * subh;
	size_t		totalsize	= mainsize + subsize*srcinfo.auxbufs + 4 * srcinfo.palsize;

#ifdef _DEBUG
	totalsize += 28;
#endif

	if (mLinearSize != totalsize) {
		clear();
		mpBuffer = new char[totalsize + 15];
		mLinearSize = totalsize;
	}

	char *p = mpBuffer + (-(int)(uintptr)mpBuffer & 15);

#ifdef _DEBUG
	*(uint32 *)p = totalsize;
	for(int i=0; i<12; ++i)
		p[4+i] = (char)(0xa0 + i);

	p += 16;
#endif

	data	= p;
	pitch	= mainpitch;
	p += mainsize;

	palette	= NULL;
	data2	= NULL;
	pitch2	= NULL;
	data3	= NULL;
	pitch3	= NULL;
	w		= width;
	h		= height;
	format	= f;

	if (srcinfo.auxbufs >= 1) {
		data2	= p;
		pitch2	= subpitch;
		p += subsize;
	}

	if (srcinfo.auxbufs >= 2) {
		data3	= p;
		pitch3	= subpitch;
		p += subsize;
	}

	if (srcinfo.palsize) {
		palette = (const uint32 *)p;
		p += srcinfo.palsize * 4;
	}

#ifdef _DEBUG
	for(int j=0; j<12; ++j)
		p[j] = (char)(0xb0 + j);
#endif
}

void VDPixmapBuffer::init(const VDPixmapLayout& layout) {
	const VDPixmapFormatInfo& srcinfo = VDPixmapGetInfo(layout.format);
	sint32		qw			= -(-layout.w >> srcinfo.qwbits);
	sint32		qh			= -(-layout.h >> srcinfo.qhbits);
	sint32		subw		= -(-layout.w >> srcinfo.auxwbits);
	sint32		subh		= -(-layout.h >> srcinfo.auxhbits);

	ptrdiff_t mino=0, maxo=0;

	if (layout.pitch < 0) {
		mino = std::min<ptrdiff_t>(mino, layout.data + layout.pitch * (qh-1));
		maxo = std::max<ptrdiff_t>(maxo, layout.data - layout.pitch);
	} else {
		mino = std::min<ptrdiff_t>(mino, layout.data);
		maxo = std::max<ptrdiff_t>(maxo, layout.data + layout.pitch*qh);
	}

	if (srcinfo.auxbufs >= 1) {
		if (layout.pitch2 < 0) {
			mino = std::min<ptrdiff_t>(mino, layout.data2 + layout.pitch2 * (subh-1));
			maxo = std::max<ptrdiff_t>(maxo, layout.data2 - layout.pitch2);
		} else {
			mino = std::min<ptrdiff_t>(mino, layout.data2);
			maxo = std::max<ptrdiff_t>(maxo, layout.data2 + layout.pitch2*subh);
		}

		if (srcinfo.auxbufs >= 2) {
			if (layout.pitch3 < 0) {
				mino = std::min<ptrdiff_t>(mino, layout.data3 + layout.pitch3 * (subh-1));
				maxo = std::max<ptrdiff_t>(maxo, layout.data3 - layout.pitch3);
			} else {
				mino = std::min<ptrdiff_t>(mino, layout.data3);
				maxo = std::max<ptrdiff_t>(maxo, layout.data3 + layout.pitch3*subh);
			}
		}
	}

	ptrdiff_t linsize = ((maxo - mino + 3) & ~(uintptr)3);

	ptrdiff_t totalsize = linsize + 4*srcinfo.palsize;

#ifdef _DEBUG
	totalsize += 28;
#endif

	if (mLinearSize != totalsize) {
		clear();
		mpBuffer = new char[totalsize + 15];
		mLinearSize = totalsize;
	}

	char *p = mpBuffer + (-(int)(uintptr)mpBuffer & 15);

#ifdef _DEBUG
	*(uint32 *)p = totalsize - 28;
	for(int i=0; i<12; ++i)
		p[4+i] = (char)(0xa0 + i);

	p += 16;
#endif

	w		= layout.w;
	h		= layout.h;
	format	= layout.format;
	data	= p + layout.data - mino;
	data2	= p + layout.data2 - mino;
	data3	= p + layout.data3 - mino;
	pitch	= layout.pitch;
	pitch2	= layout.pitch2;
	pitch3	= layout.pitch3;
	palette	= NULL;

	if (srcinfo.palsize) {
		palette = (const uint32 *)(p + linsize);
		memcpy((void *)palette, layout.palette, 4*srcinfo.palsize);
	}

#ifdef _DEBUG
	for(int j=0; j<12; ++j)
		p[totalsize + j - 28] = (char)(0xb0 + j);
#endif

	VDAssertValidPixmap(*this);
}

void VDPixmapBuffer::assign(const VDPixmap& src) {
	if (!src.format) {
		delete[] mpBuffer;
		mpBuffer = NULL;
		data = NULL;
		format = 0;
	} else {
		init(src.w, src.h, src.format);

		const VDPixmapFormatInfo& srcinfo = VDPixmapGetInfo(src.format);
		int qw = (src.w >> srcinfo.qwbits);
		int qh = src.h >> srcinfo.qhbits;
		int subw = -(-src.w >> srcinfo.auxwbits);
		int subh = -(-src.h >> srcinfo.auxhbits);

		if (srcinfo.palsize)
			memcpy((void *)palette, src.palette, 4 * srcinfo.palsize);

		switch(srcinfo.auxbufs) {
		case 2:
			VDMemcpyRect(data3, pitch3, src.data3, src.pitch3, subw, subh);
		case 1:
			VDMemcpyRect(data2, pitch2, src.data2, src.pitch2, subw, subh);
		case 0:
			VDMemcpyRect(data, pitch, src.data, src.pitch, qw * srcinfo.qsize, qh);
		}
	}
}

void VDPixmapBuffer::swap(VDPixmapBuffer& dst) {
	std::swap(mpBuffer, dst.mpBuffer);
	std::swap(mLinearSize, dst.mLinearSize);
	std::swap(static_cast<VDPixmap&>(*this), static_cast<VDPixmap&>(dst));
}

#ifdef _DEBUG
void VDPixmapBuffer::validate() {
	if (mpBuffer) {
		char *p = (char *)(((uintptr)mpBuffer + 15) & ~(uintptr)15);

		// verify head bytes
		for(int i=0; i<12; ++i)
			if (p[i+4] != (char)(0xa0 + i))
				VDASSERT(!"VDPixmapBuffer: Buffer underflow detected.\n");

		// verify tail bytes
		for(int j=0; j<12; ++j)
			if (p[mLinearSize - 12 + j] != (char)(0xb0 + j))
				VDASSERT(!"VDPixmapBuffer: Buffer overflow detected.\n");
	}
}
#endif
