// rom config
enum class MIRRORING {
	MIN=0,
    HORIZONTAL=0,
    VERTICAL,
    FOURSCREEN,
    SINGLESCREEN,
	MAX=SINGLESCREEN
};

enum class ROMCONTROL1 {
    VERTICALM=0x1,
    BATTERYPACK=0x2,
    TRAINER=0x4,
    FOURSCREEN=0x8,
    MAPPERLOW=0xF0
};

enum class ROMCONTROL2 {
    RESERVED=0xF,
    MAPPERHIGH=0xF0
};

// global functions
namespace rom
{
	bool load( const _TCHAR *romFile );
	void unload();

	int mapperType();
	MIRRORING mirrorMode();
	void setMirrorMode(MIRRORING newMode);

	const char* getImage();
	int countPRG();
	size_t sizeOfImage();

	const char* getVROM();
	int countCHR();
	size_t sizeOfVROM();
}