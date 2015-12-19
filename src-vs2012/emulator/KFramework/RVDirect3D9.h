#pragma once
#include "KThread.h"
#include "kEvent.h"
#include "KMutex.h"

class KFRAMEWORK_API RVDirect3D9 :
	public IRunnable, private IObjectMutex
{
public:
	virtual const TCHAR* toString(void) const {return _TEXT("Direct3D9 Video Renderer Object");}

	static bool __stdcall init();
	static bool __stdcall deinit();

	operator bool() const;

	RVDirect3D9(const UINT width,const UINT height,const DWORD format,const int bitCount,const int numBuffers,const TCHAR* title,const bool autorender=false);
	~RVDirect3D9();

	bool write(const unsigned char* buf,const int idx,const bool fwait=true);
	bool push(const unsigned char* buf,const bool fwait=true);
	bool flip();

	bool hasError() const {return m_fDeviceError;}
	bool isPaused() const {return m_fPaused;}
	bool isActive() const {return m_fActive;}
	bool isClosed() const {return m_hWnd==NULL;}

	__int64 getFrameCount() const {return m_nFrames;}

	UINT getWidth() const {return m_iWidth;}
	UINT getHeight() const {return m_iHeight;}
	DWORD getFormat() const {return m_iFormat;}

	HWND getWindow() const {return m_hWnd;}

	bool isKeyPressed(int key) {key&=0xFF;bool ret=m_bKeyPressed[key];m_bKeyPressed[key]=false;return ret;}
	bool isKeyUp(int key) {key&=0xFF;bool ret=m_bKeyUp[key];m_bKeyUp[key]=false;return ret;}
	bool isKeyDown(int key) const {return m_bKeyDown[key&0xFF];}

	// don't call the following functions outside
	friend LRESULT CALLBACK _wndentry(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	
private:
	static void* g_pD3D;
	
	/* D3D Objects*/
	void* m_pd3dDevice;
	void* m_pd3dImageSurface[5];

	HWND m_hWnd;
	void* m_d3dpp;

	/* Device State */
	bool m_fPaused;
	bool m_fInSizeMove,m_fMinimized,m_fMaximized;
	bool m_fDeviceLost,m_fDeviceError;
	bool m_fActive;
	bool m_fInsideMainloop;

	/* Queue control */
	volatile ULONG m_head, m_tail;
	KMutex m_mtx_push,m_mtx_flip;

	/* Behaviour */
	const bool m_fAutoRender;

	/* Statistics */
	__int64 m_nFrames;

	/* Misc */
	bool m_bKeyPressed[256];
	bool m_bKeyUp[256];
	bool m_bKeyDown[256];

	const UINT m_iWidth,m_iHeight;
	const DWORD m_iFormat;
	const int m_iBitCount;
	const int m_nBuffers;
	const TCHAR* m_strTitle;

	KEvent m_evtCreation;
	KEvent m_evtRestored;
	KEvent m_evtBufferFree;

	virtual DWORD _run(void);
	bool _create(void);
	void _messageloop(const bool autorender=false);
	bool _onResetDevice(void);
	void _onLostDevice(void);
	bool _onWindowSizeChanged(void);
	bool _render(void);
	bool _draw(void);
	bool _move(void);
	LRESULT _wndproc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

	RVDirect3D9(const RVDirect3D9&);
};

