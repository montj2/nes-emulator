#include "macro.h"
#include "datatype.h"
#include "nes.h"
#include "cpu.h"
#include "ui.h"
#include "ppu.h"

enum PPUSTATUS {
    STATUS_VBLANK=0x80,
    STATUS_HIT=0x40,
    STATUS_COUNTGT8=0x20,
    STATUS_WRITEIGNORED=0x10
};

enum PPUCONTROL1 {
    CTL1_NMI=0x80,
    CTL1_MASTER_SLAVE=0x40,
    CTL1_8X16=0x20,
    CTL1_PT_BG=0x10,
    CTL1_PT_SPR=0x8,
    CTL1_VERTICAL32=0x4,
    CTL1_CURNT=0x3
};

enum PPUCONTROL2 {
    CTL2_EMPHASIS=0xE0,
    CTL2_INTENSITY=CTL2_EMPHASIS,
    CTL2_EMPHASIS_RED=0x80,
    CTL2_EMPHASIS_GREEN=0x40,
    CTL2_EMPHASIS_BLUE=0x20,
    CTL2_SPR_VISIBLE=0x10,
    CTL2_BG_VISIBLE=0x8,
    CTL2_SPR_CLIP8=0x4,
    CTL2_BG_CLIP8=0x2,
    CTL2_MONO=0x1
};

enum PPUADDRESS{
    ADDRREG_HT=0x1F,
    ADDRREG_XSCROLL=ADDRREG_HT,
    ADDRREG_VT=0x3E0,
    ADDRREG_YSCROLL=ADDRREG_VT,
    ADDRREG_H=0x400,
    ADDRREG_HNT=ADDRREG_H,
    ADDRREG_V=0x800,
    ADDRREG_VNT=ADDRREG_V,
    ADDRREG_NT=ADDRREG_VNT|ADDRREG_HNT,
    ADDRREG_NTOFFSET=0x3FF,
    ADDRREG_FV=0x7000,
    ADDRREG_YOFFSET=ADDRREG_FV,

    ADDRREG_HIGHBYTE=0x7F00/*0x3F00*/,
    ADDRREG_LOWBYTE=0xFF,

    ADDRREG_BANK=0x3000/*0xF000*/,
    ADDRREG_BANKOFFSET=0x0FFF
};

typedef FLAG<_addr15_t,PPUADDRESS,15> scroll_flag_t;
typedef FLAG<_addr14_t,PPUADDRESS,14> vaddr_flag_t;

FLAG<_reg8_t,PPUCONTROL1,8>     control1;               // $2000
FLAG<_reg8_t,PPUCONTROL2,8>     control2;               // $2001
FLAG<_reg8_t,PPUSTATUS,8>       status;                 // $2002

#pragma pack(1)
struct NESVRAM                  vram;                   // Video RAM
struct NESSPRRAM                spriteMem;              // Sprite RAM
#pragma pack()

rgb32_t                         pal32[64];
palindex_t                      vBuffer[240][256];
rgb32_t                         vBuffer32[256*240];

bool                            firstWrite;
bool                            spriteChanged;

vaddr_flag_t                    address1;               // $2006 PPUAddress1 VRAMAddress cnts
scroll_flag_t                   address2;               // $2005 PPUAddress2(scroll) tmpVRAMAddress regs
#define scrollReg address2

off3_t                          xoffset;
saddr_t                         ppu_sprAddress;
byte_t                          ppu_latch;

unsigned long                   scanline;
unsigned long                   frame;

static inline vaddr_t nt(vaddr_t addr);

void ppu_reset() {
	control1.clear();
	control2.clear();
	status.clear();

	firstWrite=true;
	spriteChanged=false;

	address1.clear();
	address2.clear();
	ppu_sprAddress=0;
	ppu_latch=0;
	xoffset=0;
	scanline=0;
	frame=0;

	memset(&vram,0,sizeof(vram));
	memset(&spriteMem,0,sizeof(spriteMem));

	memset(vBuffer,0,sizeof(vBuffer));
	memset(vBuffer32,0,sizeof(vBuffer32));
}

