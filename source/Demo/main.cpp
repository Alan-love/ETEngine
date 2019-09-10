#include "stdafx.h"

#include "MainFramework.h"

#include <iostream>

#include <EtCore/FileSystem/FileUtil.h>

#include <Engine/linkerHelper.h>

#include <Demo/_generated/compiled_package.h>


void SetDebuggingOptions();

//---------------------------------
// main
//
// Entry point into the framework. 
//
//#if defined(PLATFORM_Win)
//int WinMain(int argc, char *argv[])
//#else
int main(int argc, char *argv[])
//#endif
{
	// set up environment
	//-------------------------
	SetDebuggingOptions();

	ForceLinking(); // makes sure the linker doesn't ignore reflection only data

	// pass compiled data into core libraries so that core systems have access to it
	FileUtil::SetCompiledData(GetCompiledData_compiled_package());

	// working dir
	if (argc > 0)
	{
		// all engine files are seen as relative to the executable path by default, so it needs to be set
		FileUtil::SetExecutablePath(argv[0]);
	}
	else
	{
		std::cerr << "main > Couldn't get extract working directory from arguments, exiting!" << std::endl;
		return 1;
	}

	// run the actual framework
	//--------------------------
	MainFramework* pFW = new MainFramework();
	pFW->Run();

	delete pFW;
	return 0;
}

//---------------------------------
// SetDebuggingOptions
//
// On debug builds this will tell us about memory leaks
//
void SetDebuggingOptions()
{
	//notify user if heap is corrupt
	HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

	// Enable run-time memory leak check for debug builds.
#if defined(ET_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	typedef HRESULT(__stdcall *fPtr)(const IID&, void**);

	//_CrtSetBreakAlloc(106750);
#endif
}