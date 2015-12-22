#include "../stdafx.h"

// local header files
#include "../macros.h"
#include "../types/types.h"
#include "../unittest/framework.h"

#include "internals.h"
#include "debug.h"
#include "rom.h"
#include "mmc.h"
#include "cpu.h"
#include "ppu.h"

#include "../ui.h"

// NES main memory
__declspec(align(0x1000))
struct NESRAM ram;

namespace mmc
{
	// addresses of currently selected prg-rom banks.
	static int p8, pA, pC, pE;

	static bool sramEnabled;

	static void updateBank(uint8_t * const dest, int& prev, int current)
	{
		// first mask bank the address
		current = mapper::maskPRG(current, rom::count8KPRG());
		assert(current<rom::count8KPRG());

		if (current!=prev)
		{
			// perform update
			memcpy(dest, rom::getImage()+current*0x2000, 0x2000);
			prev=current;
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

	void setSRAMEnabled(bool v)
	{
		sramEnabled=v;
	}

	void reset()
	{
		// no prg-rom selected now
		p8=INVALID;
		pA=INVALID;
		pC=INVALID;
		pE=INVALID;

		// SRAM is disabled by default
		sramEnabled = false;

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

		// data in memory
		fwrite(ram.bank0, sizeof(ram.bank0), 1, fp);
		fwrite(ram.bank6, sizeof(ram.bank6), 1, fp);

#ifdef SAVE_COMPLETE_MEMORY
		// code in memory
		fwrite(ram.code, sizeof(ram.code), 1, fp);
#endif
	}
	
	void load(FILE *fp)
	{
		// bank-switching state
		int r8, rA, rC, rE;
		fread(&r8, sizeof(r8), 1, fp);
		fread(&rA, sizeof(rA), 1, fp);
		fread(&rC, sizeof(rC), 1, fp);
		fread(&rE, sizeof(rE), 1, fp);

		// data in memory
		fread(ram.bank0, sizeof(ram.bank0), 1, fp);
		fread(ram.bank6, sizeof(ram.bank6), 1, fp);

#ifdef SAVE_COMPLETE_MEMORY
		// code in memory
		fwrite(ram.code, sizeof(ram.code), 1, fp);
		p8 = r8;
		pA = rA;
		pC = rC;
		pE = rE;
#else
		// restore code
		bankSwitch(r8, rA, rC, rE);
#endif
	}

	opcode_t fetchOpcode(maddr_t& pc)
	{
		opcode_t opcode;
#ifdef WANT_MEM_PROTECTION
		// check if address in code section [$8000, $FFFF]
		FATAL_ERROR_UNLESS(valueOf(pc)>=0x6000, INVALID_MEMORY_ACCESS, MEMORY_NOT_EXECUTABLE, "PC", valueOf(pc));
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
	// MMC1 registers
	static ioreg_t mmc1Sel; // register select
	static ioreg_t mmc1Pos;
	static ioreg_t mmc1Tmp;
	static flag_set<ioreg_t, MMC1REG, 5> mmc1Regs[4];
	#define mmc1Cfg mmc1Regs[0]

	// MMC3 registers
	static ioreg_t mmc3Control;
	static ioreg_t mmc3Cmd;
	static ioreg_t mmc3Data;
	static ioreg_t mmc3Counter;
	static ioreg_t mmc3Latch;
	static bool mmc3IRQ;

	void reset()
	{
		// MMC1
		mmc1Sel=INVALID;
		mmc1Tmp=0;
		mmc1Pos=0;
		for (int i=0; i<4; i++)
		{
			mmc1Regs[i].clearAll();
		}

		// MMC3
		mmc3Control=INVALID;
		mmc3Cmd=0;
		mmc3Data=0;
		mmc3Counter=0;
		mmc3Latch=INVALID;
		mmc3IRQ=false;
	}

	void load(FILE* fp)
	{
		fread(&mmc1Sel, sizeof(mmc1Sel), 1, fp);
		fread(&mmc1Pos, sizeof(mmc1Pos), 1, fp);
		fread(&mmc1Tmp, sizeof(mmc1Tmp), 1, fp);
		fread(&mmc1Regs, sizeof(mmc1Regs), 1, fp);

		fread(&mmc3Control, sizeof(mmc3Control), 1, fp);
		fread(&mmc3Cmd, sizeof(mmc3Cmd), 1, fp);
		fread(&mmc3Data, sizeof(mmc3Data), 1, fp);
		fread(&mmc3Counter, sizeof(mmc3Counter), 1, fp);
		fread(&mmc3Latch, sizeof(mmc3Latch), 1, fp);
		fread(&mmc3IRQ, sizeof(mmc3IRQ), 1, fp);
	}

	void save(FILE* fp)
	{
		fwrite(&mmc1Sel, sizeof(mmc1Sel), 1, fp);
		fwrite(&mmc1Pos, sizeof(mmc1Pos), 1, fp);
		fwrite(&mmc1Tmp, sizeof(mmc1Tmp), 1, fp);
		fwrite(&mmc1Regs, sizeof(mmc1Regs), 1, fp);

		fwrite(&mmc3Control, sizeof(mmc3Control), 1, fp);
		fwrite(&mmc3Cmd, sizeof(mmc3Cmd), 1, fp);
		fwrite(&mmc3Data, sizeof(mmc3Data), 1, fp);
		fwrite(&mmc3Counter, sizeof(mmc3Counter), 1, fp);
		fwrite(&mmc3Latch, sizeof(mmc3Latch), 1, fp);
		fwrite(&mmc3IRQ, sizeof(mmc3IRQ), 1, fp);
	}

	bool setup()
	{
		switch (rom::mapperType())
		{
		case 0: // no mapper
		case 2: // Mapper 2: UNROM - PRG/16K
		case 3: // Mapper 3: CNROM - VROM/8K
		case 7: // Mapper 7: AOROM - PRG/32K, Name Table Select
			if (rom::sizeOfImage() >= 0x8000) // 32K of code or more
				mmc::bankSwitch(0, 1, 2, 3);
			else if (rom::sizeOfImage() == 0x4000) // 16K of code
				mmc::bankSwitch(0, 1, 0, 1);
			else
			{
invalidROMSize:
				ERROR(INVALID_MEMORY_ACCESS, MAPPER_FAILURE, "image size", rom::sizeOfImage());
				return false;
			}
			return true;		
		case 1: // Mapper 1: MMC1
			if (rom::count16KPRG()>=1 && rom::count16KPRG()<=16) // at least 16K of code, at most 256K of code
			{
				mmc::bankSwitch(0, 1, rom::count8KPRG()-2, rom::count8KPRG()-1);
				return true;
			}
			goto invalidROMSize;
		case 4: // Mapper 4: MMC3 - PRG/8K, VROM/2K/1K, VT, SRAM, IRQ
			if (rom::count8KPRG()>=2)
			{
				mmc::bankSwitch(0, 1, rom::count8KPRG()-2, rom::count8KPRG()-1);
				return true;
			}
			goto invalidROMSize;
		}
		// unknown mapper
		FATAL_ERROR(INVALID_ROM, UNSUPPORTED_MAPPER_TYPE, "mapper", rom::mapperType());
		return false;
	}

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

	static void selectFirst16KROM(const byte_t value) // for Mapper 2
	{
		ERROR_IF(((int)value+1)*2>=rom::count8KPRG(), INVALID_MEMORY_ACCESS, MAPPER_FAILURE, "bankSize", 16, "num", value);
		mmc::bankSwitch((value<<1), (value<<1)+1, INVALID, INVALID);
	}

	static void select32KROM(const byte_t value) // for MMC1
	{
		ERROR_UNLESS((int)value*4<rom::count8KPRG(), INVALID_MEMORY_ACCESS, MAPPER_FAILURE, "bankSize", 32, "num", value);
		mmc::bankSwitch((value<<2), (value<<2)+1, (value<<2)+2, (value<<2)+3);
	}

	bool write(const maddr_t addr, const byte_t value)
	{
		switch (rom::mapperType())
		{
		case 0: // no mapper
			return false;
		case 1: // Mapper 1:
			return mmc1Write(addr, value);
		case 2: // Mapper 2: Select 16K ROM
			selectFirst16KROM(value);
			return true;
		case 3: // Mapper 3: Select 8K VROM
			pmapper::select8KVROM(value&3);
			return true;
		case 4: // MMC3:
			return mmc3Write(addr, value);
		case 7: // Mapper 7: Select 32K ROM & Name Table Select
			select32KROM(value&7);
			rom::setMirrorMode((value&16)?MIRRORING::HSINGLESCREEN:MIRRORING::LSINGLESCREEN);
			return true;
		}
		FATAL_ERROR(INVALID_MEMORY_ACCESS, MAPPER_FAILURE, "addr", valueOf(addr), "value", value, "mapper", rom::mapperType());
		return false;
	}

	void HBlank()
	{
		switch (rom::mapperType())
		{
			case 4: // MMC3:
				mmc3HBlank();
				break;
		}
	}

	// various mapper handlers
	static void mmc1Apply(const byte_t sel)
	{
		byte_t value=mmc1Regs[sel](MMC1REG::MASK);
		switch (sel)
		{
		case 0: // Configuration Register
			// configure mirroring
			switch (mmc1Cfg(MMC1REG::NT_MIRRORING))
			{
			case 0:
				rom::setMirrorMode(MIRRORING::LSINGLESCREEN);
				break;
			case 1:
				rom::setMirrorMode(MIRRORING::HSINGLESCREEN);
				break;
			case 2:
				rom::setMirrorMode(MIRRORING::VERTICAL);
				break;
			case 3:
				rom::setMirrorMode(MIRRORING::HORIZONTAL);
				break;
			}
			break;

		case 1: // Select 4K or 8K VROM bank at 0000h
			if (rom::count8KCHR()>0)
			{
				if (mmc1Cfg[MMC1REG::VROM_SWITCH_MODE])
				{
					// 4K
					ERROR_IF((int)value>=rom::count4KCHR(), INVALID_MEMORY_ACCESS, MAPPER_FAILURE, "bankSize", 4, "num", value);
					pmapper::selectVROM(4, value, 0);
				}else
				{
					// 8K
					ERROR_IF((int)value>=rom::count8KCHR(), INVALID_MEMORY_ACCESS, MAPPER_FAILURE, "bankSize", 8, "num", value);
					pmapper::select8KVROM(value);
				}
			}
			break;

		case 2: // Select 4K VROM bank at 1000h (4K mode only)
			if (rom::count8KCHR()>0)
			{
				FATAL_ERROR_IF(mmc1Cfg[MMC1REG::VROM_SWITCH_MODE], ILLEGAL_OPERATION, MAPPER_FAILURE, "mmc1reg2", value);
				ERROR_IF((int)value>=rom::count4KCHR(), INVALID_MEMORY_ACCESS, MAPPER_FAILURE, "bankSize", 4, "num", value);
				pmapper::selectVROM(4, value, 1);
			}
			break;

		case 3: // Select 16K or 2x16K ROM bank
			value=mmc1Regs[sel](MMC1REG::PRG_BANK);
			switch (mmc1Cfg(MMC1REG::PRG_SWITCH_MODE))
			{
			case 0:
			case 1: // Switchable 32K Area at 8000h-FFFFh
				select32KROM(value);
				break;
			case 2: // Switchable 16K Area at C000h-FFFFh
				mmc::bankSwitch(0, 1, (value<<1), (value<<1)+1);
				break;
			case 3: // Switchable 16K Area at 8000h-BFFFh
				mmc::bankSwitch((value<<1), (value<<1)+1, rom::count8KPRG()-2, rom::count8KPRG()-1);
				break;
			}
			break;
		}
	}

	bool mmc1Write(const maddr_t addr, const byte_t value)
	{
		// And I hope the address is multiple of 0x2000.
		vassert(0==(addr&0x1FFF));

		const byte_t i=(valueOf(addr)>>13)&3;
		vassert(i<4);
		assert(mmc1Sel==INVALID || mmc1Sel==i);
		mmc1Sel=i;

		if (value&0x80)
		{
			// clear shift register
			mmc1Regs[0].asBitField()=valueOf(mmc1Regs[0])|0xC; // ?
			mmc1Pos=0;
			mmc1Tmp=0;
			mmc1Sel=INVALID;
			return true;
		}else
		{
			// D1-D7 had better be zero.
			//vassert(0==(value&0xFE));

			// serial load (LSB first)
			vassert(mmc1Pos<5);
			mmc1Tmp&=~(1<<mmc1Pos);
			mmc1Tmp|=((value&1)<<mmc1Pos);

			// increase position
			mmc1Pos++;
			if (mmc1Pos==5)
			{
				// fifth write
				// copy to selected register
				mmc1Regs[i].asBitField()=mmc1Tmp;
				mmc1Apply(mmc1Sel);
				// reset
				mmc1Pos=0;
				mmc1Tmp=0; // optional
				mmc1Sel=INVALID;
			}
			return true;
		}
		return false;
	}

	static void mmc3Apply()
	{
		FATAL_ERROR_IF(mmc3Control==INVALID, ILLEGAL_OPERATION, MAPPER_FAILURE, "mmc3data", mmc3Data);
		
		const bool CHRSelect=(mmc3Control&0x80)==0x80;
		const bool PRGSelect=(mmc3Control&0x40)==0x40;

		switch (mmc3Cmd) // Command Number
		{
		case 0: // Select 2x1K VROM at PPU 0000h-07FFh
			//assert(0==(mmc3Data&1));
			//pmapper::selectVROM(2, mmc3Data>>1, CHRSelect?2:0); 
			pmapper::selectVROM(1, mmc3Data, CHRSelect?4:0);  
			pmapper::selectVROM(1, mmc3Data+1, CHRSelect?5:1);
			break;
		case 1: // Select 2x1K VROM at PPU 0800h-0FFFh
			//assert(0==(mmc3Data&1));
			//pmapper::selectVROM(2, mmc3Data>>1, CHRSelect?3:1); 
			pmapper::selectVROM(1, mmc3Data, CHRSelect?6:2); 
			pmapper::selectVROM(1, mmc3Data+1, CHRSelect?7:3); 
			break;
		case 2: // Select 1K VROM at PPU 1000h-13FFh
			pmapper::selectVROM(1, mmc3Data, CHRSelect?0:4);
			break;
		case 3: // Select 1K VROM at PPU 1400h-17FFh
			pmapper::selectVROM(1, mmc3Data, CHRSelect?1:5);
			break;
		case 4: // Select 1K VROM at PPU 1800h-1BFFh
			pmapper::selectVROM(1, mmc3Data, CHRSelect?2:6);
			break;
		case 5: // Select 1K VROM at PPU 1C00h-1FFFh
			pmapper::selectVROM(1, mmc3Data, CHRSelect?3:7);
			break;
		case 6: // Select 8K ROM at 8000h-9FFFh
			if (!PRGSelect)
				mmc::bankSwitch(mmc3Data, INVALID, rom::count8KPRG()-2, rom::count8KPRG()-1);
			else
				mmc::bankSwitch(rom::count8KPRG()-2, INVALID, mmc3Data, rom::count8KPRG()-1);
			break;
		case 7: // Select 8K ROM at A000h-BFFFh
			if (!PRGSelect)
				mmc::bankSwitch(INVALID, mmc3Data, rom::count8KPRG()-2, rom::count8KPRG()-1);
			else
				mmc::bankSwitch(rom::count8KPRG()-2, mmc3Data, INVALID, rom::count8KPRG()-1);
			break;
		}
	}

	bool mmc3Write(const maddr_t addr, const byte_t value)
	{
		switch (valueOf(addr))
		{
		case 0x8000: // Index/Control (5bit)
			mmc3Cmd=value&7;
			mmc3Control=value;
			return true;
		case 0x8001: // Data Register
			mmc3Data=value;
			mmc3Apply();
			return true;
		case 0xA000: // Mirroring Select
			rom::setMirrorMode((value&1)?MIRRORING::HORIZONTAL:MIRRORING::VERTICAL);
			return true;
		case 0xA001: // SaveRAM Toggle
			mmc::setSRAMEnabled((value&1)==1);
			return true;
		case 0xC000: // IRQ Counter Register
			mmc3Counter=value;
			return true;
		case 0xC001: // IRQ Latch Register
			mmc3Latch=value;
			return true;
		case 0xE000: // IRQ Control Register 0
			mmc3IRQ=false;
			mmc3Counter=mmc3Latch;
			return true;
		case 0xE001: // IRQ Control Register 1
			mmc3IRQ=true;
			return true;
		}
		return false;
	}

	void mmc3HBlank()
	{
		if (ppu::currentScanline()==-1)
			mmc3Counter=mmc3Latch;
		else if (ppu::currentScanline()>=0 && ppu::currentScanline()<=239)
		{
			if (mmc3IRQ && render::enabled())
			{
				mmc3Counter=(mmc3Counter-1)&0xFF;
				if (!mmc3Counter)
				{
					cpu::irq(IRQTYPE::IRQ);
				}
			}
		}
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
		tassert(&ram.code[0]+sizeof(ram.code)==&ramData(0)+sizeof(ram));

		// mapper test
		tassert(mapper::maskPRG(0,4)==0);
		tassert(mapper::maskPRG(3,4)==3);
		tassert(mapper::maskPRG(5,4)==1);
		tassert(mapper::maskPRG(5,3)==1);
		tassert(mapper::maskPRG(6,3)==2);
		tassert(mapper::maskPRG(31,16)==15);

		tassert(pmapper::maskCHR(0,4)==0);
		tassert(pmapper::maskCHR(3,4)==3);
		tassert(pmapper::maskCHR(5,4)==1);
		tassert(pmapper::maskCHR(5,3)==1);
		tassert(pmapper::maskCHR(5,2)==1);
		tassert(pmapper::maskCHR(6,6)==5);
		tassert(pmapper::maskCHR(6,7)==6);
		tassert(pmapper::maskCHR(32,16)==0);

		printf("[ ] System memory at 0x%p\n", &ram);
		return SUCCESS;
	}
};

registerTestCase(MMCTest);