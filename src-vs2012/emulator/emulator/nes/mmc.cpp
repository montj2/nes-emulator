#include "../stdafx.h"

// local header files
#include "../macros.h"
#include "../types/types.h"
#include "../unittest/framework.h"

#include "internals.h"
#include "debug.h"
#include "rom.h"
#include "mmc.h"
#include "ppu.h"

#include "../ui.h"

// NES main memory
__declspec(align(0x1000))
struct NESRAM ram;

// addresses of currently selected prg-rom banks.
static int p8, pA, pC, pE;

namespace mmc
{
	static void updateBank(uint8_t * const dest, int& prev, int current)
	{
		// first mask bank the address
		current = mapper::maskPRG(current, rom::countPRG()*2);
		assert(current<rom::countPRG()*2);

		if (current!=prev)
		{
			// perform update
			memcpy(dest, rom::getImage()+current*0x2000, 0x2000);
			current=prev;
		}
	}

	// perform bank switching
	void bankSwitch(int reg8, int regA, int regC, int regE)
	{
		if (reg8!=INVALID) updateBank(ram.bank8, p8, reg8);
		if (regA!=INVALID) updateBank(ram.bankA, pA, regA);
		if (regC!=INVALID) updateBank(ram.bankC, pC, regC);
		if (regE!=INVALID) updateBank(ram.bankE, pE, regE);
	}

	bool setup()
	{
		switch (rom::mapperType())
		{
		case 0: // no mapper
		case 1: // Mapper 1: MMC1
		case 2: // Mapper 2: UNROM - PRG/16K
		case 3: // Mapper 3: CNROM - VROM/8K
			if (rom::sizeOfImage() >= 0x8000) // 32K of code or more
				bankSwitch(0, 1, 2, 3);
			else if (rom::sizeOfImage() == 0x4000) // 16K of code
				bankSwitch(0, 1, 0, 1);
			else
			{
				ERROR(INVALID_MEMORY_ACCESS, MAPPER_FAILURE);
				return false;
			}
			return true;
		}
		// unknown mapper
		FATAL_ERROR(INVALID_ROM, UNSUPPORTED_MAPPER_TYPE, "mapper", rom::mapperType());
		return false;
	}

	void reset()
	{
		// no prg-rom selected now
		p8=INVALID;
		pA=INVALID;
		pC=INVALID;
		pE=INVALID;

		// clear memory
		memset(&ram,0,sizeof(ram));
	}

	void save(FILE *fp)
	{
		// bank-switching state
		fwrite(&p8, sizeof(p8), 1, fp);
		fwrite(&pA, sizeof(pA), 1, fp);
		fwrite(&pC, sizeof(pC), 1, fp);
		fwrite(&pE, sizeof(pE), 1, fp);

		// code&data in memory
		fwrite(ram.bank0, sizeof(ram.bank0), 1, fp);
		fwrite(ram.code, sizeof(ram.code), 1, fp);
	}
	
	void load(FILE *fp)
	{
		// bank-switching state
		fread(&p8, sizeof(p8), 1, fp);
		fread(&pA, sizeof(pA), 1, fp);
		fread(&pC, sizeof(pC), 1, fp);
		fread(&pE, sizeof(pE), 1, fp);

		// code&data in memory
		fread(ram.bank0, sizeof(ram.bank0), 1, fp);
		fread(ram.code, sizeof(ram.code), 1, fp);
	}

	opcode_t fetchOpcode(maddr_t& pc)
	{
		opcode_t opcode;
#ifdef WANT_MEM_PROTECTION
		// check if address in code section [$8000, $FFFF]
		FATAL_ERROR_UNLESS(MSB(pc), INVALID_MEMORY_ACCESS, MEMORY_NOT_EXECUTABLE, "PC", valueOf(pc));
		opcode = ram.bank8[pc-0x8000];
#else
		WARN_IF(!MSB(pc), INVALID_MEMORY_ACCESS, MEMORY_NOT_EXECUTABLE, "PC", valueOf(pc));
		opcode = ram.data(pc);
#endif
		inc(pc);
		return opcode;
	}

	operandb_t fetchByteOperand(maddr_t& pc)
	{
		operandb_t operand;
#ifdef WANT_MEM_PROTECTION
		operand(ram.bank8[pc-0x8000]);
#else
		operand(ram.data(pc));
#endif
		inc(pc);
		return operand;
	}

	operandw_t fetchWordOperand(maddr_t& pc)
	{
		operandw_t operand;
		FATAL_ERROR_IF(pc.reachMax(), INVALID_MEMORY_ACCESS, ILLEGAL_ADDRESS_WARP);
#ifdef WANT_MEM_PROTECTION
		operand(*(uint16_t*)&ram.bank8[pc-0x8000]);
#else
		operand(makeWord(ram.data(pc), ram.data(pc+1)));
#endif
		pc+=2;
		return operand;
	}

	byte_t loadZPByte(const maddr8_t zp)
	{
		return ram0p[zp];
	}

	word_t loadZPWord(const maddr8_t zp)
	{
#ifndef ALLOW_ADDRESS_WRAP
		FATAL_ERROR_IF(zp.reachMax(), INVALID_MEMORY_ACCESS, ILLEGAL_ADDRESS_WARP);
#else
		if (zp.reachMax())
		{
			 return (((word_t)ram0p[0])<<8)|ram0p[zp];
		}
#endif
		return *(uint16_t*)&ram0p[zp];
	}

