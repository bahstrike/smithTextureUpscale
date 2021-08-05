#include "smith.h"
#include "xbr.h"

extern "C"
{
	SMITHCALLS* smith = nullptr;

	int __cdecl SmithQueryPlugin(PLUGININFO& p)
	{
		strcpy(p.author, "bahstrike");
		strcpy(p.desc, "Provides texture upscaling support");
		p.smithRequiredVer = SMITHVERSION;
		p.ver = 100;

		return 1337;
	}

	int __cdecl InitializePlugin(SMITHCALLS* _smith)
	{
		smith = _smith;

		//MessageBox(0, "xbr init", 0, 0);
		Init();


		return true;
	}

	void __cdecl ShutdownPlugin()
	{
		Shutdown();

		smith = nullptr;
	}



}