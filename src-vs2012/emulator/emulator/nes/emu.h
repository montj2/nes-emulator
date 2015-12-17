namespace emu
{
	// global functions
	void init();
	void deinit();

	bool load(const _TCHAR* file);
	void reset();

	bool nextFrame();
	void run();

	// save state
	void saveState(FILE *fp);
	void loadState(FILE *fp);
}