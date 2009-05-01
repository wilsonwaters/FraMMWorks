#ifndef f_CAPFILTER_H
#define f_CAPFILTER_H

#include <vd2/Kasumi/pixmap.h>

class VDINTERFACE IVDCaptureFilterSystem {
public:
	enum FilterMode {
		kFilterDisable,
		kFilterLinear,
		kFilterCubic,

		kFilterCount
	};

	virtual ~IVDCaptureFilterSystem() {}

	virtual void SetCrop(uint32 x1, uint32 y1, uint32 x2, uint32 y2) = 0;
	virtual void SetNoiseReduction(uint32 threshold) = 0;
	virtual void SetLumaSquish(bool blackEnable, bool whiteEnable) = 0;
	virtual void SetFieldSwap(bool enable) = 0;
	virtual void SetVertSquashMode(FilterMode mode) = 0;
	virtual void SetChainEnable(bool enable) = 0;

	virtual void Init(VDPixmapLayout& layout, uint32 usPerFrame) = 0;
	virtual void Run(VDPixmap& px) = 0;
	virtual void Shutdown() = 0;
};

IVDCaptureFilterSystem *VDCreateCaptureFilterSystem();

#endif
