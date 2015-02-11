#include "macro.h"
#include "datatype.h"
#include <windows.h>

void ui_blit32(const rgb32_t buffer[],const int width,const int height) {
    BITMAPINFO bi;
    memset(&bi,0,sizeof(bi));
    bi.bmiHeader.biWidth=width;
    bi.bmiHeader.biHeight=-height;
    bi.bmiHeader.biSize=sizeof(bi.bmiHeader);
    bi.bmiHeader.biBitCount=32;
    bi.bmiHeader.biPlanes=1;
    StretchDIBits(GetDC(0),0,0,width,height,0,0,width,height,&buffer[0],&bi,0,SRCCOPY);
}
