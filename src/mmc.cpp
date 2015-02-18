#include "macro.h"
#include "datatype.h"
#include "rominfo.h"
#include "mmc.h"
#include "ppu.h"

struct NESRAM ram;

int reg8,regA,regC,regE;
int p8,pA,rC,pE; // addresses of currently selected prg-rom banks.
int prevBSSrc[8]; // used to ensure that it doesn't bankswitch when the correct bank is already selected

void MmcSetupBanks(const char* gameImage)
{
    if (p8!=reg8) memcpy(ram.bank8,&gameImage[reg8*0x2000],0x2000);
    if (pA!=regA) memcpy(ram.bankA,&gameImage[regA*0x2000],0x2000);
    if (rC!=regC) memcpy(ram.bankC,&gameImage[regC*0x2000],0x2000);
    if (pE!=regE) memcpy(ram.bankE,&gameImage[regE*0x2000],0x2000);
    p8=reg8;pA=regA;rC=regC;
    pE=regE;
}

int MmcBankSwitch(int dest,int src,int count)
{
    for (int i=0; i<count; i++)
    {
        if (prevBSSrc[dest+i]!=src+i)
        {
            prevBSSrc[dest+i]=src+i;
            PpuCopyBanks(dest+i,src+i,1);
        }
    }
    return 1;
}

void MmcReset()
{
    p8=pA=rC=pE=-1;
    memset(prevBSSrc,-1,sizeof(prevBSSrc));
    memset(&ram,0,sizeof(ram));
    // TODO: FILL WITH 0XCC
}

int MmcSetup(uint8_t mapperType,const void* gameImage)
{
    switch (mapperType)
    {
    case 0: // Direct Access
        MmcBankSwitch(0,0,8);
        reg8=0;
        regA=1;
        regC=2;
        regE=3;
        MmcSetupBanks((const char*)gameImage);
        return 1;
    default:
        return 0;
    }
}

byte_t readCode(const maddr_t pc)
{
    #ifdef DEP
        assert((valueOf(pc)>>15)==1); // $8000<=PC<=$FFFF
    #endif
	#ifdef NDEBUG
        return ram.bank8[pc-0x8000];
    #else
        return read6502(pc);
    #endif
}

byte_t loadOperand(const maddr_t pc)
{
    #ifdef DEP
        assert((valueOf(pc)>>15)==1);
	#endif
	return ram.ram_data[valueOf(pc)];
}

word_t loadOperand16bit(const maddr_t pc)
{
    #ifdef DEP
        assert((valueOf(pc)>>15)==1);
    #endif
    assert(pc.isNotMax());
	return *(uint16_t*)&(ram.ram_data[valueOf(pc)]);
}

byte_t loadZp(const addr8_t zp)
{
	return zeropage[valueOf(zp)];
}

word_t loadZp16bit(const addr8_t zp)
{
    #ifndef ALLOW_ADDRESS_WRAP
        assert(zp.isNotMax());
    #else
        if (zp.isMax())
        {
            return ( ((uint16_t)zeropage[0])<<8)|zeropage[valueOf(zp)];
        }
    #endif
	return *(uint16_t*)&(zeropage[valueOf(zp)]);
}


uint8_t joystick1_pos;

byte_t read6502(const maddr_t Address)
{
    switch (valueOf(Address)>>13)
    {
    case 0: //[$0000,$2000)
        return ram.bank0[valueOf(Address)&0x7FF];
    case 1: //[$2000,$4000) PPU Registers
		return PpuLoadReg(Address);
        break;
    case 2: //[$4000,$6000)
        return 0;
        break;

#ifdef NDEBUG
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        return ram.ram_data[Address];
#else
    case 3: //[$6000,$8000)
        return ram.bank6[valueOf(Address)&0x1FFF];
    case 4: //[$8000,$A000)
        return ram.bank8[valueOf(Address)&0x1FFF];
    case 5: //[$A000,$C000)
        return ram.bankA[valueOf(Address)&0x1FFF];
    case 6: //[$C000,$E000)
        return ram.bankC[valueOf(Address)&0x1FFF];
    case 7: //[$E000,$FFFF]
        return ram.bankE[valueOf(Address)&0x1FFF];
#endif
    }
    assert(0);
    return 0;
}

static void mapperWrite(const maddr_t Address,const byte_t Value)
{
	fprintf(stderr,"[MMC] It's illegal to write to $%04X [Value=%02X]\n",valueOf(Address),Value);
	assert(0);
}

void write6502(const maddr_t Address,const byte_t Value)
{
    #ifdef MONITOR_RAM_IO
        printf("* Write %X to $%X *\n",Value,valueOf(Address));
    #endif
	switch (valueOf(Address)>>13)
    {
    case 0: //[$0000,$2000) Internal RAM
        ram.bank0[valueOf(Address)&0x7FF]=Value;
		break;
    case 1: //[$2000,$4000) PPU Registers
		PpuWriteReg(Address,Value);
		break;
    case 3: //[$6000,$8000) SRAM
        // if (specialwrite6000)
		ram.bank6[valueOf(Address)&0x1FFF]=Value;
		break;
    case 4: //[$8000,$A000)
    case 5: //[$A000,$C000)
    case 6: //[$C000,$E000)
    case 7: //[$E000,$FFFF]
		mapperWrite(Address,Value);
		break;
    case 2: //[$4000,$6000) Other Registers
        switch (valueOf(Address)) {
            case 0x4014:
                PpuWriteSramDMA(&ram.ram_page[Value][0]);
                break;
            case 0x4016:
            case 0x4017:
                // printf("[IOReg] Write to 0x%04x: %x\n",valueOf(Address),Value);
                if (!(Value&1)) // reset joystick
                {
                    joystick1_pos=0;
                }
                break;
        }
        break;
    default:
        assert(0);
        break;
    }
}

void MmcSelfTest()
{
    static_assert(sizeof(ram)==0x10000,"RAM struct error");
    static_assert(&ram.bank6[0]-&ram.ram_data[0]==0x6000,"RAM struct error");

    printf("[MMC] System memory at 0x%p\n",&ram.ram_data[0]);
}
