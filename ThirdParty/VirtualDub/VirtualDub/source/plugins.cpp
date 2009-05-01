#include "stdafx.h"
#include <list>
#include <vd2/system/VDString.h>
#include <vd2/system/debug.h>
#include <vd2/system/error.h>
#include <vd2/system/filesys.h>
#include <vd2/system/refcount.h>
#include <vd2/system/vdalloc.h>

#include <vd2/plugin/vdplugin.h>
#include <vd2/plugin/vdaudiofilt.h>
#include <vd2/plugin/vdvideofilt.h>
#include <vd2/plugin/vdinputdriver.h>

#include "plugins.h"
#include "misc.h"

extern FilterFunctions g_filterFuncs;

namespace {
	class VDShadowedPluginDescription : public VDPluginDescription, public vdrefcounted<IVDRefCount> {
	public:
		virtual ~VDShadowedPluginDescription() {}

		virtual void Init(const VDPluginInfo *pInfo, VDExternalModule *pModule) {
			mName			= pInfo->mpName;
			mAuthor			= pInfo->mpAuthor ? pInfo->mpAuthor : L"(internal)";
			mDescription	= pInfo->mpDescription;
			mVersion		= pInfo->mVersion;
			mType			= pInfo->mType;
			mpModule		= pModule;

			mShadowedInfo	= *pInfo;
			mShadowedInfo.mpName		= mName.c_str();
			mShadowedInfo.mpAuthor		= mAuthor.c_str();
			mShadowedInfo.mpDescription	= mDescription.c_str();
		}

		VDPluginInfo			mShadowedInfo;
	};

	class VDShadowedVideoFilterDescription : public VDShadowedPluginDescription {
	public:
		virtual void Init(const VDPluginInfo *pInfo, VDExternalModule *pModule) {
			VDShadowedPluginDescription::Init(pInfo, pModule);

			const VDVideoFilterDefinition *def = static_cast<const VDVideoFilterDefinition *>(pInfo->mpTypeSpecificInfo);
			memset(&mDefinition, 0, sizeof mDefinition);
			memcpy(&mDefinition, def, std::min<uint32>(def->mSize, sizeof mDefinition));
			mDefinition.mpConfigInfo = NULL;
			mShadowedInfo.mpTypeSpecificInfo = &mDefinition;
		}

		VDVideoFilterDefinition	mDefinition;
	};

	class VDShadowedAudioFilterDescription : public VDShadowedPluginDescription {
	public:
		virtual void Init(const VDPluginInfo *pInfo, VDExternalModule *pModule) {
			VDShadowedPluginDescription::Init(pInfo, pModule);

			const VDAudioFilterDefinition *def = static_cast<const VDAudioFilterDefinition *>(pInfo->mpTypeSpecificInfo);
			memset(&mDefinition, 0, sizeof mDefinition);
			memcpy(&mDefinition, def, std::min<uint32>(def->mSize, sizeof mDefinition));
			mDefinition.mpConfigInfo = NULL;
			mShadowedInfo.mpTypeSpecificInfo = &mDefinition;
		}

	protected:
		VDAudioFilterDefinition	mDefinition;
	};

	class VDShadowedInputDriverDescription : public VDShadowedPluginDescription {
	public:
		virtual void Init(const VDPluginInfo *pInfo, VDExternalModule *pModule) {
			VDShadowedPluginDescription::Init(pInfo, pModule);

			const VDInputDriverDefinition *def = static_cast<const VDInputDriverDefinition *>(pInfo->mpTypeSpecificInfo);
			memset(&mDefinition, 0, sizeof mDefinition);
			memcpy(&mDefinition, def, std::min<uint32>(def->mSize, sizeof mDefinition));
			mShadowedInfo.mpTypeSpecificInfo = &mDefinition;

			if (def->mpSignature && def->mSignatureLength)
				mSignature.assign((const uint8 *)def->mpSignature, (const uint8 *)def->mpSignature + def->mSignatureLength);
			mDefinition.mpSignature = mSignature.data();

			if (def->mpFilenameDetectPattern) {
				mFilenameDetectPattern = def->mpFilenameDetectPattern;
				mDefinition.mpFilenameDetectPattern = mFilenameDetectPattern.c_str();
			}

			if (def->mpFilenamePattern) {
				mFilenamePattern = def->mpFilenamePattern;
				mDefinition.mpFilenamePattern = mFilenamePattern.c_str();
			}

			mDriverTagName = def->mpDriverTagName;
			mDefinition.mpDriverTagName = mDriverTagName.c_str();
		}

	protected:
		VDInputDriverDefinition	mDefinition;