static ioreg_t readStatusRegister() {
    const ioreg_t ret=ValueOf(status);
    // clear VBlank flag
    status.clear(STATUS_VBLANK);
    // After a read occurs, $2005 and $2006 are reset.
    /**
    note!  2005 and 2006 share the toggle that selects between first/second
    writes.  reading 2002 will clear it.
    */
    firstWrite=true;
    return ret;
}

static void blt() {
    rgb32_t p32[32];
    // TODO: MIRRORING
    for (int i=0;i<32;i++) p32[i]=pal32[vram.pal_data[i]];
    rgb32_t* vBuf32=vBuffer32;
    const palindex_t* vBufIdx=&vBuffer[0][0];
    for (int i=0;i<256*240;i++)
        *vBuf32++=p32[valueOf(*vBufIdx++)];
    ui_blit32(vBuffer32,256,240);
}

static void beginFrame() {
    if (control2[CTL2_BG_VISIBLE]) address1.bitCopy(ValueOf(scrollReg)); // apply scroll register
    // invalid frame buffer
    memset(vBuffer,0,sizeof(vBuffer));
}

static void presentFrame() {
    blt();
}

static void endFrame() {
    frame++;
    printf("======= FRAME %ld =======\n",frame);
}

static void renderScanline(const unsigned long scanline) {
    assert(scanline<=239);
    if (!control2[CTL2_BG_VISIBLE]) return;

    address1.set(ADDRREG_HNT,scrollReg[ADDRREG_HNT]);
    address1.update(ADDRREG_HT,scrollReg(ADDRREG_HT));

    const unsigned long HScroll=(address1(ADDRREG_HT)<<3)+xoffset;
    const unsigned long YScroll=(address1(ADDRREG_VT)<<3)+address1(ADDRREG_YOFFSET);

    if (0==address1.inc(ADDRREG_YOFFSET)) {
        if (30==address1.inc(ADDRREG_YSCROLL)) {
            address1.clear(ADDRREG_YSCROLL);
            address1.flip(ADDRREG_VNT);
        }
    }

    const unsigned long TileRow=(YScroll>>3)%30;
    const unsigned long TileOffset=YScroll&7;
    const NESVRAM::VROM::PATTERN_TABLE* const tabPattern=&patternTable(control1[CTL1_PT_BG]?1:0);
    const NESVRAM::NAMEATTRIB_TABLE::NAME_TABLE* tabName;
    const NESVRAM::NAMEATTRIB_TABLE::ATTRIB_TABLE *tabAtr;
    // find out which name table we will actually use
    vaddr_flag_t addrName;
    addrName.clear();
    addrName.update(ADDRREG_BANK,2);
    addrName.update(ADDRREG_NT,control1(CTL1_CURNT));
    addrName.bitCopy(nt(addrName.toBIT()));
    tabName=&nameTable(addrName(ADDRREG_NT));
    tabAtr=&attribTable(addrName(ADDRREG_NT));

    for (unsigned long TileCounter=(HScroll>>3);TileCounter<=31;TileCounter++) {
        const tileid_t TileIndex=tileid_t::wrapper(tabName->tile[TileRow][TileCounter]);
        const unsigned long X=(TileCounter<<3)+7-HScroll; // X coordiante of the right border of the tile on the screen
        // look up in attribute table
        byte_t LookUp=tabAtr->attrib[(TileRow>>2)*8+(TileCounter>>2)];
        switch ((((TileRow&3)>>1)<<1)|(TileCounter&3)>>1) {
        case 0:
            LookUp<<=2;
            break;
        case 1:
            break;
        case 2:
            LookUp>>=2;
            break;
        case 3:
            LookUp>>=4;
            break;
        }
        const byte_t colorHigh=LookUp&0x0C; // D3D2
        // look up in pattern table
        const byte_t ByteLow=tabPattern->ptTile[TileIndex].colorlow[TileOffset];
        const byte_t ByteHigh=tabPattern->ptTile[TileIndex].colorhi[TileOffset];
        for (int pixel=min(X,(unsigned long)7);pixel>=0;pixel--) {
            // Note: B0 indicates the color of the 7th pixel of the tile
            const byte_t colorLow=((ByteLow>>pixel)&1)|(((ByteHigh>>pixel)<<1)&2);
            if (colorLow!=0) // opaque
                vBuffer[scanline][X-pixel]=colorLow|colorHigh;
        }
    }
}

