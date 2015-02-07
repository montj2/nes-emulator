#include "macro.h"
#include "datatype.h"
#include "nes.h"
#include "mmc.h"
#include "ppu.h"
#include "cpu.h"
#include "unittest.h"

/*ENUM<uint32_t,MIRRORING,MIRROR_MAX>*/
MIRRORING                   mirroring;
uint8_t                     mapperType;
uint8_t                     prgCount,chrCount;
FLAG<uint8_t,ROMCONTROL1>   RomCtrl;
FLAG<uint8_t,ROMCONTROL2>   RomCtrl2;
void                        *trainerData;
void                        *gameImage;
void                        *VROM;

bool loadRom(const char* filename)
{
    FILE *fp;
    char nesMagic[4]={0};
    char Reserved[8]={0};

    fp=fopen(filename,"rb");
    if (fp==NULL)
    {
        printf("fopen(%s) failed\nerror code is %d",filename,errno);
        return false;
    }

    fread(nesMagic,4,1,fp);
    if (nesMagic[0]!='N' || nesMagic[1]!='E' || nesMagic[2]!='S') {
        puts("Invalid nes header");
        goto onerror;
    }

    fread(&prgCount,1,1,fp);
    fread(&chrCount,1,1,fp);
    fread(&RomCtrl,1,1,fp);
    fread(&RomCtrl2,1,1,fp);
    fread(&Reserved,8,1,fp);

    printf("[ %u ] ROM Banks\n",prgCount);
    printf("[ %u ] CHR Banks\n",chrCount);
    printf("[ %u ] ROM Control Byte #1\n",RomCtrl());
    printf("[ %u ] ROM Control Byte #2\n",RomCtrl2());
    mapperType=RomCtrl(RCTL1_MAPPERLOW);
    mapperType|=RomCtrl2(RCTL2_MAPPERHIGH)<<4;
    printf("[ %u ] Mapper\n",mapperType);
    mirroring=RomCtrl[RCTL1_VERTICALM]?MIRROR_VERTICAL:MIRROR_HORIZONTAL;
    printf("Mirroring=%u Trainer=%u FourScreen=%u SRAM=%u\n",(int)mirroring,(uint8_t)RomCtrl[RCTL1_TRAINER],(uint8_t)RomCtrl[RCTL1_FOURSCREEN],(uint8_t)RomCtrl[RCTL1_BATTERYPACK]);
    if (RomCtrl[RCTL1_FOURSCREEN]) mirroring=MIRROR_FOURSCREEN;

    if (RomCtrl[RCTL1_TRAINER])
    {
        trainerData=malloc(512);
        assert(trainerData!=NULL);
        if (1!=fread(trainerData,512,1,fp)) goto incomplete;
    }

    gameImage=(char*)malloc(prgCount*0x4000);
    assert(gameImage!=NULL);
    if (prgCount!=fread(gameImage,0x4000,prgCount,fp))
    {
incomplete:
        puts("File is incomplete.");
onerror:
        fclose(fp);
        return 0;
    }
    VROM=(char*)malloc(chrCount*0x2000);
    assert(VROM!=NULL);
    if (chrCount!=fread(VROM,0x2000,chrCount,fp)) goto incomplete;

    fclose(fp);
    printf("%s loaded\n",filename);
    return 1;
}

int ppu_copyBanks(int dest,int src,int count)
{
    if (mapperType==0)
    {
        memcpy(&vram.vram_data[dest*0x400],(char*)VROM+src*0x400,count*0x400);
        return 1;
    }
    return 0;
}