		vdfastvector<uint8>		mSignature;
		VDStringW				mFilenameDetectPattern;
		VDStringW				mFilenamePattern;
		VDStringW				mDriverTagName;
	};
}



vdfastvector<VDShadowedPluginDescription *> g_plugins;

VDPluginDescription *VDGetPluginDescription(const wchar_t *pName, uint32 type) {
	for(vdfastvector<VDShadowedPluginDescription *>::iterator it(g_plugins.begin()), itEnd(g_plugins.end());
			it != itEnd; ++it)
	{
		VDPluginDescription& desc = **it;

		if (desc.mName == pName && desc.mType == type)
			return &desc;
	}

	return NULL;
}

void VDConnectPluginDescription(const VDPluginInfo *pInfo, VDExternalModule *pModule) {
	VDShadowedPluginDescription *pDesc = static_cast<VDShadowedPluginDescription *>(VDGetPluginDescription(pInfo->mpName, pInfo->mType));

	if (!pDesc) {
		switch(pInfo->mType) {
			case kVDPluginType_Video:
				pDesc = new VDShadowedVideoFilterDescription;
				break;

			case kVDPluginType_Audio:
				pDesc = new VDShadowedAudioFilterDescription;
				break;

			case kVDPluginType_Input:
				pDesc = new VDShadowedInputDriverDescription;
				break;
		}

		pDesc->AddRef();
		g_plugins.push_back(pDesc);

		pDesc->Init(pInfo, pModule);
	}

	pDesc->mpInfo = pInfo;
	pDesc->mpShadowedInfo = &pDesc->mShadowedInfo;
}

void VDConnectPluginDescriptions(const VDPluginInfo *const *ppInfos, VDExternalModule *pModule) {
	while(const VDPluginInfo *pInfo = *ppInfos++)
		VDConnectPluginDescription(pInfo, pModule);
}

void VDDisconnectPluginDescriptions(VDExternalModule *pModule) {
	for(vdfastvector<VDShadowedPluginDescription *>::iterator it(g_plugins.begin()), itEnd(g_plugins.end());
			it != itEnd; ++it)
	{
		VDShadowedPluginDescription& desc = **it;

		if (desc.mpModule == pModule)
			desc.mpInfo = NULL;
	}	
}

void VDEnumeratePluginDescriptions(std::vector<VDPluginDescription *>& plugins, uint32 type) {
	for(vdfastvector<VDShadowedPluginDescription *>::iterator it(g_plugins.begin()), itEnd(g_plugins.end());
			it != itEnd; ++it)
	{
		VDPluginDescription& desc = **it;

		if (desc.mType == type)
			plugins.push_back(&desc);
	}

}

std::list<class VDExternalModule *>		g_pluginModules;

VDExternalModule::VDExternalModule(const VDStringW& filename)
	: mFilename(filename)
	, mhModule(NULL)
	, mModuleRefCount(0)
{
	memset(&mModuleInfo, 0, sizeof mModuleInfo);
}

VDExternalModule::~VDExternalModule() {
}

void VDExternalModule::Lock() {
	if (!mhModule) {
		{
			VDExternalCodeBracket bracket(mFilename.c_str(), __FILE__, __LINE__);

			if (GetVersion() & 0x80000000)
				mhModule = LoadLibraryA(VDTextWToA(mFilename).c_str());
			else
				mhModule = LoadLibraryW(mFilename.c_str());
		}

		if (!mhModule)
			throw MyWin32Error("Cannot load plugin module \"%ls\": %%s", GetLastError(), mFilename.c_str());

		ReconnectOldPlugins();
		ReconnectPlugins();
	}
	++mModuleRefCount;
}

void VDExternalModule::Unlock() {
	VDASSERT(mModuleRefCount > 0);
	VDASSERT(mhModule);

	if (!--mModuleRefCount) {
		DisconnectOldPlugins();
		VDDisconnectPluginDescriptions(this);
		FreeLibrary(mhModule);
		mhModule = 0;
		VDDEBUG("Plugins: Unloading module \"%s\"\n", VDTextWToA(mFilename).c_str());
	}
}

void VDExternalModule::DisconnectOldPlugins() {
	if (mModuleInfo.hInstModule) {
		{
			VDExternalCodeBracket bracket(mFilename.c_str(), __FILE__, __LINE__);
			mModuleInfo.deinitProc(&mModuleInfo, &g_filterFuncs);
		}

		mModuleInfo.hInstModule = NULL;
	}
}

