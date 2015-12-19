#include "stdafx.h"

// local header files
#include "macros.h"
#include "types/types.h"
#include "unittest/framework.h"

#include "nes/internals.h"
#include "nes/emu.h"
#include "ui.h"
#include "kfw.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <mmsystem.h>

namespace ui
{
	static bool quitRequired = false;

	// controller state
	static bool joypadPresent[2] = {false};
	static unsigned joypadPosition[2];
	static int buttonState[2][BUTTON_COUNT];
	static int buttonMapping[2][BUTTON_COUNT];

	// render state
	static const int MAX_FPS = 60;
	static const int TIMER_RESOLUTION = 2;
	static int fpsCounter=0;
	static UINT frameTimer;
	static HANDLE frameTickEvent;
	static ULONGLONG frameStartTime;
	static ULONGLONG lastSecond;

	void init()
	{
#ifdef WANT_DX9
		dx9render::init();
#endif // WANT_DX9

		// default keyboard settings
		buttonMapping[0][BUTTON_A]='X';
		buttonMapping[0][BUTTON_B]='Z';
		buttonMapping[0][BUTTON_SELECT]=VK_SHIFT;
		buttonMapping[0][BUTTON_START]=VK_RETURN;
		buttonMapping[0][BUTTON_UP]=VK_UP;
		buttonMapping[0][BUTTON_DOWN]=VK_DOWN;
		buttonMapping[0][BUTTON_LEFT]=VK_LEFT;
		buttonMapping[0][BUTTON_RIGHT]=VK_RIGHT;
		joypadPresent[0]=true;
	}

	void deinit()
	{
#ifdef WANT_DX9
		dx9render::deinit();
#endif
	}

	void reset()
	{
		// reset input state
		resetInput();
	}

	void blt32(const uint32_t buffer[], const int width, const int height)
	{
#ifdef WANT_DX9
		dx9render::draw32(buffer);
#else
		BITMAPINFO bi;
		memset(&bi,0,sizeof(bi));
		bi.bmiHeader.biWidth=width;
		bi.bmiHeader.biHeight=-height;
		bi.bmiHeader.biSize=sizeof(bi.bmiHeader);
		bi.bmiHeader.biBitCount=32;
		bi.bmiHeader.biPlanes=1;
		StretchDIBits(GetDC(0),0,0,width,height,0,0,width,height,&buffer[0],&bi,0,SRCCOPY);
#endif
	}

	void CALLBACK FrameTimerCallBack(UINT uID,UINT uMsg,DWORD dwUsers,DWORD dw1,DWORD dw2)
	{
		SetEvent(frameTickEvent);
	}

	void onGameStart()
	{
#ifdef WANT_DX9
		dx9render::create(SCREEN_WIDTH, SCREEN_HEIGHT);
#endif
#ifdef FPS_LIMIT
		timeBeginPeriod(TIMER_RESOLUTION);
		frameTickEvent=CreateEvent(NULL, FALSE, TRUE, NULL);
		frameTimer=timeSetEvent(1000/MAX_FPS, TIMER_RESOLUTION, FrameTimerCallBack, 0, TIME_PERIODIC);
		assert(frameTickEvent && frameTimer!=0);
#endif // FPS_LIMIT

	}

	void onGameEnd()
	{
#ifdef WANT_DX9
		dx9render::destroy();
#endif
#ifdef FPS_LIMIT
		timeKillEvent(frameTimer);
		CloseHandle(frameTickEvent);
		timeEndPeriod(TIMER_RESOLUTION);
#endif
	}

	void onFrameBegin()
	{
		frameStartTime=GetTickCount64();
	}

	void onFrameEnd()
	{
#ifdef WANT_DX9
		++fpsCounter;
		if (GetTickCount64()-lastSecond>=1000)
		{
			if (lastSecond!=0)
			{
				// display status in window title
				TCHAR caption[256];
				wsprintf(caption, _T("FPS: %d"), fpsCounter);
				dx9render::setTitle(caption);
			}
			fpsCounter=0;
			lastSecond=GetTickCount64();
		}
#endif // WANT_DX9
		// printf("Frame %I64d\n", emu::frameCount());
	}

	void readKeyboardState()
	{
		for (int p=0;p<2;p++)
		{
			if (hasInput(p))
			{
				for (int i=0;i<BUTTON_COUNT;i++)
				{
#ifdef WANT_DX9
					if (dx9render::keyDown(buttonMapping[p][i]))
#else
					const SHORT ret=GetAsyncKeyState(buttonMapping[p][i]);
					if (ret&0x8000)
#endif
					{
						buttonState[p][i]=0x41;
						switch (i)
						{
						case BUTTON_RIGHT:
							buttonState[p][BUTTON_LEFT]=0x40;
							break;
						case BUTTON_DOWN:
							buttonState[p][BUTTON_UP]=0x40;
							break;
						}
					}
#ifdef WANT_DX9
					else if (dx9render::keyUp(buttonMapping[p][i]))
#else
					else if (ret&1)
#endif
					{
						buttonState[p][i]=0x40;
					}else
					{
						buttonState[p][i]=0;
					}
				}
			}
		}
	}

	void doEvents()
	{
#ifdef PAUSE_WHEN_INACTIVE
#ifdef WANT_DX9
		if (!dx9render::active())
			dx9render::wait();
#endif
#endif

		// read keyboard state
		readKeyboardState();

		// hotkeys
#ifdef WANT_DX9
		if (dx9render::keyPressed(VK_ESCAPE))
#else
		if (GetAsyncKeyState(VK_ESCAPE)!=0)
#endif
		{
			if (GetAsyncKeyState(VK_CONTROL)!=0)
			{
				quitRequired=true;
			}
			else
			{
				ui::reset();
				emu::reset();
			}
		}else
		{
			quitRequired=false;
		}

#ifdef WANT_DX9
		if (dx9render::keyPressed('S'))
		{
			FILE *fp=fopen("default.sav","wb");
			emu::saveState(fp);
			fclose(fp);
			puts("State saved");
		}else if (dx9render::keyPressed('L'))
		{
			FILE *fp=fopen("default.sav","rb");
			if (fp!=nullptr)
			{
				ui::reset(); // necessary
				emu::loadState(fp);
				puts("State loaded");
				fclose(fp);
			}else
			{
				puts("Previous state not found");
			}
		}
#endif

#ifdef WANT_DX9
		// exit on window close or device error
		quitRequired|=dx9render::closed() || dx9render::error();
#endif
	}

	void limitFPS()
	{
#ifdef FPS_LIMIT
		WaitForSingleObject(frameTickEvent, 1000/MAX_FPS);
		return;
		
		/*
			long long msToWait = 1000/MAX_FPS-(GetTickCount64()-frameStartTime);
			if (msToWait>0 && msToWait<1000)
			{
				Sleep((int)msToWait);
			}
		*/
#endif
	}

	void resetInput()
	{
		joypadPosition[0]=0;
		joypadPosition[1]=0;
	}

	bool hasInput(const int player)
	{
		vassert(player==0 || player==1);
		return joypadPresent[player];
	}

	int readInput(const int player)
	{
		assert(hasInput(player));
		return readInput(player, joypadPosition[player]++);
	}

	int readInput(const int player, const int button)
	{
		assert(hasInput(player));
		assert(button>=0 && button<BUTTON_COUNT);
		if (hasInput(player))
		{
			if (button>=BUTTON_COUNT) return 1;
			return buttonState[player%2][button%BUTTON_COUNT];
		}else
			return 0;
	}

	bool forceTerminate()
	{
		return quitRequired;
	}
}