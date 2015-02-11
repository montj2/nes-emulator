struct NESSPRRAM
{
    uint8_t sprram_data[0];
	struct SPRITE
	{
		uint8_t sprY; // Y coordinate-1
		uint8_t sprTile; // Tile index
		uint8_t sprAttrib; // Attributes
		uint8_t sprX;  // X coordinate
	}sprite[64];
};

struct NESVRAM
{
	uint8_t vram_data[0];
	struct VROM
	{
	    struct PATTERN_TABLE
        {
            struct PATTERN // 8px*8px
            {
                uint8_t colorlow[8]; // D0
                uint8_t colorhi[8]; // D1
            }ptTile[256];
        }patternTable[2];
	}vrom;
	struct NAMEATTRIB_TABLE
	{
		struct NAME_TABLE
		{
			uint8_t tile[30][32];
		}nameTable;
		struct ATTRIB_TABLE
		{
			uint8_t attrib[64];
		}attribTable;
	}name_attrib[4];
	uint8_t __empty[0xF00]; // mirrored to 2000
	uint8_t pal_data[0];
	colorindex_t palBackground[0x10]; // D6 and D7 are ignored
	colorindex_t palSprite[0x10];
	uint8_t __empty2[0xE0]; // mirrored to 3000
};

extern struct NESVRAM vram;
extern struct NESSPRRAM spriteMem;
#define sprite spriteMem.sprite
#define patternTable(index) vram.vrom.patternTable[index]
#define nameTable(index)  vram.name_attrib[index].nameTable
#define attribTable(index) vram.name_attrib[index].attribTable

void PpuTests();
void PpuSelfTest();

int PpuCopyBanks(int dest,int src,int count);
void PpuReset();

byte_t PpuLoadReg(const maddr_t maddress);
void PpuWriteReg(const maddr_t maddress,const byte_t data);
void PpuWriteSramDMA(const void* src);
void PpuLoadPal();
bool PpuHSync();
