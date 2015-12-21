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

// PPU VRAM Access Registers
typedef flag_set<_addr15_t, PPUADDR, 15> scroll_flag_t;
typedef flag_set<_addr14_t, PPUADDR, 14> vaddr_flag_t;
static scroll_flag_t scroll; // $2005 Background Scrolling Offset / Reload register
static offset3_t xoffset;
static vaddr_flag_t address; // $2006 VRAM Address Register / Scrolling Pointer
static vaddr_flag_t tmpAddress; // debug only
#define address1 address
#define address2 scroll
#define scrollptr address
#define scrollrld scroll

// PPU counters
static int scanline;
static long long frameNum;

namespace mem
{
	// addresses of currently selected VROM banks.
	static int prevBankSrc[8];

	// shared for both port $2005 and $2006
	static bool firstWrite;
	// $2007 Read/Write Data Register
	static byte_t latch;
	
	static void resetToggle()
	{
		firstWrite=true;
		latch=INVALID;
	}

	static bool toggle()
	{
		const bool ret=firstWrite;
		firstWrite=!firstWrite;
		return ret;
	}

	static void reset()
	{
		// reset bank-switching state
		memset(prevBankSrc, -1, sizeof(prevBankSrc));

		// clear memory
		memset(&vram,0,sizeof(vram));
		memset(&oam,0,sizeof(oam));
	}

	static void copyBanks(const int dest, const int src, const int count)
	{
		assert((dest+count)*0x400<=0x2000);
		assert((src+count)*0x400<=(int)rom::sizeOfVROM());
		memcpy(&vramData(dest*0x400), rom::getVROM()+src*0x400, count*0x400);
	}

	void bankSwitch(const int dest, const int src, const int count)
	{
		assert(dest>=0 && dest+count<=8);
		assert(src!=INVALID && src>=0);
		for (int i=0; i<count; i++)
		{
			if (prevBankSrc[dest+i]!=src+i)
			{
				prevBankSrc[dest+i]=src+i;
				copyBanks(dest+i, src+i, 1);
			}
		}
	}
	
	static bool saveCompleteMemory()
	{
#ifdef SAVE_COMPLETE_MEMORY
		return true;
#else
		// force complete memory dump when CHR banks don't exist in the rom.
		return (rom::count8KCHR() == 0);
#endif
	}

	static void save(FILE *fp)
	{
		// memory
		if (saveCompleteMemory())
		{
			fwrite(&vram, sizeof(vram), 1, fp);
		}else
		{
			// skip vrom
			fwrite(&vram.nameTables, sizeof(vram.nameTables), 1, fp);
			fwrite(&vram.pal, sizeof(vram.pal), 1, fp);
		}
		fwrite(&oam, sizeof(oam), 1, fp);
		
		// toggle
		fwrite(&firstWrite, sizeof(firstWrite), 1, fp);

		// latch
		fwrite(&latch, sizeof(latch), 1, fp);

		// bank-switching state
		fwrite(prevBankSrc, sizeof(prevBankSrc), 1, fp);
	}

	static void load(FILE *fp)
	{
		// memory
		if (saveCompleteMemory())
		{
			fread(&vram, sizeof(vram), 1, fp);
		}else
		{
			// skip vrom
			fread(&vram.nameTables, sizeof(vram.nameTables), 1, fp);
			fread(&vram.pal, sizeof(vram.pal), 1, fp);
		}
		fread(&oam, sizeof(oam), 1, fp);

		// toggle
		fread(&firstWrite, sizeof(firstWrite), 1, fp);

		// latch
		fread(&latch, sizeof(latch), 1, fp);

		// bank-switching state
		if (saveCompleteMemory())
		{
			fread(prevBankSrc, sizeof(prevBankSrc), 1, fp);
		}else
		{
			int bankSrc[8];
			STATIC_ASSERT(sizeof(bankSrc) == sizeof(prevBankSrc));
			fread(bankSrc, sizeof(bankSrc), 1, fp);
			for (int i=0;i<8;i++)
			{
				bankSwitch(i, bankSrc[i], 1);
			}
		}
	}

