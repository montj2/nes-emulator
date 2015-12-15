#include "../stdafx.h"

// local header files
#include "../macros.h"
#include "../types/types.h"
#include "../unittest/framework.h"

#include "internals.h"
#include "debug.h"
#include "rom.h"
#include "cpu.h"
#include "ppu.h"
#include "../ui.h"

// PPU Memory
__declspec(align(0x1000))
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
static scroll_flag_t scroll; // $2005 Background Scrolling Offset / Reload register
static offset3_t xoffset;
static vaddr_flag_t address; // $2006 VRAM Address Register / Scrolling Pointer
static vaddr_flag_t tmpAddress; // debug only
static byte_t latch; // $2007 Read/Write Data Register
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
		return vaddr;
	}

	static vaddr_t mirror(vaddr_flag_t vaddr, bool forRead=true)
	{
		switch (vaddr.select(PPUADDR::BANK))
		{
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

	static void setAddress(const byte_t byte) // $2006
	{
		if (firstWrite) // 6 higher bits
		{
			assert((byte&~0x3F)==0);
			tmpAddress.update<PPUADDR::HIGH_BYTE>(byte&0x3F);

			// store in Reload register temporarily
			address2.copy<PPUADDR::FIRST_WRITE_LO, 0, 2>(byte);
			control.copy<PPUCTRL::CURRENT_NT, 2, 2>(byte);
			address2.copy<PPUADDR::FIRST_WRITE_HI, 4, 2>(byte);
		}else // 8 lower bits
		{
			tmpAddress.update<PPUADDR::LOW_BYTE>(byte);
			
			{
				// ?
				address2.update<PPUADDR::LOW_BYTE>(byte);
			}
			address1.update<PPUADDR::LOW_BYTE>(byte);
			address1.update<PPUADDR::FIRST_WRITE_LO>(address2.select(PPUADDR::FIRST_WRITE_LO));
			address1.update<PPUADDR::FIRST_WRITE_MID>(control.select(PPUCTRL::CURRENT_NT));
			address1.update<PPUADDR::FIRST_WRITE_HI>(address2.select(PPUADDR::FIRST_WRITE_HI));
			// check if correctly set
			assert(valueOf(tmpAddress)==valueOf(address1));
		}
		firstWrite=!firstWrite;
	}

	static byte_t read()
	{
		const vaddr_t addr=mirror(address, true);
		incAddress();
		if (addr<0x3F00)
		{
			// return buffered data
			const byte_t oldLatch=latch;
			latch=vramData(addr);
			return oldLatch;
		}
		// no buffering for palette memory access
		return vramData(addr);
	}

	static void write(const byte_t data)
	{
		assert(!status[PPUSTATUS::WRITEIGNORED]);
		const vaddr_t addr=mirror(address, false);
		{
			// ?
			firstWrite=true;
			latch=data;
		}
		vramData(addr)=data;
		incAddress();
	}
}

namespace render
{
	static rgb32_t pal32[64];
	static palindex_t vBuffer[240][256];
	static rgb32_t vBuffer32[256*240];

	static void setScroll(const byte_t byte)
	{
		if (firstWrite)
		{
			// set horizontal scroll
			xoffset=byte&7;
			scroll.update<PPUADDR::XSCROLL>(byte>>3);
		}else
		{
			// set vertical scroll
			scroll.update<PPUADDR::YOFFSET>(byte&7);
			scroll.update<PPUADDR::YSCROLL>(byte>>3);
		}
		firstWrite=!firstWrite;
	}

	static void getReload(int *FH, int *HT, int *VT, int *NT, int *FV)
	{
		if (FH) *FH=xoffset;
		if (HT) *HT=scroll.select(PPUADDR::TILE_H);
		if (VT) *VT=scroll.select(PPUADDR::TILE_V);
		if (NT) *NT=control.select(PPUCTRL::CURRENT_NT);
		if (FV) *FV=scroll.select(PPUADDR::FV);
	}

	static void reloadVertical()
	{
	}

	static void reloadHorizontal()
	{
	}

	static bool enabled()
	{
		return mask[PPUMASK::BG_VISIBLE] || mask[PPUMASK::SPR_VISIBLE];
	}

	static void present()
	{
		// cache palette colors
		rgb32_t p32[32];
		for (int i=0;i<32;i++) p32[i]=pal32[colorIdx(i)];

		// look up each pixel
		rgb32_t* vBuf32=vBuffer32;
		const palindex_t* vBufIdx=&vBuffer[0][0];
		for (int i=0;i<256*240;i++)
			*vBuf32++=p32[valueOf(*vBufIdx++)];
		
		// display
		ui::blt32(vBuffer32, 256, 240);
	}

	static void startVBlank()
	{
		// present frame to screen
		present();
		// set VBlank flag
		status|=PPUSTATUS::VBLANK;
		// do NMI
		if (control[PPUCTRL::NMI_ENABLED])
		{
			cpu::irq(IRQTYPE::NMI);
		}
	}

	static void duringVBlank()
	{
		// keep VBlank flag turned on
		status|=PPUSTATUS::VBLANK;
	}

	static void endVBlank()
	{
		// clear VBlank flag
		status-=PPUSTATUS::VBLANK;
	}

	static void drawBackground()
	{
	}

	static void drawSprites()
	{
	}

	static void renderScanline()
	{
		if (enabled())
		{
			reloadHorizontal();
			drawBackground();
			drawSprites();
		}
	}

	static void beginFrame()
	{
		if (enabled())
		{
			reloadVertical();
		}
		
		ui::onFrameBegin();
	}

	static void endFrame()
	{
		++frameNum;

		ui::onFrameEnd();
	}

	static bool HBlank()
	{
		printf("[ ] ------ Scanline %d ------\n",scanline);
		if (scanline==-1)
		{
			beginFrame();
		}else if (scanline>=0 && scanline<=239)
		{
			renderScanline();
		}else if (scanline==240)
		{
			startVBlank();
		}else if (scanline>=241 && scanline<=259)
		{
			duringVBlank();
		}else if (scanline==260)
		{
			endVBlank();
		}else if (scanline==261)
		{
			endFrame();
			scanline=-1;
			return false;
		}
		scanline++;
		return true;
	}

	static void loadNTSCPal()
	{
		pal32[ 0] = Rgb32(117,117,117);
		pal32[ 1] = Rgb32( 39, 27,143);
		pal32[ 2] = Rgb32(  0,  0,171);
		pal32[ 3] = Rgb32( 71,  0,159);
		pal32[ 4] = Rgb32(143,  0,119);
		pal32[ 5] = Rgb32(171,  0, 19);
		pal32[ 6] = Rgb32(167,  0,  0);
		pal32[ 7] = Rgb32(127, 11,  0);
		pal32[ 8] = Rgb32( 67, 47,  0);
		pal32[ 9] = Rgb32(  0, 71,  0);
		pal32[10] = Rgb32(  0, 81,  0);
		pal32[11] = Rgb32(  0, 63, 23);
		pal32[12] = Rgb32( 27, 63, 95);
		pal32[13] = Rgb32(  0,  0,  0);
		pal32[14] = Rgb32(  0,  0,  0);
		pal32[15] = Rgb32(  0,  0,  0);
		pal32[16] = Rgb32(188,188,188);
		pal32[17] = Rgb32(  0,115,239);
		pal32[18] = Rgb32( 35, 59,239);
		pal32[19] = Rgb32(131,  0,243);
		pal32[20] = Rgb32(191,  0,191);
		pal32[21] = Rgb32(231,  0, 91);
		pal32[22] = Rgb32(219, 43,  0);
		pal32[23] = Rgb32(203, 79, 15);
		pal32[24] = Rgb32(139,115,  0);
		pal32[25] = Rgb32(  0,151,  0);
		pal32[26] = Rgb32(  0,171,  0);
		pal32[27] = Rgb32(  0,147, 59);
		pal32[28] = Rgb32(  0,131,139);
		pal32[29] = Rgb32(  0,  0,  0);
		pal32[30] = Rgb32(  0,  0,  0);
		pal32[31] = Rgb32(  0,  0,  0);
		pal32[32] = Rgb32(255,255,255);
		pal32[33] = Rgb32( 63,191,255);
		pal32[34] = Rgb32( 95,151,255);
		pal32[35] = Rgb32(167,139,253);
		pal32[36] = Rgb32(247,123,255);
		pal32[37] = Rgb32(255,119,183);
		pal32[38] = Rgb32(255,119, 99);
		pal32[39] = Rgb32(255,155, 59);
		pal32[40] = Rgb32(243,191, 63);
		pal32[41] = Rgb32(131,211, 19);
		pal32[42] = Rgb32( 79,223, 75);
		pal32[43] = Rgb32( 88,248,152);
		pal32[44] = Rgb32(  0,235,219);
		pal32[45] = Rgb32(  0,  0,  0);
		pal32[46] = Rgb32(  0,  0,  0);
		pal32[47] = Rgb32(  0,  0,  0);
		pal32[48] = Rgb32(255,255,255);
		pal32[49] = Rgb32(171,231,255);
		pal32[50] = Rgb32(199,215,255);
		pal32[51] = Rgb32(215,203,255);
		pal32[52] = Rgb32(255,199,255);
		pal32[53] = Rgb32(255,199,219);
		pal32[54] = Rgb32(255,191,179);
		pal32[55] = Rgb32(255,219,171);
		pal32[56] = Rgb32(255,231,163);
		pal32[57] = Rgb32(227,255,163);
		pal32[58] = Rgb32(171,243,191);
		pal32[59] = Rgb32(179,255,207);
		pal32[60] = Rgb32(159,255,243);
		pal32[61] = Rgb32(  0,  0,  0);
		pal32[62] = Rgb32(  0,  0,  0);
		pal32[63] = Rgb32(  0,  0,  0);
	}

	static void clear()
	{
		// clear frame buffer
		memset(vBuffer, 0, sizeof(vBuffer));
		memset(vBuffer32, 0, sizeof(vBuffer32));
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
		tmpAddress.clearAll();

		// reset state
		firstWrite = true;
		latch = INVALID;
		scanline = -1;
		frameNum = 0;

		// clear memory
		memset(&vram,0,sizeof(vram));
		memset(&oam,0,sizeof(oam));

		// clear video buffer
		render::clear();
	}

	void init()
	{
		render::loadNTSCPal();
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

	bool hsync()
	{
		return render::HBlank();
	}

	void dma(const uint8_t* src)
	{
		assert(src!=nullptr);
		memcpy(&oam, src, sizeof(oam));
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
		tassert(ptr_diff(&vramAt(0).attribs[0],&vramData(0))==0x23C0);
		tassert(ptr_diff(&vramNt(1).tiles[0][0],&vramData(0))==0x2400);
		tassert(ptr_diff(&vram.pal,&vramData(0))==0x3F00);
		tassert(sizeof(oam)==0x100);

		printf("[ ] VRAM at 0x%p\n",&vram);
		printf("[ ] SPR-RAM at 0x%p\n",&oam);
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