#include "stdafx_kfw.h"

// KFramework headers
#include "..\KFramework\KFramework.h"
#include "..\KFramework\KObject.h"
#include "..\KFramework\KHandle.h"
#include "..\KFramework\RVDirect3D9.h"

// local headers
#include "kfw.h"

namespace dx9render
{
	static RVDirect3D9* renderer = nullptr;

	void init()
	{
		RVDirect3D9::init();
	}

	void create(const int width, const int height)
	{
		assert(renderer == NULL);
		renderer = new RVDirect3D9(width, height, BI_RGB, 32, 5, _T("Output"));
		assert(renderer);
	}

	void destroy()
	{
		assert(renderer != NULL);
		delete renderer;
	}

	void deinit()
	{
		RVDirect3D9::deinit();
	}

	bool draw32(const void* buffer)
	{
		if (!renderer->push((const unsigned char*)buffer, false))
			while (!renderer->flip());
		return true;
	}

	void setTitle(const TCHAR* title)
	{
		SetWindowText(renderer->getWindow(), title);
	}

	bool active()
	{
		return renderer->isActive();
	}

	bool paused()
	{
		return renderer->isPaused();
	}

	bool closed()
	{
		return renderer->isClosed();
	}

	bool keyUp(int k)
	{
		return renderer->isKeyUp(k);
	}

	bool keyDown(int k)
	{
		return renderer->isKeyDown(k);
	}

	bool keyPressed(int k)
	{
		return renderer->isKeyPressed(k);
	}
}