void testBIT() {
    addr14_t videoAddr;

    //videoAddr=0xFFFF;
    videoAddr=0x3FFF;
    inc(videoAddr);
    assert(valueOf(videoAddr)==0 && videoAddr.isZero());
    printf("%X\n",valueOf(videoAddr));
    dec(videoAddr);
    assert(valueOf(videoAddr)==0x3FFF && !videoAddr.isZero());
    assert(videoAddr.isMax());
    printf("%X\n",valueOf(videoAddr));
    assert(videoAddr.MSB() && videoAddr.LSB() && videoAddr.isNegative());
    videoAddr.bitShr(2);
    assert(valueOf(videoAddr)==0xFFF);
    assert(!videoAddr.MSB() && !videoAddr.isNegative());

    videoAddr.bitShl(1);
    assert(!videoAddr.LSB());
    assert(valueOf(videoAddr)==0x1FFE);
    videoAddr.bitShl(14);
    assert(videoAddr.isZero());

    videoAddr.bitXorEqual(0x3FFF);
    videoAddr.bitAndEqual(0x2001);
    assert(videoAddr.isNegative() && videoAddr.MSB() && videoAddr.LSB());

    videoAddr=0x1010;
    videoAddr.bitRol();
    assert(videoAddr.MSB() && !videoAddr.LSB());

    videoAddr.bitRol();
    assert(!videoAddr.MSB() && videoAddr.LSB());
    assert(valueOf(videoAddr)==0x0041);

    videoAddr.bitRor();
    assert(valueOf(videoAddr)==0x2020);
    videoAddr.bitRor();
    assert(valueOf(videoAddr)==0x1010);

    videoAddr.bitNOT();
    assert(valueOf(videoAddr)==0x2FEF);

    videoAddr.bitDropLowbit();
    assert(valueOf(videoAddr)==0x2FEE);
    videoAddr.bitShl(8);
    assert(valueOf(videoAddr)==0x2E00);
    assert(videoAddr.lowbit()==0x200);
    videoAddr.bitDropLowbit();
    assert(valueOf(videoAddr)==0x2C00);

    videoAddr.bitRTrim();
    assert(valueOf(videoAddr)==0b1011);
    videoAddr.bitXorEqual(0b0011);
    videoAddr.bitRTrim();
    assert(valueOf(videoAddr)==0b1);

    videoAddr.sub(0xFFFFFFFF);
    assert(valueOf(videoAddr)==2);

    videoAddr.bitShl(12);
    assert(videoAddr.MSB());
    videoAddr.bitRcl(true);
    assert(valueOf(videoAddr)==1);

    videoAddr.bitRcr(true);
    assert(videoAddr.MSB());

    puts("*** testBIT() passed! ***");

    _alutemp_t sum=0x1FF;
    assert(cast<alu_t>(sum).isOverflow());
    cast<alu_t>(sum).bitShr(1);
    assert(cast<alu_t>(sum).isOverflow()==false);
}

enum PSW {
    F_CARRY=0x1,
    F_ZERO=0x2,
    F_INTERRUPT_OFF=0x4,
    F_BCD=0x8,
    F_DECIMAL=F_BCD,
    F_BREAK=0x10,
    F_NOTUSED=0x20,
    F_OVERFLOW=0x40,
    F_NEGATIVE=0x80,
    F_SIGN=F_NEGATIVE,
    F_MULTIPLE=0x11,
    F_MULTIPLE2=0x44,
    F_MULTIPLE3=0x70,
    F_MAX=0x80
};

void testFLAG() {
    FLAG<_reg8_t,PSW,8> P;
    printf("P=%X\n",P());
    P.clear();
    assert(!P.any());

    P.set(F_ZERO);
    P-=F_BCD;
    assert(P()==F_ZERO);
    P^=F_MULTIPLE;
    assert(P()==(F_ZERO|F_MULTIPLE));
    P^=F_ZERO;
    assert(P()==F_MULTIPLE);
    P.update(F_BCD,1);
    //P.update(F_BCD,2);
    P.update(F_BCD,0);

    P.clear();
    P|=F_NEGATIVE;
    P.set(F_BREAK);
    assert(P.query(F_MULTIPLE)==0x10);
    P.bitCopy(0xFF);
    assert(P.query(F_MULTIPLE)==0x11);
    assert(P.query(F_MULTIPLE2)==0b10001);
    P.update(F_MULTIPLE2,0);
    assert(P()==0xFF-F_MULTIPLE2);

    P.clear();
    P.update(F_MULTIPLE,0x11);
    assert(P()==F_MULTIPLE);

    P.set();
    P.copy(F_MULTIPLE3,0b1101,1);
    assert(P()==0x8F);

    P.set();
    assert(P.query(F_MULTIPLE2)==0x11);
    assert(P[F_MAX] && P.test(F_MULTIPLE));
    P.update(F_MULTIPLE2,0x10);
    assert(P()==0xFF-4);
    assert(P(F_MULTIPLE2)==0x10);
    assert(!P.test(F_MULTIPLE2));

    P.clear();
    P.set(F_MULTIPLE3);
    P.inc(F_MULTIPLE3);
    assert(!P.any());
    P.inc(F_MULTIPLE3);
    assert(P(F_MULTIPLE3)==1);
    assert(P.inc(F_MULTIPLE3)==2);
    puts("*** testFLAG() passed! ***");
}

static void init() {
    //#ifndef NDEBUG
        testBIT();
        testFLAG();

        testCPU();
        testOPTABLE();
        testMMC();
        testPPU();
    //#endif

    cpu_initTable();
    cpu_testTable();

    mmc_reset();
    ppu_reset();
    ppu_loadPal();
}

int main() {
    init();

    #ifndef COMPILE_ONLY
        loadRom("nestest.nes");
        mmc_setup(mapperType,gameImage);
        cpu_reset();

        for (int i=1;i<=1000;i++) {
            printf("--- Frame %d ---\n",i);
            cpu_frame();
        }
    #endif
    return 0;
}
