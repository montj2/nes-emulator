#include "../stdafx.h"

// local header files
#include "../macros.h"
#include "../types/types.h"

#include "internals.h"
#include "debug.h"

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
			CASE_ENUM_RETURN_STRING(MEMORY_NOT_EXECUTABLE);
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