#include "../stdafx.h"

// local header files
#include "../macros.h"
#include "../types/types.h"

#include "internals.h"
#include "debug.h"

namespace debug
{
	// a NULL at the end of argv is REQUIRED!
	static void printToConsole(int type, const wchar_t * typestr, int stype, const wchar_t * stypestr, const wchar_t * file, const wchar_t * function_name, unsigned long line_number, va_list argv)
	{
		wprintf(L"Type: %s(%d)\nSub Type: %s(%d)\nLocation: %s:%ld in %s\n", typestr, type, stypestr, stype, function_name, line_number, file);

		// print custom parameters
		char* name=nullptr;
		while ((name=va_arg(argv, char*))!=nullptr)
		{
			int value=va_arg(argv, int);
			printf("<%s> = %d (hex: 0x%x)\n", name, value, value);
		}
	}

	static const wchar_t * errorTypeToString(EMUERROR type)
	{
		switch (type)
		{
			CASE_ENUM_RETURN_STRING(INVALID_ROM);
			CASE_ENUM_RETURN_STRING(INVALID_MEMORY_ACCESS);
			CASE_ENUM_RETURN_STRING(INVALID_INSTRUCTION);
			CASE_ENUM_RETURN_STRING(ILLEGAL_OPERATION);

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
			CASE_ENUM_RETURN_STRING(MEMORY_NOT_EXECUTABLE);
			CASE_ENUM_RETURN_STRING(MEMORY_CANT_BE_READ);
			CASE_ENUM_RETURN_STRING(MEMORY_CANT_BE_WRITTEN);

			CASE_ENUM_RETURN_STRING(INVALID_OPCODE);

			CASE_ENUM_RETURN_STRING(IRQ_ALREADY_PENDING);

		default: return L"UNKNOWN";
		}
	}

	void fatalError(EMUERROR type, EMUERRORSUBTYPE stype, const wchar_t * file, const wchar_t * function_name, unsigned long line_number, ...)
	{
		va_list args;
		va_start(args, line_number);
		wprintf(L"[X] Fatal error: \n");
		printToConsole(type, errorTypeToString(type), stype, errorSTypeToString(stype), file, function_name, line_number, args);
		va_end(args);
		exit(type);
	}

	void error(EMUERROR type, EMUERRORSUBTYPE stype, const wchar_t * file, const wchar_t * function_name, unsigned long line_number, ...)
	{
		va_list args;
		va_start(args, line_number);
		wprintf(L"[X] Error: \n");
		printToConsole(type, errorTypeToString(type), stype, errorSTypeToString(stype), file, function_name, line_number, args);
		va_end(args);
		__debugbreak();
	}
}