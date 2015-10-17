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
	// $6000 SaveRAM
    uint8_t bank6[8192];

	// $8000 PRG-ROM
    uint8_t bank8[8192], bankA[8192];

	// $C000 Mirror of PRG-ROM
    uint8_t bankC[8192], bankE[8192];

public:
	inline uint8_t& data(const size_t ptr)
	{
		vassert(ptr<0x800 || ptr>=0x6000);
		return *((uint8_t*)this+ptr);
	}

	inline uint8_t* page(const size_t num)
	{
		return (uint8_t*)this+(num<<8);
	}
};

extern struct NESRAM ram;
#define ramSt ram.stack
#define ram0p ram.zeropage

namespace mmc
{
	// global functions
	void reset();

	void bankSwitch(int reg8, int regA, int regC, int regE, const uint8_t* image);
	bool setup(int mapper_type, const uint8_t* image, const size_t image_size);

	opcode_t fetchOpcode(const maddr_t pc);
	byte_t fetchByteOperand(const maddr_t addr);
	word_t fetchWordOperand(const maddr_t addr);
	byte_t loadZPByte(const addr8_t zp);
	word_t loadZPWord(const addr8_t zp);

	byte_t read(const maddr_t addr);
	void write(const maddr_t addr, const byte_t value);
}