void VDExternalModule::ReconnectOldPlugins() {
	if (!mModuleInfo.hInstModule) {
		VDStringA nameA(VDTextWToA(mFilename));

		mModuleInfo.hInstModule = mhModule;

		try {
			mModuleInfo.initProc   = (FilterModuleInitProc  )GetProcAddress(mModuleInfo.hInstModule, "VirtualdubFilterModuleInit2");
			mModuleInfo.deinitProc = (FilterModuleDeinitProc)GetProcAddress(mModuleInfo.hInstModule, "VirtualdubFilterModuleDeinit");

			if (!mModuleInfo.initProc) {
				void *fp = GetProcAddress(mModuleInfo.hInstModule, "VirtualdubFilterModuleInit");

				if (fp)
					throw MyError(
						"This filter was created for VirtualDub 1.1 or earlier, and is not compatible with version 1.2 or later. "
						"Please contact the author for an updated version.");

				if (GetProcAddress(mModuleInfo.hInstModule, "VDGetPluginInfo")) {
					mModuleInfo.hInstModule = NULL;
					return;
				}
			}

			if (!mModuleInfo.initProc || !mModuleInfo.deinitProc)
				throw MyError("Module \"%s\" does not contain VirtualDub filters.", nameA.c_str());

			int ver_hi = VIRTUALDUB_FILTERDEF_VERSION;
			int ver_lo = VIRTUALDUB_FILTERDEF_COMPATIBLE;

			if (mModuleInfo.initProc(&mModuleInfo, &g_filterFuncs, ver_hi, ver_lo))
				throw MyError("Error initializing module \"%s\".",nameA.c_str());

			if (ver_hi < VIRTUALDUB_FILTERDEF_COMPATIBLE) {
				mModuleInfo.deinitProc(&mModuleInfo, &g_filterFuncs);

				throw MyError(
					"This filter was created for an earlier, incompatible filter interface. As a result, it will not "
					"run correctly with this version of VirtualDub. Please contact the author for an updated version.");
			}

			if (ver_lo > VIRTUALDUB_FILTERDEF_VERSION) {
				mModuleInfo.deinitProc(&mModuleInfo, &g_filterFuncs);

				throw MyError(
					"This filter uses too new of a filter interface!  You'll need to upgrade to a newer version of "
					"VirtualDub to use this filter."
					);
			}
		} catch(...) {
			mModuleInfo.hInstModule = NULL;
			throw;
		}
	}
}

void VDExternalModule::ReconnectPlugins() {
	tpVDXGetPluginInfo pVDGetPluginInfo = (tpVDXGetPluginInfo)GetProcAddress(mhModule, "VDGetPluginInfo");

	if (pVDGetPluginInfo) {
		const VDXPluginInfo *const *ppPluginInfo = pVDGetPluginInfo();

		VDConnectPluginDescriptions(ppPluginInfo, this);
	}
}

///////////////////////////////////////////////////////////////////////////

void VDDeinitPluginSystem() {
	while(!g_plugins.empty()) {
		VDShadowedPluginDescription *pDesc = g_plugins.back();
		g_plugins.pop_back();

		pDesc->Release();
	}

	while(!g_pluginModules.empty()) {
		VDExternalModule *pModule = g_pluginModules.back();
		delete pModule;					// must be before pop_back() (seems STL uses aliasing after all)
		g_pluginModules.pop_back();
	}
}

void VDAddPluginModule(const wchar_t *pFilename) {
	VDStringW path(VDGetFullPath(pFilename));

	if (path.empty())
		path = pFilename;

	std::list<class VDExternalModule *>::const_iterator it(g_pluginModules.begin()),
			itEnd(g_pluginModules.end());

	for(; it!=itEnd; ++it) {
		VDExternalModule *pModule = *it;

		if (pModule->GetFilename() == pFilename)
			return;
	}

	g_pluginModules.push_back(new VDExternalModule(path));
	VDExternalModule *pModule = g_pluginModules.back();

	// lock the module to bring in the plugin descriptions -- this may bomb
	// if the plugin doesn't exist or couldn't load

	try {
		pModule->Lock();
		pModule->Unlock();
	} catch(...) {
		g_pluginModules.pop_back();
		delete pModule;
		throw;
	}

}

void VDAddInternalPlugins(const VDPluginInfo *const *ppInfo) {
	VDConnectPluginDescriptions(ppInfo, NULL);
}

VDExternalModule *VDGetExternalModuleByFilterModule(const FilterModule *fm) {
	std::list<class VDExternalModule *>::const_iterator it(g_pluginModules.begin()),
			itEnd(g_pluginModules.end());

	for(; it!=itEnd; ++it) {
		VDExternalModule *pModule = *it;

		if (fm == &pModule->GetFilterModuleInfo())
			return pModule;
	}

	return NULL;
}

