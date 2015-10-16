#include "../stdafx.h"

// local header files
#include "../macros.h"
#include "../types/types.h"
#include "../unittest/framework.h"

#include "internals.h"
#include "debug.h"
#include "mmc.h"

// NES main memory
__declspec(align(64))
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
				break;
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

	opcode_t fetchOpcode(const maddr_t pc)
	{
#ifdef WANT_MEM_PROTECTION
		if (!MSB(pc)) // address out of range [$8000, $FFFF]
		{
			FATAL_ERROR(INVALID_MEMORY_ACCESS, MEMORY_NON_EXECUTABLE);
		}
#endif
		return ram.bank8[pc-0x8000];
	}

	byte_t fetchByteOperand(const maddr_t pc)
	{
		return 0;
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