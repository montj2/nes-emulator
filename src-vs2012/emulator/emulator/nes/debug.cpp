#include "../stdafx.h"

// local header files
#include "../macros.h"
#include "../types/types.h"

#include "internals.h"
#include "debug.h"
#include "opcodes.h"
#include "mmc.h"

static FILE* foutput = stdout;

namespace debug
{
	void printDisassembly(const maddr_t pc, const opcode_t opcode, const _reg8_t rx, const _reg8_t ry, const maddr_t addr, const operand_t operand)
	{
		const M6502_OPCODE op = opcode::decode(opcode);
		switch (op.size)
		{
		case 1:
			fprintf(foutput, "%04X  %02X        %s", valueOf(pc), opcode, opcode::instName(op.inst));
			break;
		case 2:
			fprintf(foutput, "%04X  %02X %02X     %s", valueOf(pc), opcode, ram.data(pc+1), opcode::instName(op.inst));
			break;
		case 3:
			fprintf(foutput, "%04X  %02X %02X %02X  %s", valueOf(pc), opcode, ram.data(pc+1), ram.data(pc+2), opcode::instName(op.inst));
			break;
		}
		switch (op.addrmode)
		{
		case ADR_IMP:
			break;
		case ADR_ZP:
		case ADR_ZPX:
		case ADR_ZPY:
			fprintf(foutput, " $%02X = %02X", valueOf(addr), operand);
			break;
		case ADR_REL:
			fprintf(foutput, " to $%04X", valueOf(addr));
			break;
		case ADR_ABS:
		case ADR_ABSX:
		case ADR_ABSY:
		case ADR_INDX:
		case ADR_INDY:
		case ADR_IND:
			fprintf(foutput, " $%04X = %02X", valueOf(addr), operand);
			break;
		case ADR_IMM:
			fprintf(foutput, " #$%02X", operand);
			break;
		}
		fprintf(foutput, "\n");
	}

	void printCPUState(const maddr_t pc, const _reg8_t ra, const _reg8_t rx, const _reg8_t ry, const _reg8_t rp, const _reg8_t rsp, const int cyc)
	{
		fprintf(foutput, "[A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%3d] -> %04X\n", ra, rx, ry, rp, rsp, cyc, valueOf(pc));
	}

	// a NULL at the end of argv is REQUIRED!
	static void printToConsole(int type, const wchar_t * typestr, int stype, const wchar_t * stypestr, const wchar_t * file, const wchar_t * function_name, unsigned long line_number, va_list argv)
	{
		wprintf(L"Type: %s (%d)\nSub Type: %s (%d)\nProc: %s:%ld\n", typestr, type, stypestr, stype, function_name, line_number);
		if (file != nullptr)
		{
			wprintf(L"File: %s\n", file);
		}

		// print custom parameters
		char* name=nullptr;
		while ((name=va_arg(argv, char*))!=nullptr)
		{
			int value=va_arg(argv, int);
			printf("<%s> = %Xh (%d)\n", name, value, value);
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
			CASE_ENUM_RETURN_STRING(MEMORY_CANT_BE_COPIED);

			CASE_ENUM_RETURN_STRING(INVALID_OPCODE);
			CASE_ENUM_RETURN_STRING(INVALID_ADDRESS_MODE);

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
		fflush(foutput);
#ifndef NDEBUG
		assert(0);
#endif
		exit(type);
	}

	void error(EMUERROR type, EMUERRORSUBTYPE stype, const wchar_t * file, const wchar_t * function_name, unsigned long line_number, ...)
	{
		va_list args;
		va_start(args, line_number);
		wprintf(L"[X] Error: \n");
		printToConsole(type, errorTypeToString(type), stype, errorSTypeToString(stype), file, function_name, line_number, args);
		va_end(args);
		fflush(foutput);
#ifndef NDEBUG
		assert(0);
#else
		__debugbreak();
#endif
	}

	void warn(EMUERROR type, EMUERRORSUBTYPE stype, const wchar_t * function_name, unsigned long line_number, ...)
	{
		va_list args;
		va_start(args, line_number);
		wprintf(L"[!] Warning: \n");
		printToConsole(type, errorTypeToString(type), stype, errorSTypeToString(stype), NULL, function_name, line_number, args);
		va_end(args);
	}

	void setOutputFile(FILE *fp)
	{
		foutput = fp;
	}
}