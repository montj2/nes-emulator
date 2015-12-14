// rom config
enum MIRRORING {
	MIRROR_MIN=0,
    MIRROR_HORIZONTAL=0,
    MIRROR_VERTICAL,
    MIRROR_FOURSCREEN,
    MIRROR_SINGLESCREEN,
	MIRROR_MAX=MIRROR_SINGLESCREEN
};

enum ROMCONTROL1 {
    RCTL1_VERTICALM=0x1,
    RCTL1_BATTERYPACK=0x2,
    RCTL1_TRAINER=0x4,
    RCTL1_FOURSCREEN=0x8,
    RCTL1_MAPPERLOW=0xF0
};

enum ROMCONTROL2 {
    RCTL2_RESERVED=0xF,
    RCTL2_MAPPERHIGH=0xF0
};

// global functions
namespace rom
{
	bool load( const _TCHAR *romFile );
	void unload();

	int mapperType();
	int mirrorMode();
	void setMirrorMode(MIRRORING newMode);

	const char* getImage();
	const char* getVROM();

	size_t sizeOfImage();
	size_t sizeOfVROM();
}