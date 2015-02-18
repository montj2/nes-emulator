struct NESRAM
{
    uint8_t ram_data[0];
    uint8_t ram_page[0][0x100];
	union
	{
		uint8_t bank0[0x800]; // $0000 Internal RAM
		struct
		{
			uint8_t zeropage[0x100];
			uint8_t stack[0x100]; // $0100 Stack
		};
	};
    uint8_t __bank0_mirror[0x1800];
    uint8_t __io[0x3000]; // don't access directly
    uint8_t __ext[0x1000];
    uint8_t bank6[8192]; // $6000 SaveRAM
    uint8_t bank8[8192],bankA[8192]; // $8000 PRG-ROM
    uint8_t bankC[8192],bankE[8192]; // C8000 Mirror of PRG-ROM
};

extern struct NESRAM ram;
#define stack ram.stack
#define zeropage ram.zeropage

void MmcSelfTest();

void MmcSetupBanks(const char* gameImage);
int MmcBankSwitch(int dest,int src,int count);
void MmcReset();
int MmcSetup(uint8_t mapperType,const void* gameImage);
void write6502(const maddr_t Address,const byte_t Value);
byte_t read6502(const maddr_t Address);
byte_t readCode(const maddr_t pc);
byte_t loadOperand(const maddr_t pc);
word_t loadOperand16bit(const maddr_t pc);
byte_t loadZp(const addr8_t zp);
word_t loadZp16bit(const addr8_t zp);
