#ifndef f_VD2_CAPUTILS_H
#define f_VD2_CAPUTILS_H

#include <vd2/system/vdtypes.h>

class VDCaptureAutoPriority {
public:
	VDCaptureAutoPriority();
	~VDCaptureAutoPriority();

protected:
	bool		mbPowerOffState;
	bool		mbLowPowerState;
	bool		mbScreenSaverState;
};

int VDCaptureIsCatchableException(uint32 ec);
long VDCaptureHashDriverName(const char *name);

#endif