	static vaddr_t ntMirror(vaddr_flag_t vaddr)
	{
		switch (rom::mirrorMode())
		{
		case MIRRORING::HORIZONTAL:
			vaddr.change<PPUADDR::NT_H>(vaddr[PPUADDR::NT_V]);
		case MIRRORING::VERTICAL:
			vaddr-=PPUADDR::NT_V;
			break;
		case MIRRORING::LSINGLESCREEN:
			vaddr.update<PPUADDR::NT>(0);
			break;
		case MIRRORING::HSINGLESCREEN:
			vaddr.update<PPUADDR::NT>(1);
			break;
		case MIRRORING::FOURSCREEN:
			// no mirroring
			break;
		}
		return vaddr;
	}

	static vaddr_t mirror(vaddr_flag_t vaddr, bool forRead=true)
	{
		assert(address.mask(PPUADDR::UNUSED)==0);
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
		vaddr.clear(PPUADDR::UNUSED);
		return vaddr;
	}

	static void incAddress()
	{
		assert(address.mask(PPUADDR::UNUSED)==0);
		address.asBitField()+=control[PPUCTRL::VERTICAL_WRITE]?32:1;
	}

	static void setAddress(const byte_t byte) // $2006
	{
		if (firstWrite) // higher 6 bits
		{
			vassert((byte&~0x3F)==0);
			tmpAddress.update<PPUADDR::HIGH_BYTE>(byte&0x3F);

			// store in Reload register temporarily
			address2.copy<PPUADDR::FIRST_WRITE_LO, 0, 2>(byte);
			control.copy<PPUCTRL::CURRENT_NT, 2, 2>(byte);
			address2.copy<PPUADDR::FIRST_WRITE_HI, 4, 2>(byte);
		}else // lower 8 bits
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
			address1.asBitField()&=address1.asBitField().MAX;
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

	static bool canWrite()
	{
		return !status[PPUSTATUS::WRITEIGNORED];
	}

	static void write(const byte_t data)
	{
		// make sure it's safe to write
		// assert(canWrite());

		const vaddr_t addr=mirror(address, false);
		{
			// ?
			// resetToggle();
		}
#ifdef WANT_MEM_PROTECTION
		if (rom::count8KCHR()>0)
		{
			// don't allow writes to vrom if the rom file has any CHR-ROM data.
			ERROR_IF(addr<0x2000, INVALID_MEMORY_ACCESS, MEMORY_CANT_BE_WRITTEN, "vaddress", valueOf(address), "actual vaddress", valueOf(addr));
		}
#endif
		vramData(addr)=data;
		incAddress();
	}
}

namespace render
{
	const int RENDER_WIDTH=256;
	const int RENDER_HEIGHT=240;

	static rgb32_t pal32[64];
	static palindex_t vBuffer[RENDER_HEIGHT][RENDER_WIDTH];
	static rgb32_t vBuffer32[SCREEN_HEIGHT*SCREEN_WIDTH];

	static int8_t pendingSprites[64];
	static int pendingSpritesCount;
	static bool solidPixel[RENDER_WIDTH];

	static void setScroll(const byte_t byte)
	{
		if (mem::toggle())
		{
			// set horizontal scroll
			xoffset=byte&7;
			scroll.update<PPUADDR::XSCROLL>(byte>>3);
		}else
		{
			// set vertical scroll
			scroll.update<PPUADDR::YOFFSET>(byte&7);
			assert((byte>>3)<30);
			scroll.update<PPUADDR::YSCROLL>(byte>>3);
		}
	}

	static void getReload(int *FH, int *HT, int *VT, int *NT, int *FV)
	{
		if (FH) *FH=xoffset;
		if (HT) *HT=scroll.select(PPUADDR::TILE_H);
		if (VT) *VT=scroll.select(PPUADDR::TILE_V);
		if (NT) *NT=control.select(PPUCTRL::CURRENT_NT);
		if (FV) *FV=scroll.select(PPUADDR::YOFFSET);
	}

	static void reloadVertical()
	{
		int VT, NT, FV;
		getReload(0, 0, &VT, &NT, &FV);
		address.update<PPUADDR::TILE_V>(VT);
		address.update<PPUADDR::NT_V>(NT>>1);
		address.update<PPUADDR::YOFFSET>(FV);
	}

	static void reloadHorizontal(int *FH=0)
	{
		int HT, NT;
		getReload(FH, &HT, 0, &NT, 0);
		address.update<PPUADDR::TILE_H>(HT);
		address.update<PPUADDR::NT_H>(NT&1);
	}

	static void clear()
	{
		// clear back buffer
		memset(vBuffer, 0, sizeof(vBuffer));
	}

