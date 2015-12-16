enum class PPUSTATUS {
    VBLANK=0x80,
    HIT=0x40,
    COUNTGT8=0x20,
    WRITEIGNORED=0x10
};

enum class PPUCTRL {
    NMI_ENABLED=0x80,
    MASTER_SLAVE=0x40,
    LARGE_SPRITE=0x20,
    BG_PATTERN=0x10,
    SPRITE_PATTERN=0x8,
    VERTICAL_WRITE=0x4,
    CURRENT_NT=0x3
};

enum class PPUMASK {
    EMPHASIS=0xE0,
    INTENSITY=EMPHASIS,
    EMPHASIS_RED=0x80,
    EMPHASIS_GREEN=0x40,
    EMPHASIS_BLUE=0x20,
    SPR_VISIBLE=0x10,
    BG_VISIBLE=0x8,
    SPR_CLIP8=0x4,
    BG_CLIP8=0x2,
    MONOCHROME=0x1
};

enum class PPUADDR {
    TILE_H=0x1F,
    XSCROLL=TILE_H,
    TILE_V=0x3E0,
    YSCROLL=TILE_V,
    NT_H=0x400,
    NT_V=0x800,
    NT=NT_H|NT_V,
    NT_OFFSET=0x3FF,
    YOFFSET=0x7000,
	// XOFFSET is stored in reload register

	FIRST_WRITE_HI=0x3000,
	FIRST_WRITE_MID=0x0C00,
	FIRST_WRITE_LO=0x0300,
    HIGH_BYTE=0x3F00/*0x7F00*/,
    LOW_BYTE=0xFF,

    BANK=0x3000/*0xF000*/,
    BANK_OFFSET=0x0FFF,

	PAL_SELECT=0x10,
	PAL_NUM=0xC,
	PAL_ITEM=0x3
};

enum class SPRATTR : uint8_t {
	COLOR_HI=0x3,
	RESERVED=0x1C,
	PRIORITY=0x20,
	BEHIND_BG=PRIORITY,
	FLIP_H=0x40,
	FLIP_V=0x80
};

// Object/Sprite Attribute Memory
struct NESOAM
{
private:
	struct SPRITE
	{
		uint8_t yminus1; // y coordinate - 1
		uint8_t tile; // tile index number
		SPRATTR attrib; // attributes
		uint8_t x; // x coordinate
	}sprites[64];

public:
	inline SPRITE& sprite(const size_t index)
	{
		vassert(index<64);
		return sprites[index];
	}

	inline uint8_t& data(const size_t ptr)
	{
		vassert(ptr<0x100);
		return ((uint8_t*)this)[ptr];
	}
};

struct NESVRAM
{
	struct VROM
	{
	    struct PATTERN_TABLE
        {
            struct PATTERN // 8px*8px
            {
                uint8_t colorD0[8];
                uint8_t colorD1[8];
            }tiles[256];
        }patternTables[2];
	}vrom;

	struct NAMEATTRIB_TABLE
	{
		struct NAME_TABLE
		{
			uint8_t tiles[30][32];
		}nameTable;
		struct ATTRIBUTE_TABLE
		{
			uint8_t attribs[64];

			// returns the higher 2 bits of the palette index for the specified tile
			uint8_t lookup(const int tileRow, const int tileCol) const
			{
				vassert((unsigned)tileRow<30 && (unsigned)tileCol<32);
				byte_t value=attribs[((tileRow>>2)<<3)+(tileCol>>2)];
				switch ((((tileRow&3)>>1)<<1)|(tileCol&3)>>1)
				{
					case 0:
						value<<=2;
						break;
					case 1:
						break;
					case 2:
						value>>=2;
						break;
					case 3:
						value>>=4;
						break;
				}
				return value&0x0C;
			}
		}attribTable;
	}nameTables[4];

private:
	uint8_t __empty1[0xF00]; // mirrors of 0x2000-0x2eff

public:
	struct PALETTE_MEMORY
	{
		struct PALETTE_COLOR_INDEXES
		{
			// entry #0 is used as background color
			// D6 and D7 of all entries are not used 
			colorindex_t background[0x10];
			colorindex_t sprites[0x10];
		}colors;
	}pal;
	
	inline colorindex_t colorIndex(const size_t index)
	{
		vassert(index<32);
		if ((index&3)==0)
			return pal.colors.background[0]; // due to palette mirroring and backdrop
		else
			return pal.colors.background[index];
	}
	
private:
	uint8_t __empty2[0xE0]; // mirrors of 0x3f00-0x3f1f

public:
	inline uint8_t& data(const size_t ptr)
	{
		vassert(ptr<0x3000 || (ptr>=0x3F00 && ptr<0x3F20));
		return ((uint8_t*)this)[ptr];
	}
};

extern struct NESVRAM vram;
extern struct NESOAM oam;

#define vramPt(ptindex) vram.vrom.patternTables[ptindex]
#define vramNt(ntindex) vram.nameTables[ntindex].nameTable
#define vramAt(ntindex) vram.nameTables[ntindex].attribTable
#define vramData(offset) vram.data(offset)
#define oamData(offset) oam.data(offset)

#define spr(index) oam.sprite(index)
#define colorIdx(index) vram.colorIndex(index)

namespace ppu
{
	// global functions
	void init();
	void reset();

	void bankSwitch(const uint8_t* vrom, const int dest, const int src, const int count);
	bool setup(int mapper_type, const uint8_t* vrom, const size_t vrom_size);

	bool readPort(const maddr_t maddress, byte_t& data);
	bool writePort(const maddr_t maddress, const byte_t data);

	void dma(const uint8_t* src);

	bool hsync();

	int currentScanline();
	long long currentFrame();
}