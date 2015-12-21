namespace emu
{
	// global functions
	void init();
	void deinit();

	bool load(const _TCHAR* file);
	void reset();
	bool setup();

	bool nextFrame();
	void run();

	long long frameCount();

	// save state
	void saveState(FILE *fp);
	void loadState(FILE *fp);
}