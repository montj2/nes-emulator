#include "../stdafx.h"

// local header files
#include "../macros.h"
#include "../types/types.h"
#include "../unittest/framework.h"

#include "internals.h"
#include "debug.h"
#include "rom.h"
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
typedef flag_set<_addr15_t, PPUADDR, 15> scroll_flag_t;
typedef flag_set<_addr14_t, PPUADDR, 14> vaddr_flag_t;
static scroll_flag_t scroll; // $2005 Background Scrolling Offset
static vaddr_flag_t address; // $2006 VRAM Address Register / Scrolling Pointer
static vaddr_t tmpAddress;
static byte_t latch; // the LATCH of $2007 Read/Write Data Register
#define address1 address
#define address2 scroll
#define scrollptr address
#define scrollrld scroll

// PPU State
static bool firstWrite; // shared for both port $2005 and $2006

static int scanline;
static long long frameNum;

namespace mem
{
	static vaddr_t ntMirror(vaddr_flag_t vaddr)
	{
		vassert(vaddr.select(PPUADDR::BANK)==2);
		switch (rom::mirrorMode())
		{
		case MIRROR_HORIZONTAL:
			vaddr-=PPUADDR::HNT;
			break;
		case MIRROR_VERTICAL:
			vaddr-=PPUADDR::VNT;
			break;
		case MIRROR_SINGLESCREEN:
			vaddr-=PPUADDR::NT;
			break;
		case MIRROR_FOURSCREEN:
			// do nothing here
			break;
		}
		return vaddr; // auto conversion works?
	}

	static vaddr_t mirror(vaddr_flag_t vaddr, bool forRead=true)
	{
		switch (vaddr.select(PPUADDR::BANK)) {
		case 0:
		case 1: // [$0,$2000)
			// no mirroring
			break;
		case 2: // [$2000,$3000)
			mirror2000:
				vaddr=ntMirror(vaddr);
			break;
		case 3: // [$3000,$4000)
			if (vaddr.select(PPUADDR::BANK_OFFSET)<0xF00) {
				vaddr.update<PPUADDR::BANK>(2);
				goto mirror2000;
			}
			// [$3F00,$3FFF]
			vaddr.asBitField()&=0x3F1F;
			if (vaddr.select(PPUADDR::PAL_ITEM)==0)
			{
				if (vaddr[PPUADDR::PAL_SELECT])
				{
					// sprite palette is selected
					// mirror $3F10, $3F14, $3F18, $3F1C to $3F00, $3F04, $3F08, $3F0C
					vaddr-=PPUADDR::PAL_SELECT;
				}
				/*else if (forRead)
				{
					// background palette is selected
					// $3F04, $3F08, $3F0C can contain unique data,
					// but PPU uses palette index 0 instead when rendering
					vaddr-=PPUADDR::PAL_NUM;
				}*/
			}
			break;
		}
		return vaddr;
	}

	static void incAddress()
	{
		address.asBitField()+=control[PPUCTRL::VERTICAL_WRITE]?32:1;
	}

	static void setAddress(const byte_t byte)
	{


	}

	static byte_t read()
	{
		const vaddr_t addr=mirror(address);
		incAddress();
		if (addr<0x3F00)
		{
			// return buffered data
			const byte_t oldLatch=latch;
			latch=vram.data(addr);
			return oldLatch;
		}
		// no buffering for palette memory access
		return vram.data(addr);
	}

	static void write(const byte_t data)
	{
		assert(!status[PPUSTATUS::WRITEIGNORED]);
		const vaddr_t addr=mirror(address);
		{
			// ?
			firstWrite=true;
			latch=data;
		}
		vram.data(addr)=data;
		incAddress();
	}
}

