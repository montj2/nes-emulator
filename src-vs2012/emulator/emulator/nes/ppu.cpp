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

// PPU Memory
struct NESVRAM vram;
struct NESOAM oam;

// PPU Control & Status Registers
static flag_set<_reg8_t, PPUCTRL, 8> control; // $2000
static flag_set<_reg8_t, PPUMASK, 8> mask; // $2001
static flag_set<_reg8_t, PPUSTATUS, 8> status; // $2002
#define control1 control
#define control2 mask

// PPU SPR-RAM Access Registers
static saddr_t oamAddr; // $2003
static byte_t oamData; // $2004

// PPU VRAM Access Registers
static flag_set<_addr15_t, PPUADDR, 15> scroll; // $2005 Background Scrolling Offset
static flag_set<_addr14_t, PPUADDR, 14> address; // $2006 VRAM Address Register
static byte_t latch; // the LATCH of $2007 Read/Write Data Register
#define address1 scroll
#define address2 address

// PPU State
static bool firstWrite; // shared for both port $2005 and $2006

static int scanline;
static long long frameNum;

namespace ppu
{
	void reset()
	{
		// reset all registers
		control1.clearAll();
		control2.clearAll();
		status.clearAll();

		address1.clearAll();
		address2.clearAll();
		latch = 0xFF;

		// reset state
		firstWrite = true;
		scanline = 0;
		frameNum = 0;

		// flush memory
		memset(&vram,0,sizeof(vram));
		memset(&oam,0,sizeof(oam));
	}
}

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