#include "macro.h"
#include "datatype.h"
#include "rominfo.h"
#include "mmc.h"
#include "ppu.h"

struct NESRAM ram;

int reg8,regA,regC,regE;
int p8,pA,rC,pE; // addresses of currently selected prg-rom banks.
int prevBSSrc[8]; // used to ensure that it doesn't bankswitch when the correct bank is already selected

void mmc_setupbanks(const char* gameImage)
{
    memcpy(ram.bankC,&gameImage[0],0x4000);
    /*
    if (p8!=reg8) memcpy(ram.bank8,&gameImage[reg8*0x2000],0x2000);
    if (pA!=regA) memcpy(ram.bankA,&gameImage[regA*0x2000],0x2000);
    if (rC!=regC) memcpy(ram.bankC,&gameImage[regC*0x2000],0x2000);
    if (pE!=regE) memcpy(ram.bankE,&gameImage[regE*0x2000],0x2000);
    */
    p8=reg8;
    pA=regA;
    rC=regC;
    pE=regE;
}

int mmc_bankswitch(int dest,int src,int count)
{
    int i,last,c;
    for (i=last=c=0; i<count; i++)
    {
        if (prevBSSrc[dest+i]!=src+i)
        {
            prevBSSrc[dest+i]=src+i;
            ++c;
        }
        else
        {
            if (c>0)
            {
copylastbanks:
                if (!PpuCopyBanks(dest+last,src+last,c)) return 0;
                c=0;
            }
            last=i+1;
        }
    }
    if (c>0) goto copylastbanks;
    return 1;
}

void MmcReset()
{
    p8=pA=rC=pE=-1;
    memset(prevBSSrc,-1,sizeof(prevBSSrc));
    memset(&ram,0,sizeof(ram));
    // TODO: FILL WITH 0XCC
}

int mmc_setup(uint8_t mapperType,const void* gameImage)
{
    switch (mapperType)
    {
    case 0: // Direct Access
        mmc_bankswitch(0,0,8);
        reg8=0;
        regA=1;
        regC=2;
        regE=3;
        mmc_setupbanks((const char*)gameImage);
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
        assert((valueOf(pc)>>15)==1 && pc.isNotMax());
    #endif
	return *(uint16_t*)&(ram.ram_data[valueOf(pc)]);
}

byte_t loadZp(const addr8_t zp)
{
	return zeropage[valueOf(zp)];
}

word_t loadZp16bit(const addr8_t zp)
{
    assert(zp.isNotMax());
	return *(uint16_t*)&(zeropage[valueOf(zp)]);
}


uint8_t joystick1_pos;
#define VK_LEFT	37
#define VK_UP	38
#define VK_RIGHT	39
#define VK_DOWN	40
extern "C" int __stdcall GetAsyncKeyState(int);
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
        switch (valueOf(Address))
        {
        case 0x4016:
            switch (joystick1_pos++)
            {
        case 2: // SELECT
                if (GetAsyncKeyState(32)) return 0x41;else return 0;
                break;
        case 3: // START
                if (GetAsyncKeyState(13)) return 0x41;else return 0;
                break;

            case 4:
                if (GetAsyncKeyState(VK_UP)) return 0x41;else return 0;
                break;
            case 5:
                if (GetAsyncKeyState(VK_DOWN)) return 0x41;else return 0;
                break;
            case 6:
                if (GetAsyncKeyState(VK_LEFT)) return 0x41;else return 0;
                break;
            case 7:
                if (GetAsyncKeyState(VK_RIGHT)) return 0x41;else return 0;
                break;

            }
            break;
        }
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
    if (valueOf(Address)==0x2007 && (Value==0 || Value==0x24) ) goto dontlog;

    printf("* Write %X to $%X *\n",Value,valueOf(Address));
    dontlog:
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