	static void reset()
	{
		render::clear();

		// also clear front buffer
		memset(vBuffer32, 0, sizeof(vBuffer32));

		pendingSpritesCount = 0;
		memset(pendingSprites, -1, sizeof(pendingSprites));	
	}

	static bool enabled()
	{
		return mask[PPUMASK::BG_VISIBLE] || mask[PPUMASK::SPR_VISIBLE];
	}

	static bool leftClipping()
	{
		return mask[PPUMASK::BG_CLIP8] || mask[PPUMASK::SPR_CLIP8];
	}

	static void present()
	{
		if (enabled())
		{
			// cache palette colors
			rgb32_t p32[32];
			for (int i=0;i<32;i++) p32[i]=pal32[colorIdx(i)];

			// look up each pixel
			rgb32_t* vBuf32=vBuffer32;
			const palindex_t* vBufIdx=&vBuffer[SCREEN_YOFFSET][0];
			for (int i=0;i<SCREEN_HEIGHT*SCREEN_WIDTH;i++)
			{
				*vBuf32++=p32[valueOf(*vBufIdx++)];
			}
		}
		
		// display
		ui::blt32(vBuffer32, SCREEN_WIDTH, SCREEN_HEIGHT);
	}

	static void startVBlank()
	{
		// present frame onto screen
		present();
		// set VBlank flag
		status|=PPUSTATUS::VBLANK;
		// allow writes
		status-=PPUSTATUS::WRITEIGNORED;
		// do NMI
		if (control[PPUCTRL::NMI_ENABLED])
		{
			cpu::irq(IRQTYPE::NMI);
		}
	}

	static void duringVBlank()
	{
		// ? keep VBlank flag turned on
		{
			// status|=PPUSTATUS::VBLANK;
		}
	}

	static void endVBlank()
	{
		// clear VBlank flag
		status-=PPUSTATUS::VBLANK;
		// clear HIT flag
		status-=PPUSTATUS::HIT;
	}

	static void preRender()
	{
		if (enabled())
		{
			reloadVertical();
			status|=PPUSTATUS::WRITEIGNORED;
		}
	}

	static void postRender()
	{
	}

	static void beginFrame()
	{
		ui::onFrameBegin();
	}

	static void endFrame()
	{
		++frameNum;

		ui::onFrameEnd();
	}

	static void drawBackground()
	{
		if (mask[PPUMASK::BG_VISIBLE])
		{
			// determine origin
			int xoffset;
			reloadHorizontal(&xoffset);
			int startX=(address(PPUADDR::XSCROLL)<<3)+xoffset;
			const int startY=(address(PPUADDR::YSCROLL)<<3)+address(PPUADDR::YOFFSET);

			assert(startX>=0 && startX<256);
			assert(startY>=0 && startY<240);

			// determine what tables to use for the first part
			vaddr_flag_t mirrored(mem::ntMirror(address));
			const NESVRAM::NAMEATTRIB_TABLE::NAME_TABLE *nt;
			const NESVRAM::NAMEATTRIB_TABLE::ATTRIBUTE_TABLE *attr;
			const NESVRAM::VROM::PATTERN_TABLE *pt;
			nt=&vramNt(mirrored(PPUADDR::NT));
			attr=&vramAt(mirrored(PPUADDR::NT));
			pt=&vramPt(control[PPUCTRL::BG_PATTERN]?1:0);

			// determine tile position in current name table
			const int tileRow=(startY>>3)%30;
			const int tileYOffset=startY&7;

			for (int tileCounter=(startX>>3);tileCounter<=31;tileCounter++)
			{
				const tileid_t tileIndex(nt->tiles[tileRow][tileCounter]);
				const int X = (tileCounter<<3)+7-startX;

				// look up the tile in attribute table to find its color (D2 and D3)
				const byte_t colorD2D3 = attr->lookup(tileRow, tileCounter);

				// look up the tile in pattern table to find its color (D0 and D1)
				const byte_t colorD0 = pt->tiles[tileIndex].colorD0[tileYOffset];
				const byte_t colorD1 = pt->tiles[tileIndex].colorD1[tileYOffset];

				for (int pixel=min(X,7);pixel>=0;pixel--)
				{
					// Note: B0 indicates the color at the 7th pixel of the tile
					const byte_t colorD0D1 = ((colorD0>>pixel)&1)|(((colorD1>>pixel)<<1)&2);
					const byte_t color = colorD0D1|colorD2D3;
					// write to frame buffer
					vassert(X-pixel>=0 && X-pixel<RENDER_WIDTH);
					vBuffer[scanline][X-pixel]=color;
				}
			}

			if (startX>=0)
			{
				// now render for the second part
				// switch across to the next tables
				{
					// ?
					address.flip(PPUADDR::NT_H);
				}
				mirrored=mem::ntMirror(address);
				nt=&vramNt(mirrored(PPUADDR::NT));
				attr=&vramAt(mirrored(PPUADDR::NT));

				const int endTile = (startX+7)>>3;
				for (int tileCounter=0;tileCounter<endTile;tileCounter++)
				{
					const tileid_t tileIndex(nt->tiles[tileRow][tileCounter]);
					const int X = (tileCounter<<3)+7+(256-startX);

					// look up the tile in attribute table to find its color (D2 and D3)
					const byte_t colorD2D3 = attr->lookup(tileRow, tileCounter);

					// look up the tile in pattern table to find its color (D0 and D1)
					const byte_t colorD0 = pt->tiles[tileIndex].colorD0[tileYOffset];
					const byte_t colorD1 = pt->tiles[tileIndex].colorD1[tileYOffset];

					for (int pixel=max(X-255,0);pixel<=7;pixel++)
					{
						// Note: B0 indicates the color at the 7th pixel of the tile
						const byte_t colorD0D1 = ((colorD0>>pixel)&1)|(((colorD1>>pixel)<<1)&2);
						const byte_t color = colorD0D1|colorD2D3;
						// write to frame buffer
						vassert(X-pixel>=0 && X-pixel<RENDER_WIDTH);
						vBuffer[scanline][X-pixel]=color;
					}
				}
			}

			// set address to next scanline
			if (address.inc(PPUADDR::YOFFSET)==0)
			{
				if (address.inc(PPUADDR::YSCROLL)==30)
				{
					address.update<PPUADDR::YSCROLL>(0);
					address.flip(PPUADDR::NT_V);
					// no need to update scroll reload
				}
			}
		}
	}

