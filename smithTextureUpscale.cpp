#include "smith.h"
#include "xbr.h"

extern "C"
{
	SMITHCALLS* smith = nullptr;

	int __cdecl SmithQueryPlugin(PLUGININFO& p)
	{
		strcpy(p.name, "Texture Upscale");
		strcpy(p.author, "bahstrike");
		strcpy(p.authorEmail, "strike@bah.wtf");
		strcpy(p.attributions, "Hyllian/Jararaca&Copyright 2011, 2012 Hyllian/Jararaca (sergiogdb@gmail.com)|Arwa Arif&Copyright 2014 Arwa Arif (arwaarif1994@gmail.com)|Treeki&Copyright 2015 Treeki (treeki@gmail.com)");
		strcpy(p.desc, "Provides texture upscaling support using xBR algorithm in 3x scaling mode.");
		strcpy(p.homepageURL, "https://github.com/bahstrike/smithTextureUpscale");
		p.smithRequiredVer = SMITHVERSION;
		p.ver = 100;
		p.purpose = PP_TEXTUREUPSCALE;

		return 1337;
	}

	int __cdecl InitializePlugin(SMITHCALLS* _smith)
	{
		smith = _smith;

		Init();


		return true;
	}

	void __cdecl ShutdownPlugin()
	{
		Shutdown();

		smith = nullptr;
	}


	int __cdecl OnProcessTexture(void* input, int inWidth, int inHeight, int inStride, void* output, int outStride)
	{
		if (smith == nullptr)
			return false;

		Process(input, inWidth, inHeight, inStride, output, outStride);

		return true;
	}


}