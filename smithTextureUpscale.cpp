#include "smith.h"

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

		MessageBox(0, 0, 0, 0);

		return true;
	}

	void __cdecl ShutdownPlugin()
	{

		smith = nullptr;
	}



// once the session is over, smith will call ShutdownPlugin to allow you to gracefully tie up any loose ends.
// generally, this will be followed by actually unloading this plugin DLL; however that's not gauranteed.
// if smith supports a more dynamic plugin load/unload system then its possible sessions can be started/stopped
// multiple times without the plugin being unloaded. so bear that in mind:  plz do not leak.
// 
// long story short;  kill any possible tasks you have that may either call smith API, or expect to be called by smith API.
// forget the SMITHCALLS interface; its not gauranteed to be valid anymore.
// the only functions smith will call after this point is either SmithQueryPlugin, or InitializePlugin again.
//void __cdecl ShutdownPlugin()

}