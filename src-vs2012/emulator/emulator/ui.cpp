#include "stdafx.h"

// local header files
#include "macros.h"

#include "nes/emu.h"
#include "ui.h"

#include <Windows.h>

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
	static ULONGLONG frameStartTime;

	void init()
	{
		// default keyboard settings
		buttonMapping[0][BUTTON_A]='X';
		buttonMapping[0][BUTTON_B]='Z';
		buttonMapping[0][BUTTON_SELECT]=VK_LSHIFT;
		buttonMapping[0][BUTTON_START]=VK_RETURN;
		buttonMapping[0][BUTTON_UP]=VK_UP;
		buttonMapping[0][BUTTON_DOWN]=VK_DOWN;
		buttonMapping[0][BUTTON_LEFT]=VK_LEFT;
		buttonMapping[0][BUTTON_RIGHT]=VK_RIGHT;
		joypadPresent[0]=true;
	}

	void reset()
	{
		// reset input state
		resetInput();
	}

	void blt32(const uint32_t buffer[], const int width, const int height)
	{
		BITMAPINFO bi;
		memset(&bi,0,sizeof(bi));
		bi.bmiHeader.biWidth=width;
		bi.bmiHeader.biHeight=-height;
		bi.bmiHeader.biSize=sizeof(bi.bmiHeader);
		bi.bmiHeader.biBitCount=32;
		bi.bmiHeader.biPlanes=1;
		StretchDIBits(GetDC(0),0,0,width,height,0,0,width,height,&buffer[0],&bi,0,SRCCOPY);
	}

	void onFrameBegin()
	{
		frameStartTime=GetTickCount64();
	}

	void onFrameEnd()
	{
	}

	void doEvents()
	{
		// read keyboard state
		for (int p=0;p<2;p++)
		{
			if (hasInput(p))
			{
				for (int i=0;i<BUTTON_COUNT;i++)
				{
					const SHORT ret=GetAsyncKeyState(buttonMapping[p][i]);
					if (ret&0x8000)
					{
						buttonState[p][i]=0x41;
						switch (i)
						{
						case BUTTON_RIGHT:
							buttonState[p][BUTTON_LEFT]=0;
							break;
						case BUTTON_DOWN:
							buttonState[p][BUTTON_UP]=0;
							break;
						}
					}else if (ret&1)
					{
						buttonState[p][i]=0x40;
					}else
					{
						buttonState[p][i]=0;
					}
				}
			}
		}

		// hotkeys
		if (GetAsyncKeyState(VK_ESCAPE)!=0)
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

		static int lastState;
		if (lastState==0)
		{
			if (GetAsyncKeyState('S'))
			{
				lastState=1;
				puts("Save State");
				FILE *fp=fopen("default.sav","wb");
				emu::saveState(fp);
				fclose(fp);
			}else if (GetAsyncKeyState('L'))
			{
				lastState=2;
				puts("Load State");
				FILE *fp=fopen("default.sav","rb");
				if (fp!=nullptr)
				{
					ui::reset();
					emu::loadState(fp);
					fclose(fp);
				}else
				{
					puts("state not found");
				}
			}
		}else if (!GetAsyncKeyState('S') && !GetAsyncKeyState('L'))
		{
			lastState=0;
		}
	}

	void waitForVSync()
	{
		long long msToWait = 1000/MAX_FPS-(GetTickCount64()-frameStartTime);
		if (msToWait>0 && msToWait<1000)
		{
			Sleep((int)msToWait);
		}
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