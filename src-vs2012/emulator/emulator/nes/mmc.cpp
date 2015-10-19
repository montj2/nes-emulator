#include "../stdafx.h"

// local header files
#include "../macros.h"
#include "../types/types.h"
#include "../unittest/framework.h"

#include "internals.h"
#include "debug.h"
#include "mmc.h"

// NES main memory
__declspec(align(0x1000))
struct NESRAM ram;

// addresses of currently selected prg-rom banks.
static int p8,pA,pC,pE;

namespace mmc
{
	// perform bank switching
	void bankSwitch(int reg8, int regA, int regC, int regE, const uint8_t* image)
	{
		if (p8!=reg8) memcpy(ram.bank8, &image[reg8*0x2000], 0x2000);
		if (pA!=regA) memcpy(ram.bankA, &image[regA*0x2000], 0x2000);
		if (pC!=regC) memcpy(ram.bankC, &image[regC*0x2000], 0x2000);
		if (pE!=regE) memcpy(ram.bankE, &image[regE*0x2000], 0x2000);
		// update current selection
		p8=reg8;pA=regA;pC=regC;pE=regE;
	}

	bool setup(int mapper_type, const uint8_t* image, const size_t image_size)
	{
		switch (mapper_type)
		{
		case 0: // no mapper
			if (image_size == 0x8000) // 32K of code
				bankSwitch(0, 1, 2, 3, image);
			else if (image_size == 0x4000) // 16K of code
				bankSwitch(0, 1, 0, 1, image);
			else
			{
				ERROR(INVALID_MEMORY_ACCESS, MAPPER_FAILURE);
				return false;
			}
			return true;
		}
		// unknown mapper
		FATAL_ERROR(INVALID_ROM, UNSUPPORTED_MAPPER_TYPE);
		return false;
	}

	void reset()
	{
		// no prg-rom selected now
		p8=pA=pC=pE=-1;

		// flush memory
		memset(&ram,0,sizeof(ram));
	}

	opcode_t fetchOpcode(maddr_t& pc)
	{
		opcode_t opcode;
#ifdef WANT_MEM_PROTECTION
		// check if address in code section [$8000, $FFFF]
		FATAL_ERROR_UNLESS(MSB(pc), INVALID_MEMORY_ACCESS, MEMORY_NOT_EXECUTABLE, "PC", valueOf(pc));
		opcode = ram.bank8[pc^0x8000];
#else
		WARN_IF(!MSB(pc), INVALID_MEMORY_ACCESS, MEMORY_NOT_EXECUTABLE, "PC", valueOf(pc));
		opcode = ram.data(pc);
#endif
		inc(pc);
		return opcode;
	}

	operandb_t fetchByteOperand(maddr_t& pc)
	{
		operandb_t operand;
#ifdef WANT_MEM_PROTECTION
		operand(ram.bank8[pc^0x8000]);
#else
		operand(ram.data(pc));
#endif
		inc(pc);
		return operand;
	}

	operandw_t fetchWordOperand(maddr_t& pc)
	{
		operandw_t operand;
		FATAL_ERROR_IF(pc.reachMax(), INVALID_MEMORY_ACCESS, ILLEGAL_ADDRESS_WARP);
#ifdef WANT_MEM_PROTECTION
		operand(*(uint16_t*)&ram.bank8[pc^0x8000]);
#else
		operand(makeWord(ram.data(pc), ram.data(pc+1)));
#endif
		pc+=2;
		return operand;
	}

	byte_t loadZPByte(const maddr8_t zp)
	{
		return ram0p[zp];
	}

	word_t loadZPWord(const maddr8_t zp)
	{
#ifndef ALLOW_ADDRESS_WRAP
		FATAL_ERROR_IF(zp.reachMax(), INVALID_MEMORY_ACCESS, ILLEGAL_ADDRESS_WARP);
#else
		if (zp.reachMax())
		{
			 return (((word_t)ram0p[0])<<8)|ram0p[zp];
		}
#endif
		return *(uint16_t*)&ram0p[zp];
	}

	byte_t read(const maddr_t addr)
	{
		switch (addr>>13) // bank number/2
		{
		case 0: //[$0000,$2000)
			return ram.bank0[addr&0x7FF];
		case 1: //[$2000,$4000)
			// TODO: read ppu register
			break;
		case 2: //[$4000,$6000)
			// TODO: read IO register
			break;
		case 3:
			return ram.bank6[addr&0x1FFFF];
		case 4:
		case 5:
		case 6:
		case 7:
			return ram.bank8[addr^0x8000];
		}
		ERROR(INVALID_MEMORY_ACCESS, MEMORY_CANT_BE_READ, "addr", valueOf(addr));
		return ~0;
	}

	void write(const maddr_t addr, const byte_t value)
	{
		switch (addr>>13) // bank number/2
		{
			case 0: //[$0000,$2000) Internal RAM
				ram.bank0[addr&0x7FF]=value;
				return;
			case 1: //[$2000,$4000) PPU Registers
				// write to ppu register
				break;
			case 3: //[$6000,$8000) SRAM
				ram.bank6[addr&0x1FFF]=value;
				return;
			case 4: //[$8000,$A000)
			case 5: //[$A000,$C000)
			case 6: //[$C000,$E000)
			case 7: //[$E000,$FFFF]
				break;
			case 2: //[$4000,$6000) Other Registers
				break;
		}
		ERROR(INVALID_MEMORY_ACCESS, MEMORY_CANT_BE_WRITTEN, "addr", valueOf(addr));
	}
}

// unit tests
class MMCTest : public TestCase
{
public:
	virtual const char* name()
	{
		return "MMC Unit Test";
	}

	virtual TestResult run()
	{
		puts("checking RAM struture...");
		tassert(sizeof(ram)==0x10000);
		tassert(&ram.bank6[0]-&ram.data(0)==0x6000);
		tassert(ram.page(0x20)-&ram.data(0)==0x2000);

		printf("[ ] System memory at 0x%p\n", &ram);
		return SUCCESS;
	}
};

registerTestCase(MMCTest);