// emulator.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

// local header files
#include "macros.h"
#include "types/types.h"
#include "unittest/framework.h"

#include "nes/internals.h"
#include "nes/rom.h"
#include "nes/opcodes.h"
#include "nes/mmc.h"
#include "nes/cpu.h"

static void welcome()
{
	puts("Portable NES Emulator 1.0"); 
}

static void usage(_TCHAR* self_path)
{
	// _tprintf(_T("%s <nes file path>\n"), self_path);
}

static void init()
{
	opcode::initTable();
}

static void deinit()
{
	rom::unload();
}

int _cdecl _tmain(int argc, _TCHAR* argv[])
{
	welcome();
	usage(argv[0]);
	TestFramework::instance().runAll();
	init();
	if (argc>=2)
	{
		if (rom::load(argv[1]))
		{
			// setup mmc
			mmc::reset();
			mmc::setup(rom::mapperType(), (const uint8_t*)rom::getImage(), rom::sizeOfImage());

			// setup cpu
			cpu::reset();

			// start execution
			for (int i=1;i<=100;i++)
			{
				cpu::nextInstruction();
			}
		}else
		{
			puts("[X] Unable to load the rom.");
		}
	}else
	{
		puts("[!] No rom file specified.");
	}
	deinit();
	TestFramework::destroy();
	return 0;
}