static void startVBlank() {
    status.set(STATUS_VBLANK);
    // Do NMI:
    if (control1[CTL1_NMI]) cpu_requestIRQ(IRQ_NMI);
    // Present
    presentFrame();
}

static void endVBlank() {
    status.clear(STATUS_VBLANK);

}

static void startHBlank() {

}

bool ppu_endScanline() {
    if (scanline==0)  beginFrame();

    printf("---- SCANLINE %ld ----\n",scanline);
    startHBlank();
    if (scanline<=239) renderScanline(scanline);
    if (scanline>=240) status.set(STATUS_VBLANK);
    switch (scanline) {
    case 239:
        startVBlank();
        //getInput();
        break;
    case 258:
        endVBlank();
        break;
    case 261:
        endFrame();
        scanline=0;
        return true;
    }
    scanline++;
    return false;
}

static void scrollWrite(const byte_t byte) {
    if (firstWrite) {
        // First write, horizontal scroll:
        xoffset=byte&7;
        scrollReg.update(ADDRREG_XSCROLL,byte>>3);
    }else {
        // Second write, vertical scroll:
        scrollReg.update(ADDRREG_YOFFSET,byte&7);
        scrollReg.update(ADDRREG_YSCROLL,byte>>3);
    }
    firstWrite=!firstWrite;
}

static void setVRAMAddress(const byte_t byte) {
    if (firstWrite) { // write 6 higher bits
        address1.update(ADDRREG_HIGHBYTE,byte);
    }else { // 8 lower bits
        address1.update(ADDRREG_LOWBYTE,byte);
    }
    printf("ADDRESS1 <- %X\n",ValueOf(address1));
    firstWrite=!firstWrite;
}

static void setSprRamAddress(const saddr_t saddr) {
    ppu_sprAddress=saddr;
}

static void sprramUpdate(const saddr_t saddr,const byte_t data) {
    spriteChanged=true;
}

static byte_t sprramLoad() {
    const byte_t ret=spriteMem.sprram_data[ppu_sprAddress];
    inc(ppu_sprAddress);
    return ret;
}

static void sprramWrite(const byte_t data) {
    spriteMem.sprram_data[ppu_sprAddress]=data;
    sprramUpdate(ppu_sprAddress,data);
    inc(ppu_sprAddress);
}

void ppu_sramDMA(const uint8_t* src) {
    #ifdef VERBOSE
        assert(src!=NULL);
    #endif // VERBOSE
    memcpy(spriteMem.sprram_data,src,sizeof(spriteMem));
}

#define vaddr cast<vaddr_flag_t>(addr)
static inline vaddr_t nt(vaddr_t addr) {
    #ifdef VERBOSE
        assert(vaddr(ADDRREG_BANK)==2); // [$2000,$3000)
    #endif
    switch (mirroring) {
    case MIRROR_HORIZONTAL:
        vaddr.clear(ADDRREG_HNT);
        break;
    case MIRROR_VERTICAL:
        vaddr.clear(ADDRREG_VNT);
        break;
    case MIRROR_SINGLESCREEN:
        vaddr.clear(ADDRREG_NT);
        break;
    case MIRROR_FOURSCREEN:
        break;
    }
    return addr;
}

