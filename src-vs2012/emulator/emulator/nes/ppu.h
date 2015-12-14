// Object/Sprite Attribute Memory
struct NESOAM
{
private:
	struct SPRITE
	{
		uint8_t yminus1; // y coordinate - 1
		uint8_t tile; // tile index number
		uint8_t attrib; // attributes
		uint8_t x; // x coordinate
	}sprites[64];

public:
	inline SPRITE& sprite(const size_t index)
	{
		vassert(index<64);
		return sprites[index];
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
                uint8_t colorlow[8]; // D0
                uint8_t colorhi[8]; // D1
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
		}attribTable;
	}nameTables[4];

private:
	uint8_t __empty1[0xF00]; // mirrors of 0x2000-0x2eff

public:
	struct PALETTE_RAM
	{
		struct PALETTE_COLOR_INDEXES
		{
			colorindex_t background[0x10]; // D6 and D7 are ignored
			colorindex_t sprites[0x10];
		}colors;
	}pal;

	inline colorindex_t* palettes()
	{
		return &pal.colors.background[0];
	}
	
	inline colorindex_t& colorIndex(const size_t index)
	{
		vassert(index<32);
		return palettes()[index];
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

#define vramPt(index) vram.vrom.patternTables[index]
#define vramNt(index) vram.nameTables[index].nameTable
#define vramAt(index) vram.nameTables[index].attribTable
#define vramData(offset) vram.data(offset)

#define spr(index) oam.sprite(index)
#define colorIdx(index) vram.colorIndex(index)
#define universalBackground colorIdx(0)