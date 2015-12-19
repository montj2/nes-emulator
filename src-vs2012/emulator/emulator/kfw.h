namespace dx9render
{
	void init();
	void deinit();

	void create(const int width, const int height);
	void destroy();
	
	bool draw32(const void* buffer);

	void setTitle(const TCHAR* title);

	bool active();
	bool paused();
	bool closed();

	bool keyPressed(int key);
	bool keyUp(int key);
	bool keyDown(int key);
}