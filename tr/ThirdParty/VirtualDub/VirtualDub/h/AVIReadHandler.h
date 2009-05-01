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

#ifndef f_AVIREADHANDLER_H
#define f_AVIREADHANDLER_H

#include <windows.h>
#include <vfw.h>
#include <vector>
#include <list>
#include <vd2/system/VDString.h>

// These are meant as AVIFile replacements.  They're not quite to AVIFile
// specs, but they'll do for now.

class IAVIReadStream {
public:
	virtual ~IAVIReadStream();

	virtual HRESULT BeginStreaming(VDPosition lStart, VDPosition lEnd, long lRate)=0;
	virtual HRESULT EndStreaming()=0;
	virtual HRESULT Info(AVISTREAMINFO *pasi, long lSize)=0;
	virtual bool IsKeyFrame(VDPosition lFrame)=0;
	virtual HRESULT Read(VDPosition lStart, long lSamples, void *lpBuffer, long cbBuffer, long *plBytes, long *plSamples)=0;
	virtual VDPosition Start()=0;
	virtual VDPosition End()=0;
	virtual VDPosition PrevKeyFrame(VDPosition lFrame)=0;
	virtual VDPosition NextKeyFrame(VDPosition lFrame)=0;
	virtual VDPosition NearestKeyFrame(VDPosition lFrame)=0;
	virtual HRESULT FormatSize(VDPosition lFrame, long *plSize)=0;
	virtual HRESULT ReadFormat(VDPosition lFrame, void *pFormat, long *plSize)=0;
	virtual bool isStreaming()=0;
	virtual bool isKeyframeOnly()=0;

	virtual bool getVBRInfo(double& bitrate_mean, double& bitrate_stddev, double& maxdev)=0;
	virtual sint64		getSampleBytePosition(VDPosition sample_num) = 0;

	virtual VDPosition TimeToPosition(VDTime timeInMicroseconds) = 0;
	virtual VDTime PositionToTime(VDPosition pos) = 0;
};

class IAVIReadHandler {
public:
	virtual void AddRef()=0;
	virtual void Release()=0;
	virtual IAVIReadStream *GetStream(DWORD fccType, LONG lParam)=0;
	virtual void EnableFastIO(bool)=0;
	virtual bool isOptimizedForRealtime()=0;
	virtual bool isStreaming()=0;
	virtual bool isIndexFabricated()=0;
	virtual bool AppendFile(const wchar_t *pszFile)=0;
	virtual bool getSegmentHint(const char **ppszPath)=0;

	typedef std::list<std::pair<uint32, VDStringA> > tTextInfo;
	virtual void GetTextInfo(tTextInfo& textInfo) = 0;
	virtual void GetTextInfoEncoding(int& codePage, int& countryCode, int& language, int& dialect) = 0;
};

IAVIReadHandler *CreateAVIReadHandler(PAVIFILE paf);
IAVIReadHandler *CreateAVIReadHandler(const wchar_t *pszFile);

#endif