namespace render
{
	void setScroll(const byte_t byte)
	{
	}
}

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
		tmpAddress = 0;

		// reset state
		firstWrite = true;
		latch = INVALID;
		scanline = 0;
		frameNum = 0;

		// clear memory
		memset(&vram,0,sizeof(vram));
		memset(&oam,0,sizeof(oam));
	}

	bool readPort(const maddr_t maddress, byte_t& data)
	{
		data=INVALID;
		vassert(1==valueOf(maddress)>>13); // [$2000,$4000)
		switch (valueOf(maddress)&7)
		{
		case 2: // $2002 PPU Status Register
			data=valueOf(status);
			status.clear(PPUSTATUS::VBLANK);
			firstWrite=true;
			latch=INVALID;
			return true;
		case 0: // $2000 PPU Control Register 1
		case 1: // $2001 PPU Control Register 2
		case 3: // $2003 Sprite RAM address
		case 5: // $2005 Screen Scroll offsets
		case 6: // $2006 VRAM address
			break; // The above are write-only registers.
		case 4: // $2004 Sprite Memory Read
			data=oamData(oamAddr);
			return true;
		case 7: // $2007 VRAM read
			data=mem::read();
			return true;
		}
		return false;
	}

	bool writePort(const maddr_t maddress, const byte_t data)
	{
		vassert(1==valueOf(maddress)>>13); // [$2000,$4000)
		switch (valueOf(maddress)&7)
		{
		case 0: // $2000 PPU Control Register 1
			control1.asBitField()=data;
			return true;
		case 1: // $2001 PPU Control Register 2
			control2.asBitField()=data;
			return true;
		case 3: // $2003 Sprite RAM address
			oamAddr=data;
			return true;
		case 4: // $2004 Sprite Memory Data
			oamData(oamAddr)=data;
			inc(oamAddr);
			return true;
		case 5: // $2005 Screen Scroll offsets
			render::setScroll(data);
			return true;
		case 6: // $2006 VRAM address
			mem::setAddress(data);
			return true;
		case 7: // $2007 VRAM write
			mem::write(data);
			return true;
		case 2: // $2002 PPU Status Register
			break;
		}
		return false;
	}

	void hsync()
	{
	}

	int currentScanline()
	{
		return scanline;
	}

	long long currentFrame()
	{
		return frameNum;
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
		tassert(ptr_diff(&vram.pal,&vram.data(0))==0x3F00);
		tassert(sizeof(oam)==0x100);

		printf("[ ] VRAM at 0x%p\n",&vram);
		return SUCCESS;
	}
};

class PPUMirroringTest : public TestCase
{
public:
	virtual const char* name()
	{
		return "PPU Mirroring Test";
	}

	virtual TestResult run()
	{
		for (int i=MIRROR_MIN;i<=MIRROR_MAX;i++)
		{
			rom::setMirrorMode((MIRRORING)i);
			tassert(mem::mirror(vaddr_t(0x1395))==0x1395); // no mapping should occur
		}

		// test nametable mirroring
		rom::setMirrorMode(MIRROR_HORIZONTAL);
		/*
		this.defineMirrorRegion(0x2400,0x2000,0x400);
		this.defineMirrorRegion(0x2c00,0x2800,0x400);
		*/
		tassert(mem::mirror(vaddr_t(0x2011))==0x2011);
		tassert(mem::mirror(vaddr_t(0x22FF))==0x22FF);

		tassert(mem::mirror(vaddr_t(0x2409))==0x2009);
		tassert(mem::mirror(vaddr_t(0x2409))==0x2009);

		tassert(mem::mirror(vaddr_t(0x2871))==0x2871);
		tassert(mem::mirror(vaddr_t(0x2AF1))==0x2AF1);

		tassert(mem::mirror(vaddr_t(0x2D22))==0x2922);

		rom::setMirrorMode(MIRROR_VERTICAL);
		/*
		this.defineMirrorRegion(0x2800,0x2000,0x400);
		this.defineMirrorRegion(0x2c00,0x2400,0x400);
		*/
		tassert(mem::mirror(vaddr_t(0x2011))==0x2011);
		tassert(mem::mirror(vaddr_t(0x22FF))==0x22FF);
		tassert(mem::mirror(vaddr_t(0x2405))==0x2405);
		tassert(mem::mirror(vaddr_t(0x2677))==0x2677);

		tassert(mem::mirror(vaddr_t(0x28A3))==0x20A3);
		tassert(mem::mirror(vaddr_t(0x2FFF))==0x27FF);

		rom::setMirrorMode(MIRROR_SINGLESCREEN);
		/*
		this.defineMirrorRegion(0x2400,0x2000,0x400);
		this.defineMirrorRegion(0x2800,0x2000,0x400);
		this.defineMirrorRegion(0x2c00,0x2000,0x400);
		*/
		tassert(mem::mirror(vaddr_t(0x2D70))==0x2170);

		rom::setMirrorMode(MIRROR_FOURSCREEN);
		tassert(mem::mirror(vaddr_t(0x2FED))==0x2FED);
		tassert(mem::mirror(vaddr_t(0x3AED))==0x2AED);

		// test palette mirroring
		tassert(mem::mirror(vaddr_t(0x3F9F))==0x3F1F);
		tassert(mem::mirror(vaddr_t(0x3F04))==0x3F04);

		tassert(mem::mirror(vaddr_t(0x3F08))==0x3F08);
		tassert(mem::mirror(vaddr_t(0x3F0C))==0x3F0C);
		tassert(mem::mirror(vaddr_t(0x3F18))==0x3F08);
		tassert(mem::mirror(vaddr_t(0x3F12))==0x3F12);
		tassert(mem::mirror(vaddr_t(0x3F05))==0x3F05);

		tassert(mem::mirror(vaddr_t(0x3F19))==0x3F19);
		return SUCCESS;
	}
};

registerTestCase(PPUMemTest);
registerTestCase(PPUMirroringTest);