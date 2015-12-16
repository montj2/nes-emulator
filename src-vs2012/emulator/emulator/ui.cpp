#include "stdafx.h"

// local header files
#include "macros.h"

#include "ui.h"

#include <Windows.h>

namespace ui
{
	static unsigned joypadPosition[2];
	static int buttonState[2][BUTTON_COUNT];
	static int buttonMapping[2][BUTTON_COUNT];

	void init()
	{
		// default keyboard settings
		buttonMapping[0][BUTTON_A]='Z';
		buttonMapping[0][BUTTON_B]='X';
		buttonMapping[0][BUTTON_SELECT]=VK_LSHIFT;
		buttonMapping[0][BUTTON_START]=VK_RETURN;
		buttonMapping[0][BUTTON_UP]=VK_UP;
		buttonMapping[0][BUTTON_DOWN]=VK_DOWN;
		buttonMapping[0][BUTTON_LEFT]=VK_LEFT;
		buttonMapping[0][BUTTON_RIGHT]=VK_RIGHT;

		// reset input state
		resetInput(0);
		resetInput(1);
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
		// read keyboard state
		for (int p=0;p<2;p++)
			for (int i=0;i<BUTTON_COUNT;i++)
				buttonState[p][i]=GetAsyncKeyState(buttonMapping[p][i]);
	}

	void onFrameEnd()
	{
	}

	void resetInput(const int player)
	{
		vassert(player==0 || player==1);
		joypadPosition[player]=0;
	}

	bool hasInput(const int player)
	{
		vassert(player==0 || player==1);
		return player==0;
	}

	bool readInput(const int player)
	{
		vassert(player==0 || player==1);
		assert(hasInput(player));
		return readInput(player, joypadPosition[player]++);
	}

	bool readInput(const int player, const int button)
	{
		vassert(player==0 || player==1);
		assert(hasInput(player));
		assert(button>=0 && button<BUTTON_COUNT);
		return buttonState[player%2][button%BUTTON_COUNT]!=0;
	}
}