const VDPluginInfo *VDLockPlugin(VDPluginDescription *pDesc) {
	if (pDesc->mpModule)
		pDesc->mpModule->Lock();

	return pDesc->mpInfo;
}

void VDUnlockPlugin(VDPluginDescription *pDesc) {
	if (pDesc->mpModule)
		pDesc->mpModule->Unlock();
}

void VDLoadPlugins(const VDStringW& path, int& succeeded, int& failed) {
	static const wchar_t *const kExtensions[]={
		L"*.vdf",
		L"*.vdplugin"
	};

	succeeded = failed = 0;

	for(int i=0; i<sizeof(kExtensions)/sizeof(kExtensions[0]); ++i) {
		VDDirectoryIterator it(VDMakePath(path.c_str(), kExtensions[i]).c_str());

		while(it.Next()) {
			VDDEBUG("Plugins: Attempting to load \"%ls\"\n", it.GetFullPath().c_str());
			VDStringW path(it.GetFullPath());
			try {
				VDAddPluginModule(path.c_str());
				++succeeded;
			} catch(const MyError& e) {
				VDDEBUG("Plugins: Failed to load \"%ls\": %s\n", it.GetFullPath().c_str(), e.gets());
				++failed;
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////

VDPluginPtr::VDPluginPtr(VDPluginDescription *pDesc)
	: mpDesc(pDesc)
{
	VDLockPlugin(mpDesc);
}

VDPluginPtr::VDPluginPtr(const VDPluginPtr& src)
	: mpDesc(src.mpDesc)
{
	VDLockPlugin(src.mpDesc);
}

VDPluginPtr::~VDPluginPtr() {
	if (mpDesc)
		VDUnlockPlugin(mpDesc);
}

VDPluginPtr& VDPluginPtr::operator=(const VDPluginPtr& src) {
	if (mpDesc != src.mpDesc) {
		if (mpDesc)
			VDUnlockPlugin(mpDesc);
		mpDesc = src.mpDesc;
		if (mpDesc)
			VDLockPlugin(mpDesc);
	}
	return *this;
}

VDPluginPtr& VDPluginPtr::operator=(VDPluginDescription *pDesc) {
	if (mpDesc != pDesc) {
		if (mpDesc)
			VDUnlockPlugin(mpDesc);
		mpDesc = pDesc;
		if (mpDesc)
			VDLockPlugin(mpDesc);
	}
	return *this;
}

///////////////////////////////////////////////////////////////////////////

VDPluginConfigVariant::VDPluginConfigVariant(const VDPluginConfigVariant& x) : mType(x.mType), mData(x.mData) {
	switch(mType) {
	case kTypeAStr:
		mType = kTypeInvalid;
		SetAStr(x.mData.vsa.s);
		break;
	case kTypeWStr:
		mType = kTypeInvalid;
		SetWStr(x.mData.vsw.s);
		break;
	case kTypeBlock:
		mType = kTypeInvalid;
		SetBlock(x.mData.vb.s, x.mData.vb.len);
		break;
	}
}

VDPluginConfigVariant::~VDPluginConfigVariant() {
	Clear();
}

VDPluginConfigVariant& VDPluginConfigVariant::operator=(const VDPluginConfigVariant& x) {
	if (this != &x) {
		switch(mType = x.mType) {
		case kTypeAStr:
		case kTypeWStr:
		case kTypeBlock:
			this->~VDPluginConfigVariant();
			new(this) VDPluginConfigVariant(x);
			break;
		default:
			mData = x.mData;
			break;
		}
	}
	return *this;
}

void VDPluginConfigVariant::Clear() {
	switch(mType) {
	case kTypeAStr:
		delete[] mData.vsa.s;
		break;
	case kTypeWStr:
		delete[] mData.vsw.s;
		break;
	case kTypeBlock:
		delete[] mData.vb.s;
		break;
	}

	mType = kTypeInvalid;
}

void VDPluginConfigVariant::SetAStr(const char *s) {
	Clear();
	mType = kTypeAStr;

	size_t l = strlen(s);
	mData.vsa.s = new char[l+1];
	memcpy(mData.vsa.s, s, sizeof(char) * (l+1));
}

void VDPluginConfigVariant::SetWStr(const wchar_t *s) {
	Clear();
	mType = kTypeWStr;

	size_t l = wcslen(s);
	mData.vsw.s = new wchar_t[l+1];
	memcpy(mData.vsw.s, s, sizeof(wchar_t) * (l+1));
}

void VDPluginConfigVariant::SetBlock(const void *s, unsigned b) {
	Clear();
	mType = kTypeBlock;

	mData.vb.s = (char *)malloc(b);
	mData.vb.len = b;
	memcpy(mData.vb.s, s, b);
}