	int visibleSpriteCount=0;
	static void evaluateSprites()
	{
		pendingSpritesCount=0;

		if (mask[PPUMASK::SPR_VISIBLE])
		{
			status-=PPUSTATUS::COUNTGT8;

			// find sprites that are within y range for the scanline
			const int sprHeight=control[PPUCTRL::LARGE_SPRITE]?16:8;

			for (int i=63;i>=0;i--)
			{
				if (oamSprite(i).yminus1<scanline && oamSprite(i).yminus1+sprHeight>=scanline)
				{
#ifdef SPRITE_LIMIT
					if (pendingSpritesCount>=8)
					{
						// more than 8 sprites appear in this scanline
						status|=PPUSTATUS::COUNTGT8;
						break;
					}else
#endif
					{
						pendingSprites[pendingSpritesCount++]=i;
					}
				}
			}

#ifdef MONITOR_RENDERING
			visibleSpriteCount=0;
			for (int i=63;i>=0;i--)
			{
				if (oamSprite(i).yminus1<239)
					visibleSpriteCount++;
			}
#endif
			for (int i=0;i<RENDER_WIDTH;i++)
			{
				solidPixel[i]=((vBuffer[scanline][i]&3)!=0);
			}
		}
	}

	static void drawSprites(const bool behindBG)
	{
		if (pendingSpritesCount>0)
		{
			const int sprWidth=8;
			const int sprHeight=control[PPUCTRL::LARGE_SPRITE]?16:8;

			for (int i=0;i<pendingSpritesCount;i++)
			{
				const int sprId = pendingSprites[i];
				const auto spr = oamSprite(sprId);
				if (spr.attrib[SPRATTR::BEHIND_BG]!=behindBG) continue;

				// get the sprite info
				const int sprYOffset = scanline-(spr.yminus1+1);
				vassert(sprYOffset>=0 && sprYOffset<sprHeight);

				const byte_t colorD2D3 = spr.attrib.select(SPRATTR::COLOR_HI)<<2;

				for (int pixel=0;pixel<sprWidth;pixel++)
				{
					const int X=spr.x+pixel;
					if (X>255) break;

					const NESVRAM::VROM::PATTERN_TABLE *pt;
					const int tileXOffset=spr.attrib[SPRATTR::FLIP_H]?(sprWidth-1-pixel):pixel;
					const int tileYOffset=(spr.attrib[SPRATTR::FLIP_V]?(sprHeight-1-sprYOffset):sprYOffset)&7;
					tileid_t tileIndex;
					if (control[PPUCTRL::LARGE_SPRITE])
					{
						tileIndex=(spr.tile&~1)|(sprYOffset>>3);
						pt=&vramPt(spr.tile&1);
					}else
					{
						tileIndex=spr.tile;
						pt=&vramPt(control[PPUCTRL::SPR_PATTERN]?1:0);
					}

					// look up the tile in pattern table to find its color (D0 and D1)
					const byte_t colorD0 = pt->tiles[tileIndex].colorD0[tileYOffset];
					const byte_t colorD1 = pt->tiles[tileIndex].colorD1[tileYOffset];

					const byte_t colorD0D1 = ((colorD0>>(7-tileXOffset))&1)|(((colorD1>>(7-tileXOffset))<<1)&2);
					const byte_t color = colorD0D1|colorD2D3|0x10;

					if ((color&3)!=0) // opaque pixel
					{
						// sprite 0 hit detection (regardless priority)
						
						if (sprId==0 && !status[PPUSTATUS::HIT] && solidPixel[X] && mask[PPUMASK::BG_VISIBLE] && !(leftClipping() && X<8) && X!=255)
						{
							// background is non-transparent here
							status|=PPUSTATUS::HIT;
						}
						// write to frame buffer
						if (!behindBG)
						{
							// always overwite for front-priority sprites
							vBuffer[scanline][X]=color;
						}else
						{
							// only write when the corresponding background pixel is transparent
							if ((vBuffer[scanline][X]&3)==0)
							{
								vBuffer[scanline][X]=color;
							}
						}
					}

					//if (!behindBG) vBuffer[scanline][X]=color;
				}
			}
		}
	}

