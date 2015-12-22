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
	
	// proxy functions
	void present(const uint32_t buffer[], const int width, const int height);
	void onFrameBegin();
	void onFrameEnd();

	// save state
	void saveState(FILE *fp);
	void loadState(FILE *fp);
}