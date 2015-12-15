namespace emu
{
	// global functions
	void init();
	void deinit();

	bool load(const _TCHAR* file);
	void setup();

	bool nextFrame();
	void run();
}