	byte_t read(const maddr_t addr)
	{
		byte_t ret=INVALID;
		switch (addr>>13) // bank number/2
		{
		case 0: //[$0000,$2000)
			return ram.bank0[addr&0x7FF];
		case 1: //[$2000,$4000)
			if (ppu::readPort(addr, ret)) return ret;
			break;
		case 2: //[$4000,$6000)
			switch (valueOf(addr))
			{
			case 0x4015: // APU Register
				return 0;
			case 0x4016: // Input Registers
			case 0x4017:
				if (ui::hasInput((addr==0x4017)?1:0))
					return ui::readInput((addr==0x4017)?1:0); // outputs button state
				else
					return 0; // joystick not connected
			}
			break;
		case 3:
			return ram.bank6[addr&0x1FFF];
		case 4:
		case 5:
		case 6:
		case 7:
			return ram.bank8[addr-0x8000];
		}
		ERROR(INVALID_MEMORY_ACCESS, MEMORY_CANT_BE_READ, "addr", valueOf(addr));
		return ret;
	}

	void write(const maddr_t addr, const byte_t value)
	{
		switch (addr>>13) // bank number/2
		{
			case 0: //[$0000,$2000) Internal RAM
				ram.bank0[addr&0x7FF]=value;
				return;
			case 1: //[$2000,$4000) PPU Registers
				if (ppu::writePort(addr, value)) return;
				break;
			case 3: //[$6000,$8000) SRAM
				ram.bank6[addr&0x1FFF]=value;
				return;
			case 4: //[$8000,$A000)
			case 5: //[$A000,$C000)
			case 6: //[$C000,$E000)
			case 7: //[$E000,$FFFF]
				//  mapper write
				if (mapper::write(addr, value)) return;
				break;
			case 2: //[$4000,$6000) Other Registers
				switch (valueOf(addr))
				{
				// SPR-RAM DMA Pointer Register
				case 0x4014:
					ERROR_UNLESS(value<0x8, INVALID_MEMORY_ACCESS, MEMORY_CANT_BE_COPIED, "page", value);
					ppu::dma(ramPg(value));
					return;
				// Input Registers 
				case 0x4016:
				case 0x4017:
					if (!(value&1))
					{
						ui::resetInput();
					}
					return;
				}
				if (addr>=0x4000 && addr<=0x4017)
				{
					// APU Registers
					return;
				}
				break;
		}
		ERROR(INVALID_MEMORY_ACCESS, MEMORY_CANT_BE_WRITTEN, "addr", valueOf(addr), "value", value);
	}
}

namespace mapper
{
	byte_t maskPRG(byte_t bank, const byte_t count)
	{
		assert(count!=0);
		if (bank<count)
			return bank;
		else
		{
			byte_t m=0xFF;
			for (m=0xFF;(bank&m)>=count;m>>=1);
			return bank&m;
		}
	}

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

	static void select16KROM(const byte_t value)
	{
		mmc::bankSwitch((value<<1), (value<<1)+1, INVALID, INVALID);
	}

	template <int CHRSize>
	static void selectVROM(const byte_t value, const byte_t bank)
	{
		ppu::bankSwitch(
			bank*CHRSize,
			maskCHR(value, rom::countCHR()*(8/CHRSize))*CHRSize,
			CHRSize
			);
	}

	static void select8KVROM(const byte_t value)
	{
		selectVROM<8>(value, 0);
	}

	bool write(const maddr_t addr, const byte_t value)
	{
		switch (rom::mapperType())
		{
		case 0: // no mapper
			return false;
		case 2: // Mapper 2: Select 16K ROM
			select16KROM(value);
			return true;
		case 3: // Mapper 3: Select 8K VROM
			select8KVROM(value&3);
			return true;
		}
		ERROR(INVALID_MEMORY_ACCESS, MAPPER_FAILURE, "addr", valueOf(addr), "value", value, "mapper", rom::mapperType());
		return false;
	}
}

// unit tests
class MMCTest : public TestCase
{
public:
	virtual const char* name()
	{
		return "MMC Unit Test";
	}

	virtual TestResult run()
	{
		puts("checking RAM struture...");
		tassert(sizeof(ram)==0x10000);
		tassert(ptr_diff(&ram.bank6[0],&ramData(0))==0x6000);
		tassert(ptr_diff(ram.page(0x80),&ramData(0))==0x8000);

		// mapper test
		tassert(mapper::maskPRG(0,4)==0);
		tassert(mapper::maskPRG(3,4)==3);
		tassert(mapper::maskPRG(5,4)==1);
		tassert(mapper::maskPRG(5,3)==1);
		tassert(mapper::maskPRG(6,3)==2);
		tassert(mapper::maskPRG(31,16)==15);

		tassert(mapper::maskCHR(0,4)==0);
		tassert(mapper::maskCHR(3,4)==3);
		tassert(mapper::maskCHR(5,4)==1);
		tassert(mapper::maskCHR(5,3)==1);
		tassert(mapper::maskCHR(5,2)==1);
		tassert(mapper::maskCHR(6,6)==5);
		tassert(mapper::maskCHR(6,7)==6);
		tassert(mapper::maskCHR(32,16)==0);

		printf("[ ] System memory at 0x%p\n", &ram);
		return SUCCESS;
	}
};

registerTestCase(MMCTest);