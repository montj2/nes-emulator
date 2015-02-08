#include "macro.h"
#include "datatype.h"
#include "nes.h"
#include "mmc.h"
#include "ppu.h"
#include "optable.h"
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

static bool
loadRom(const char* filename)
{
    FILE *fp;
    uint8_t nesMagic[4]={0};
    uint8_t reserved[8]={0};

    fp=fopen(filename,"rb");
    if (fp==NULL)
    {
        printf("Couldn't open %s (error code %d)\n",filename,errno);
        return false;
    }

    // check signature
    fread(nesMagic,4,1,fp);
    if (memcmp(nesMagic,"NES",3))
    {
        puts("Invalid nes header");
        goto onerror;
    }

    fread(&prgCount,1,1,fp);
    fread(&chrCount,1,1,fp);
    fread(&RomCtrl,1,1,fp);
    fread(&RomCtrl2,1,1,fp);
    fread(&reserved,8,1,fp);

    printf("[ %u ] ROM Banks\n",prgCount);
    printf("[ %u ] CHR Banks\n",chrCount);
    printf("[ %u ] ROM Control Byte #1\n",ValueOf(RomCtrl));
    printf("[ %u ] ROM Control Byte #2\n",ValueOf(RomCtrl2));

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

static void
init()
{
    #ifndef NDEBUG
        // Run Unit Tests
        BITTests();

        testCPU();
        testMMC();
        testPPU();
    #endif

    // Self-test
    OptSelfTest();

    // Module Initialization
    OptInit();

    mmc_reset();
    ppu_reset();
    ppu_loadPal();
}

int main()
{

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
