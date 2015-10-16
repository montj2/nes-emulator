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

namespace debug
{
	static void printToConsole(int type, const wchar_t * typestr, int stype, const wchar_t * stypestr, const wchar_t * file, const wchar_t * function, unsigned long lineNumber)
	{
		wprintf(L"Type: %s(%d)\nSub Type: %s(%d)\nLocation: %s:%ld in %s\n", typestr, type, stypestr, stype, function, lineNumber, file);
	}

	static const wchar_t * errorTypeToString(EMUERROR type)
	{
		switch (type)
		{
			CASE_ENUM_RETURN_STRING(INVALID_ROM);
			CASE_ENUM_RETURN_STRING(INVALID_MEMORY_ACCESS);
			CASE_ENUM_RETURN_STRING(INVALID_INSTRUCTION);

		default: return L"UNKNOWN";
		}
	}

	static const wchar_t * errorSTypeToString(EMUERRORSUBTYPE stype)
	{
		switch (stype)
		{
			CASE_ENUM_RETURN_STRING(INVALID_FILE_SIGNATURE);
			CASE_ENUM_RETURN_STRING(INVALID_ROM_CONFIG);
			CASE_ENUM_RETURN_STRING(UNEXPECTED_END_OF_FILE);
			CASE_ENUM_RETURN_STRING(UNSUPPORTED_MAPPER_TYPE);

			CASE_ENUM_RETURN_STRING(MAPPER_FAILURE);
			CASE_ENUM_RETURN_STRING(ADDRESS_OUT_OF_RANGE);
			CASE_ENUM_RETURN_STRING(ILLEGAL_ADDRESS_WARP);
			CASE_ENUM_RETURN_STRING(MEMORY_NON_EXECUTABLE);
			CASE_ENUM_RETURN_STRING(MEMORY_READ_ONLY);
			CASE_ENUM_RETURN_STRING(MEMORY_WRITE_ONLY);

		default: return L"UNKNOWN";
		}
	}

	void fatalError(EMUERROR type, EMUERRORSUBTYPE stype, const wchar_t * file, const wchar_t * function, unsigned long line_number)
	{
		wprintf(L"[X] Fatal error: \n");
		printToConsole(type, errorTypeToString(type), stype, errorSTypeToString(stype), file, function, line_number); 
		exit(type);
	}

	void error(EMUERROR type, EMUERRORSUBTYPE stype, const wchar_t * file, const wchar_t * function, unsigned long line_number)
	{
		wprintf(L"[X] Error: \n");
		printToConsole(type, errorTypeToString(type), stype, errorSTypeToString(stype), file, function, line_number); 
		__debugbreak();
	}
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
			// start execution()
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