static vaddr_t vramMirror(vaddr_t addr) {
    addr.bitAndEqual(0x3FFF);
    switch (vaddr(ADDRREG_BANK)) {
    case 0:
    case 1: // [$0,$2000)
        // no mirroring
        break;
    case 2: // [$2000,$3000)
        mirror2000:
            vaddr.bitCopy(nt(addr));
        break;
    case 3: // [$3000,$4000)
        if (vaddr(ADDRREG_BANKOFFSET)<0xF00) {
            vaddr.update(ADDRREG_BANK,2);
            goto mirror2000;
        }
        // mirrorpal: [$3F00,$3FFF]
        vaddr->bitAndEqual(0x3F1F);
        if (0==(ValueOf(vaddr)&0x3)) // $3F00=$3F04=$3F08=...
            vaddr->bitAndEqual(0x3F00);
        break;
    default:
        assert(0);
        break;
    }
    return addr;
}
#undef vdadr

static vaddr_t incAddress1() {
    const vaddr_t ret(address1.toBIT());
    address1->add(control1[CTL1_VERTICAL32]?32:1);
    return ret;
}

static byte_t vramLoad() {
    const vaddr_t addr=vramMirror(incAddress1());
    // If address is in range 0x0000-0x3EFF, return buffered values:
    if (addr<0x3F00) {
        const byte_t ret=ppu_latch;
        ppu_latch=vram.vram_data[addr];
        return ret;
    }else {
        // No buffering in this mem range. Read normally.
        return vram.vram_data[addr];
    }
}

static void vramWrite(const byte_t data) {
    const vaddr_t addr=vramMirror(incAddress1());
    ppu_latch=data;firstWrite=true; // different implementation
    assert(addr>=0x2000);
    vram.vram_data[addr]=data;
    if (addr>=0x3000)
    {
        assert(addr>=0x3F00 && addr<=0x3F1F);
        printf("[PPU] WritePal %X to 0x%04x\n",data,valueOf(addr));
    }
    printf("[PPU] Write %X to 0x%04x\n",data,valueOf(addr));
}

byte_t ppu_regLoad(const maddr_t maddress) {
    #ifdef VERBOSE
        assert(1==(valueOf(maddress)>>13)); // [$2000,$4000)
	#endif
	switch (valueOf(maddress)&0x7) {
		case 2: // $2002 PPU Status Register
			return readStatusRegister();
        case 0: // $2000 PPU Control Register 1
		case 1: // $2001 PPU Control Register 2
		case 3: // $2003 Sprite RAM address
		case 5: // $2005 Screen Scroll offsets
		case 6: // $2006 VRAM address
			break; // The above are write-only registers.
		case 4: // $2004 Sprite Memory Data
		    return sprramLoad();
		case 7: // $2007 VRAM read
			return vramLoad();
	}
	fprintf(stderr,"[PPU] The register at %04X shouldn't be read.\n",valueOf(maddress)&0x2007);
	assert(0);
	return ppu_latch;
}

void ppu_regWrite(const maddr_t maddress,const byte_t data) {
    #ifdef VERBOSE
        assert(1==(valueOf(maddress)>>13)); // [$2000,$4000)
    #endif
	switch (valueOf(maddress)&0x7) {
    case 0: // $2000 PPU Control Register 1
        control1.bitCopy(data);
        break;
    case 1: // $2001 PPU Control Register 2
        control2.bitCopy(data);
        break;
    case 3: // $2003 Sprite RAM address
        setSprRamAddress(saddr_t::wrapper(data));
        break;
    case 4: // $2004 Sprite Memory Data
        sprramWrite(data);
        break;
    case 5: // $2005 Screen Scroll offsets
        scrollWrite(data);
        break;
    case 6: // $2006 VRAM address
        setVRAMAddress(data);
        break;
    case 7: // $2007 VRAM write
        vramWrite(data);
        break;
    case 2: // $2002 PPU Status Register
    default: // Read-only
        fprintf(stderr,"[PPU] The memory at %04X couldn't be written.\n",valueOf(maddress)&0x2007);
        assert(0);
        break;
	}
}

