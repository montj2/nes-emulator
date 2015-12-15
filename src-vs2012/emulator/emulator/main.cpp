// emulator.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

// local header files
#include "macros.h"
#include "types/types.h"
#include "unittest/framework.h"

#include "nes/internals.h"
#include "nes/debug.h"
#include "nes/emu.h"

#include "ui.h"

static void welcome()
{
	puts("Portable NES Emulator 1.0"); 
}

static void usage(_TCHAR* self_path)
{
	// _tprintf(_T("%s <nes file path>\n"), self_path);
}


int _cdecl _tmain(int argc, _TCHAR* argv[])
{
	welcome();
	usage(argv[0]);
	TestFramework::instance().runAll();
	emu::init();
	if (argc>=2)
	{
		if (emu::load(argv[1]))
		{
			// setup modules
			emu::setup();

			// create log file
			FILE *fp = fopen("m:\\log.txt", "wt");
			debug::setOutputFile(fp);

			// start execution
			emu::nextFrame();

			fclose(fp);
		}else
		{
			puts("[X] Unable to load the rom.");
		}
	}else
	{
		puts("[!] No rom file specified.");
	}
	emu::deinit();
	TestFramework::destroy();
	return 0;
}

