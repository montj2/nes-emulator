#include "stdafx.h"

// local header files
#include "macros.h"

#include "ui.h"

#include <Windows.h>

namespace ui
{
	void blt32(const uint32_t buffer[],const int width,const int height)
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
}