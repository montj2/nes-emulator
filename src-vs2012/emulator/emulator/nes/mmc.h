struct NESRAM
{
	union
	{
		// $0000 Internal RAM
		uint8_t bank0[0x800];

		struct
		{
			// $0100 Zero-Page Memory
			uint8_t zeropage[0x100];

			// $0100 Stack
			uint8_t stack[0x100];
		};
	};

private:
	// memory space reserved for mirroring/mapping
    uint8_t __bank0_mirror[0x1800];
    uint8_t __io[0x3000];
    uint8_t __ext[0x1000];

public:
	union
	{
		uint8_t code[0xA000];

		struct
		{
			// $6000 SaveRAM
			uint8_t bank6[8192];

			// $8000 PRG-ROM (8K per bank, 4 built-in banks)
			uint8_t bank8[8192], bankA[8192];

			// $C000 Can be mirror of PRG-ROM at $8000
			uint8_t bankC[8192], bankE[8192];
		};
	};

public:
	inline uint8_t& data(const size_t ptr)
	{
		vassert(ptr<0x800 || ptr>=0x6000);
		return ((uint8_t*)this)[ptr];
	}

	inline uint8_t* page(const size_t num)
	{
		vassert(num<0x8 || num>=0x60);
		return &((uint8_t*)this)[num<<8];
	}
};

extern struct NESRAM ram;
#define ramSt ram.stack
#define ram0p ram.zeropage
#define ramPg(num) ram.page(num)
#define ramData(offset) ram.data(offset)

namespace mmc
{
	// global functions
	void reset();

	void bankSwitch(int reg8, int regA, int regC, int regE);

	extern __forceinline opcode_t fetchOpcode(maddr_t& pc);
	extern __forceinline maddr8_t fetchByteOperand(maddr_t& pc);
	extern __forceinline maddr_t fetchWordOperand(maddr_t& pc);
	extern __forceinline byte_t loadZPByte(const maddr8_t zp);
	extern __forceinline word_t loadZPWord(const maddr8_t zp);

	byte_t read(const maddr_t addr);
	void write(const maddr_t addr, const byte_t value);

	// save state
	void save(FILE *fp);
	void load(FILE *fp);
}

namespace mapper
{
	// global functions
	void reset();
	bool setup();

	bool write(const maddr_t addr, const byte_t value);

	bool mmc1Write(const maddr_t addr, const byte_t value);

	byte_t maskPRG(byte_t bank, const byte_t count);

	// save state
	void save(FILE *fp);
	void load(FILE *fp);
}

enum class MMC1REG
{
	NT_MIRRORING=0x3,
	PRG_SWITCH_MODE=0xC,
	VROM_SWITCH_MODE=0x10,
	PRG_BANK=0xF,

	MASK=0x1F
};