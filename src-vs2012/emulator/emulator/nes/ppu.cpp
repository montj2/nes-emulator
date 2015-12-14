#include "../stdafx.h"

// local header files
#include "../macros.h"
#include "../types/types.h"
#include "../unittest/framework.h"

#include "internals.h"
#include "debug.h"
#include "mmc.h"
#include "cpu.h"
#include "ppu.h"

// PPU memory
struct NESVRAM vram;
struct NESOAM oam;

// unit tests
class PPUMemTest : public TestCase
{
public:
	virtual const char* name()
	{
		return "PPU Memory Unit Test";
	}

	virtual TestResult run()
	{
		puts("checking VRAM struture...");
		tassert(sizeof(vram)==0x4000);
		tassert(sizeof(vram.vrom.patternTables)==0x2000);
		tassert(ptr_diff(&vramAt(0).attribs[0],&vram.data(0))==0x23C0);
		tassert(ptr_diff(&vramNt(1).tiles[0][0],&vram.data(0))==0x2400);
		tassert(ptr_diff(vram.palettes(),&vram.data(0))==0x3F00);
		tassert(sizeof(oam)==0x100);

		printf("[ ] VRAM at 0x%p\n",&vram);
		return SUCCESS;
	}
};

registerTestCase(PPUMemTest);