#include "../stdafx.h"

// local header files
#include "../macros.h"
#include "../types/types.h"
#include "../unittest/framework.h"

#include "internals.h"

MIRRORING mirroring;
uint8_t mapperType;
uint8_t prgCount, chrCount;
flag_set<uint8_t,ROMCONTROL1> romCtrl;
flag_set<uint8_t,ROMCONTROL2> romCtrl2;
char *trainerData;
char *imageData;
char *vromData;

bool loadRom( const _TCHAR *romFile )
{
	FILE *fp;
    uint8_t nesMagic[4]={0};
    uint8_t reserved[8]={0};

	// open rom file
    fp=_tfopen(romFile,_T("rb"));
    if (fp==NULL)
    {
        _tprintf(_T("Couldn't open %s (error code %d)\n"), romFile, errno);
        return false;
    }

    // check signature
    fread(nesMagic,4,1,fp);
    if (memcmp(nesMagic,"NES",3))
    {
        puts("[X] loadRom: Invalid File Signature");
        goto onError;
    }

	puts("[-] loading...");

    fread(&prgCount,1,1,fp);
    fread(&chrCount,1,1,fp);
    fread(&romCtrl,1,1,fp);
    fread(&romCtrl2,1,1,fp);
    fread(&reserved,8,1,fp);

    printf("[ ] %u ROM Banks\n", prgCount);
    printf("[ ] %u CHR Banks\n", chrCount);
    printf("[ ] %u ROM Control Byte #1\n", romCtrl);
    printf("[ ] %u ROM Control Byte #2\n", romCtrl2);

    mapperType=romCtrl(RCTL1_MAPPERLOW);
    mapperType|=romCtrl2(RCTL2_MAPPERHIGH)<<4;
	printf("[ ] Using mapper #%u\n",mapperType);

    mirroring=romCtrl[RCTL1_VERTICALM]?MIRROR_VERTICAL:MIRROR_HORIZONTAL;
	if (romCtrl[RCTL1_FOURSCREEN]) mirroring=MIRROR_FOURSCREEN;

	printf("[ ] Mirroring type : %u\n", mirroring);
	printf("[ ] Fourscreen mode : %u\n", romCtrl[RCTL1_FOURSCREEN]);
	printf("[ ] Trainer data present : %u\n", romCtrl[RCTL1_TRAINER]);
	printf("[ ] SRAM present : %u\n", romCtrl[RCTL1_BATTERYPACK]);
    
	// read trainer data (if present)
    if (romCtrl[RCTL1_TRAINER])
    {
        trainerData = new char[512];
        assert(trainerData != NULL);
        if (1 != fread(trainerData, 512, 1, fp)) goto incomplete;
    }

	puts("[ ] reading ROM image...");
	// read rom image
	imageData = new char[prgCount*0x4000];
    assert(imageData != NULL);
    if (prgCount != fread(imageData, 0x4000, prgCount, fp))
    {
incomplete:
        puts("[X] loadRom: File is incomplete.");
onError:
        fclose(fp);
        return 0;
    }

	puts("[ ] reading VROM data...");
	// read VROM
    vromData = new char[chrCount*0x2000];
    assert(vromData != NULL);
    if (chrCount != fread(vromData, 0x2000, chrCount, fp)) goto incomplete;

	// done. close file
    fclose(fp);
	puts("[-] loaded!");
	return true;
}

void unloadRom()
{
	SAFE_DELETE(trainerData);
	SAFE_DELETE(imageData);
	SAFE_DELETE(vromData);
}