	static void renderScanline()
	{
		if (enabled())
		{
			if (scanline>=0 && scanline<=239)
			{
			#ifdef MONITOR_RENDERING
				printf("[P] --- Scanline %03d --- Sprite 0: (%d, %d) %c%c%c Scroll=[%3d,%3d] %d visible\n", scanline, oamSprite(0).x, oamSprite(0).yminus1+1, 
					(rom::mirrorMode()==MIRRORING::HORIZONTAL)?'H':'V',
					mask[PPUMASK::SPR_VISIBLE]?'S':'-',
					mask[PPUMASK::BG_VISIBLE]?'B':'-',
					scroll(PPUADDR::XSCROLL)*8+xoffset,
					scroll(PPUADDR::YSCROLL)*8+scroll(PPUADDR::YOFFSET),
					visibleSpriteCount);
			#endif
				drawBackground();
				evaluateSprites();
				drawSprites(false); // front-priority sprites first
				drawSprites(true);
			}else
			{
				// dummy scanline
			}
		}
	}

	static bool HBlank()
	{
		if (scanline==-1)
		{
			beginFrame();
			preRender();
		}else if (scanline>=0 && scanline<=239)
		{
			// visible scanlines
			renderScanline();
		}else if (scanline==240)
		{
			// dummy scanline
			renderScanline();
			postRender();
			// enter vblank
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

		// reset counters
		scanline = -1;
		frameNum = 0;

		// reset renderer
		render::reset();

		// reset memory
		mem::reset();
	}

	void save(FILE *fp)
	{
		// registers
		fwrite(&control, sizeof(control), 1, fp);
		fwrite(&mask, sizeof(mask), 1, fp);
		fwrite(&status, sizeof(status), 1, fp);

		fwrite(&scroll, sizeof(scroll), 1, fp);
		fwrite(&xoffset, sizeof(xoffset), 1, fp);
		fwrite(&address, sizeof(address), 1, fp);

		fwrite(&oamAddr, sizeof(oamAddr), 1, fp);

		// memory
		mem::save(fp);
	}

	void load(FILE *fp)
	{
		// registers
		fread(&control, sizeof(control), 1, fp);
		fread(&mask, sizeof(mask), 1, fp);
		fread(&status, sizeof(status), 1, fp);

		fread(&scroll, sizeof(scroll), 1, fp);
		fread(&xoffset, sizeof(xoffset), 1, fp);
		fread(&address, sizeof(address), 1, fp);

		fread(&oamAddr, sizeof(oamAddr), 1, fp);

		// memory
		mem::load(fp);
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
			mem::resetToggle();
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
			if (!control[PPUCTRL::NMI_ENABLED] && (data&(byte_t)PPUCTRL::NMI_ENABLED) && status[PPUSTATUS::VBLANK])
			{
				// nmi should occur when enabled during VBlank
				cpu::irq(IRQTYPE::NMI);
			}
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
		#ifdef MONITOR_RENDERING
			debug::printPPUState(frameNum, scanline, status[PPUSTATUS::VBLANK], status[PPUSTATUS::HIT], mask[PPUMASK::BG_VISIBLE], mask[PPUMASK::SPR_VISIBLE]);
		#endif
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

namespace pmapper
{
	byte_t maskCHR(byte_t bank, const byte_t count)
	{
		assert(count!=0);
		if (count==0)
		{
			return 0;
		}else if (!SINGLE_BIT(count))
		{
			byte_t m;
			for (m=1;m<count;m<<=1);
			bank&=m-1;
		}else
		{
			bank&=count-1;
		}
		return bank>=count?count-1:bank;
	}

	void selectVROM(const int CHRSize, const byte_t value, const byte_t bank)
	{
		mem::bankSwitch(
			bank*CHRSize,
			maskCHR(value, rom::count8KCHR()*(8/CHRSize))*CHRSize,
			CHRSize
			);
	}

	void select8KVROM(const byte_t value)
	{
		selectVROM(8, value, 0);
	}

	bool setup()
	{
		switch (rom::mapperType())
		{
		case 0: // no mapper
		case 1: // Mapper 1: MMC1
		case 2: // Mapper 2: UNROM - PRG/16K
		case 3: // Mapper 3: CNROM - VROM/8K
			if (rom::sizeOfVROM()>=0x2000) // 8K of texture or more
				mem::bankSwitch(0, 0, 8);
			else if (rom::sizeOfVROM()>0)
			{
invalidVROMSize:
				ERROR(INVALID_MEMORY_ACCESS, MAPPER_FAILURE, "vrom size", rom::sizeOfVROM(), "mapper", rom::mapperType());
				return false;
			}
			return true;
		}
		// unknown mapper
		FATAL_ERROR(INVALID_ROM, UNSUPPORTED_MAPPER_TYPE, "mapper", rom::mapperType());
		return false;
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
		for (int i=(int)MIRRORING::MIN;i<=(int)MIRRORING::MAX;i++)
		{
			rom::setMirrorMode((MIRRORING)i);
			tassert(mem::mirror(vaddr_t(0x1395))==0x1395); // no mapping should occur
		}

		// Disable the following test because the mirroring implemention has changed.
		/*
		// test nametable mirroring
		rom::setMirrorMode(MIRRORING::HORIZONTAL);
		tassert(mem::mirror(vaddr_t(0x2011))==0x2011);
		tassert(mem::mirror(vaddr_t(0x22FF))==0x22FF);

		tassert(mem::mirror(vaddr_t(0x2409))==0x2009);
		tassert(mem::mirror(vaddr_t(0x2409))==0x2009);

		tassert(mem::mirror(vaddr_t(0x2871))==0x2871);
		tassert(mem::mirror(vaddr_t(0x2AF1))==0x2AF1);

		tassert(mem::mirror(vaddr_t(0x2D22))==0x2922);

		rom::setMirrorMode(MIRRORING::VERTICAL);
		tassert(mem::mirror(vaddr_t(0x2011))==0x2011);
		tassert(mem::mirror(vaddr_t(0x22FF))==0x22FF);
		tassert(mem::mirror(vaddr_t(0x2405))==0x2405);
		tassert(mem::mirror(vaddr_t(0x2677))==0x2677);

		tassert(mem::mirror(vaddr_t(0x28A3))==0x20A3);
		tassert(mem::mirror(vaddr_t(0x2FFF))==0x27FF);
		*/

		rom::setMirrorMode(MIRRORING::LSINGLESCREEN);
		tassert(mem::mirror(vaddr_t(0x2D70))==0x2170);
		tassert(mem::mirror(vaddr_t(0x2570))==0x2170);

		rom::setMirrorMode(MIRRORING::HSINGLESCREEN);
		tassert(mem::mirror(vaddr_t(0x2170))==0x2570);
		tassert(mem::mirror(vaddr_t(0x2D70))==0x2570);
		
		rom::setMirrorMode(MIRRORING::FOURSCREEN);
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