void ppu_loadPal() {
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

void testPPU() {
    static_assert(sizeof(vram)==0x4000,"VRAM struct error");
	static_assert(sizeof(vram.vrom.patternTable)==0x2000,"VRAM struct error");
	static_assert(&vram.name_attrib[0].attribTable.attrib[0]-&vram.vram_data[0]==0x23C0,"VRAM struct error");
	static_assert(&vram.name_attrib[1].nameTable.tile[0][0]-&vram.vram_data[0]==0x2400,"VRAM struct error");
	static_assert(&vram.pal_data[0]-&vram.vram_data[0]==0x3F00,"VRAM struct error");
	static_assert(sizeof(sprite)==0x100,"SprRAM struct error");

	printf("[PPU] VRAM is at %p.\n",&vram.vram_data[0]);

    assert(valueOf(vramMirror(vaddr_t::wrapper(0x1395)))==0x1395);

    mirroring=MIRROR_HORIZONTAL;
    /*
    this.defineMirrorRegion(0x2400,0x2000,0x400);
    this.defineMirrorRegion(0x2c00,0x2800,0x400);
    */
    assert(valueOf(vramMirror(vaddr_t::wrapper(0x2011)))==0x2011);
    assert(valueOf(vramMirror(vaddr_t::wrapper(0x22FF)))==0x22FF);

    assert(valueOf(vramMirror(vaddr_t::wrapper(0x2409)))==0x2009);
    assert(valueOf(vramMirror(vaddr_t::wrapper(0x2409)))==0x2009);

    //printf("%X\n",vramMirror(vaddr_t::wrapper(0x2871)());
    assert(valueOf(vramMirror(vaddr_t::wrapper(0x2871)))==0x2871);
    assert(valueOf(vramMirror(vaddr_t::wrapper(0x2AF1)))==0x2AF1);

    assert(valueOf(vramMirror(vaddr_t::wrapper(0x2D22)))==0x2922);

    mirroring=MIRROR_VERTICAL;
    /*
    this.defineMirrorRegion(0x2800,0x2000,0x400);
    this.defineMirrorRegion(0x2c00,0x2400,0x400);
    */
    assert(valueOf(vramMirror(vaddr_t::wrapper(0x2011)))==0x2011);
    assert(valueOf(vramMirror(vaddr_t::wrapper(0x22FF)))==0x22FF);
    assert(valueOf(vramMirror(vaddr_t::wrapper(0x2405)))==0x2405);
    assert(valueOf(vramMirror(vaddr_t::wrapper(0x2677)))==0x2677);

    assert(valueOf(vramMirror(vaddr_t::wrapper(0x28A3)))==0x20A3);
    assert(valueOf(vramMirror(vaddr_t::wrapper(0x2FFF)))==0x27FF);

    mirroring=MIRROR_SINGLESCREEN;
    /*
    this.defineMirrorRegion(0x2400,0x2000,0x400);
    this.defineMirrorRegion(0x2800,0x2000,0x400);
    this.defineMirrorRegion(0x2c00,0x2000,0x400);
    */
    assert(valueOf(vramMirror(vaddr_t::wrapper(0x2D70)))==0x2170);

    mirroring=MIRROR_FOURSCREEN;
    assert(valueOf(vramMirror(vaddr_t::wrapper(0x2FED)))==0x2FED);
    assert(valueOf(vramMirror(vaddr_t::wrapper(0x3AED)))==0x2AED);

    assert(valueOf(vramMirror(vaddr_t::wrapper(0x3F9F)))==0x3F1F);
    assert(valueOf(vramMirror(vaddr_t::wrapper(0x3F04)))==0x3F00);
    assert(valueOf(vramMirror(vaddr_t::wrapper(0x3F08)))==0x3F00);
    assert(valueOf(vramMirror(vaddr_t::wrapper(0x3F0C)))==0x3F00);
    assert(valueOf(vramMirror(vaddr_t::wrapper(0x3F18)))==0x3F00);
    assert(valueOf(vramMirror(vaddr_t::wrapper(0x3F19)))==0x3F19);
    //assert(valueOf(vramMirror(vaddr_t::wrapper(0xFF19)))==0x3F19);

    puts("**** testPPU() passed ***");
}
