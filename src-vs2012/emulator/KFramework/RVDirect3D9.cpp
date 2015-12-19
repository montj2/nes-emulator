#include "stdafx.h"
#include "KFramework.h"
#include "RVDirect3D9.h"

// Constants
static const TCHAR g_szClassName[]=_TEXT("VRWindowClass");

// Global variables
void* RVDirect3D9::g_pD3D=NULL;

RVDirect3D9::RVDirect3D9(const UINT width, const UINT height, 
						 const DWORD format, const int bitCount, 
						 const int numBuffers, 
						 const TCHAR* title,
						 const bool autorender):
	m_pd3dDevice(NULL), m_hWnd(NULL), m_iWidth(width), m_iHeight(height), m_nBuffers(numBuffers),
	m_iFormat(format), m_iBitCount(bitCount), m_evtActive(FALSE, TRUE),
	m_strTitle(title),m_fAutoRender(autorender), \
	IRunnable(true)
{
	assert(width>0 && height>0 && numBuffers>=1 && numBuffers<=5 && title!=NULL);
	// INITIALIZE
	m_fPaused=false;
	m_fInSizeMove=m_fMinimized=m_fMaximized=false;
	m_fDeviceLost=m_fDeviceError=false;
	m_fActive=true;
	m_fInsideMainloop=false;
	m_head=m_tail=0;
	memset(m_pd3dImageSurface,0,sizeof(void*)*5);
	memset(m_bKeyDown,0,sizeof(m_bKeyDown));
	memset(m_bKeyUp,0,sizeof(m_bKeyUp));
	memset(m_bKeyPressed,0,sizeof(m_bKeyPressed));
	// START WORKER THREAD
	resume();
	// WAIT UNTIL DEVICE IS CREATED
	m_evtCreation.wait();
	if (*this)
	{
	}
}


bool RVDirect3D9::_create(void)
{
	// CREATE WINDOW
	RECT rc;
	SetRect(&rc,0,0,m_iWidth,m_iHeight);
	AdjustWindowRect(&rc,WS_OVERLAPPEDWINDOW,FALSE);
	m_hWnd=CreateWindow(g_szClassName,m_strTitle,WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,(rc.right-rc.left),(rc.bottom-rc.top),NULL,0,GetModuleHandle(NULL),0);
	assert(m_hWnd!=NULL);if (m_hWnd==NULL) return false;
	// FILL IN D3DPRESENT_PARAMETERS
	m_d3dpp=malloc(sizeof(D3DPRESENT_PARAMETERS));
	memset(m_d3dpp,0,sizeof(D3DPRESENT_PARAMETERS));
	D3DPRESENT_PARAMETERS& pp=*((D3DPRESENT_PARAMETERS*)m_d3dpp);
	pp.Windowed=TRUE;
	pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
	pp.BackBufferFormat=D3DFMT_UNKNOWN;
	pp.BackBufferCount=1;
	pp.MultiSampleType=D3DMULTISAMPLE_NONE; // Disable AA
	pp.hDeviceWindow=m_hWnd;
	pp.EnableAutoDepthStencil=FALSE;
	pp.PresentationInterval=D3DPRESENT_INTERVAL_ONE;
	pp.Flags=D3DPRESENTFLAG_VIDEO;
	// CREATE DEVICE
	HRESULT hr;
	hr=((IDirect3D9*)RVDirect3D9::g_pD3D)->CreateDevice(0,D3DDEVTYPE_HAL,m_hWnd,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_MULTITHREADED,&pp,((IDirect3DDevice9**)&m_pd3dDevice));
	assert(m_pd3dDevice!=NULL && SUCCEEDED(hr));
	if (FAILED(hr) || m_pd3dDevice==NULL) return false; // faild to create d3d9 device
	// CREATE OBJECTS
	if (!_onResetDevice()) return false;
	// SHOW WINDOW
	ShowWindow(m_hWnd,SW_SHOW);
	return true;
}


void RVDirect3D9::_messageloop(const bool autorender)
{
	assert(m_hWnd!=NULL);
	MSG msg;
	m_fInsideMainloop=true;
	if (!autorender)
	{
		while (GetMessage(&msg,NULL,0,0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}else
	{
		msg.message=WM_NULL;PeekMessage(&msg,NULL,0,0,PM_NOREMOVE);
		while (msg.message!=WM_QUIT)
		{
			if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}else _render();
		}
	}
	m_fInsideMainloop=false;
}


DWORD RVDirect3D9::_run()
{
	bool ret;
	ret=_create();
	m_evtCreation.set();
	if (!ret) // CREATION FAILED
		return exit(-1);
	else
	{
		// ASSOCIATE WINDOW WITH CLASS
		SetWindowLong(m_hWnd,GWL_USERDATA,(LONG)this);
		// PROCESS WINDOWS MESSAGES
		_messageloop(m_fAutoRender);
		return exit(0);
	}
}


bool RVDirect3D9::_onWindowSizeChanged(void)
{
	assert(m_pd3dDevice!=NULL);
	LOCK_OBJECT();

	RECT rc;
	GetClientRect(m_hWnd,&rc);
	rc.right-=rc.left;
	rc.bottom-=rc.top;
	rc.left=rc.top=0;

	D3DPRESENT_PARAMETERS& pp=*((D3DPRESENT_PARAMETERS*)m_d3dpp);
	if (rc.right!=pp.BackBufferWidth || rc.bottom!=pp.BackBufferHeight)
	{
		m_fDeviceLost=true;_onLostDevice();
		pp.BackBufferWidth=rc.right;
		pp.BackBufferHeight=rc.bottom;
		if (SUCCEEDED(((IDirect3DDevice9*)m_pd3dDevice)->Reset(&pp)))
		{
			m_fDeviceLost=false;
			_onResetDevice();
			// we shouldn't repaint here because the image hasn't been restored
			return true;
		}
	}
	return false;
}


void RVDirect3D9::_onLostDevice(void) // It's called when the device is lost or the window has been resized.
{
	assert(m_pd3dDevice!=NULL && m_fDeviceLost);
	puts("onLostDevice");
	m_evtRestored.reset();

	IDirect3DDevice9& device=*(IDirect3DDevice9*)m_pd3dDevice;
	// Destroy objects in default pool
	for (int i=0;i<m_nBuffers;i++)
		if (m_pd3dImageSurface[i]!=NULL) SAFE_RELEASE_T<IDirect3DSurface9>(m_pd3dImageSurface[i]);
}


bool RVDirect3D9::_onResetDevice(void)
{
	assert(m_pd3dDevice!=NULL && !m_fDeviceLost);
	if (m_fDeviceError || m_fDeviceLost) return false;

	puts("onResetDevice");
	IDirect3DDevice9& device=*(IDirect3DDevice9*)m_pd3dDevice;
	HRESULT hr;
	// Recreate objects in default pool
	D3DFORMAT fmt;
	switch (m_iFormat)
	{
	case BI_RGB:
		fmt=D3DFMT_X8R8G8B8;
		break;
	default:
		fmt=(D3DFORMAT)m_iFormat;
		break;
	}
	for (int i=0;i<m_nBuffers;i++)
	{
		IDirect3DSurface9* tmp;
		hr=device.CreateOffscreenPlainSurface(m_iWidth,m_iHeight,fmt,D3DPOOL_DEFAULT,&tmp,NULL);
		if (!SUCCEEDED(hr)) break;
		m_pd3dImageSurface[i]=tmp;
	}
	if (SUCCEEDED(hr))
	{
		m_evtRestored.set();
		return true;
	}else
	{
		m_fDeviceError=true;
		return false;
	}
}


bool RVDirect3D9::_move(void)
{
	assert(m_pd3dDevice!=NULL);
	if (m_fDeviceError || m_fDeviceLost) return false;
	IDirect3DDevice9& device=*(IDirect3DDevice9*)m_pd3dDevice;
	/* Move objects */
	return true;
}


/*
	push() will fail if
	1. the queue is full
	2. the device is lost (by default we will wait until the device is ready again)
	3. internal error
*/
bool RVDirect3D9::push(const unsigned char* buf,const bool fwait)
{
	volatile ULONG m_cur;
	AUTO_LOCK(m_mtx_push);
	m_cur=(m_tail+1)%m_nBuffers;
	if (m_cur==m_head) // full
	{
		if (fwait) 
			m_evtBufferFree.wait();
		else 
			return false;
	}
	assert(m_cur!=m_head);
	if (write(buf,m_cur))
	{
		m_tail=m_cur;
		return true;
	}else
		return false;
}


bool RVDirect3D9::flip(void)
{
	volatile ULONG m_cur;
	AUTO_LOCK(m_mtx_flip);
	if (empty()) return false;

	// next buffer
	m_cur=(m_head+1)%m_nBuffers;
	assert(m_cur!=m_head);
	if (_render())
	{
		m_head=m_cur;
		if (m_head==m_tail) m_evtBufferFree.pulse();
		return true;
	}else return false;
}


bool RVDirect3D9::write(const unsigned char* buf,const int idx,const bool fwait)
{
	assert(idx>=0 && idx<m_nBuffers && m_pd3dDevice!=NULL);
	if (m_fDeviceError) return false;
	if ((m_pd3dImageSurface[idx]==NULL || m_fDeviceLost) && fwait) m_evtRestored.wait(); // The device hasn't been reset.
	{
		LOCK_OBJECT();
		assert(m_pd3dImageSurface[idx]!=NULL);
		if (m_pd3dImageSurface[idx]==NULL) return false;
		IDirect3DDevice9& device=*(IDirect3DDevice9*)m_pd3dDevice;
		IDirect3DSurface9& surface=*(IDirect3DSurface9*)m_pd3dImageSurface[idx];
		HRESULT hr;
		D3DLOCKED_RECT rc;
		hr=surface.LockRect(&rc,NULL,0);
		if (FAILED(hr)) return false;
		unsigned char *p=(unsigned char*)rc.pBits;
		const DWORD stride=rc.Pitch;
		const DWORD w=m_iWidth,h=m_iHeight;
		switch (m_iFormat)
		{
		case BI_RGB:
			{
				const DWORD srcPitch=m_iWidth*(m_iBitCount>>3);
				if (rc.Pitch==srcPitch)
				{
					memcpy(p,buf,h*srcPitch);
				}else
				{
					for (DWORD i=0;i<h;i++)
						memcpy(p+i*stride,buf+i*srcPitch,stride);
				}
			}
			break;
		case MAKEFOURCC('Y','V','1','2'):
			if (rc.Pitch==w)
			{
				memcpy(p,buf,w*h*3/2);
			}else
			{
				for (DWORD i=0;i<h;i++) memcpy(p+i*stride,buf+i*w,w);
				for (DWORD i=0;i<(h>>1);i++) memcpy(p+h*stride+i*(stride>>1),buf+i*(w>>1)+w*h,(w>>1));
				for (DWORD i=0;i<(h>>1);i++) memcpy(p+h*stride+h*(stride>>2)+i*(stride>>1),buf+i*(w>>1)+w*h+(w*h>>2),(w>>1));
			}
			break;
		}
		hr=surface.UnlockRect();
		assert(SUCCEEDED(hr));
		return SUCCEEDED(hr);
	}
}


bool RVDirect3D9::_draw(void)
{
	LOCK_OBJECT();
	assert(m_pd3dDevice!=NULL);
	if (m_fDeviceError || m_fDeviceLost) return false;
	IDirect3DDevice9& device=*(IDirect3DDevice9*)m_pd3dDevice;
	HRESULT hr;

	device.Clear(0,NULL,D3DCLEAR_TARGET,D3DCOLOR_XRGB(0,0,0),0,0);
	device.BeginScene();
	
	/* Draw commands */
	IDirect3DSurface9 *rt;
	if (SUCCEEDED(device.GetRenderTarget(0,&rt)))
	{
		if (m_pd3dImageSurface[m_head]!=NULL) device.StretchRect((IDirect3DSurface9*)m_pd3dImageSurface[m_head],NULL,rt,NULL,D3DTEXF_LINEAR);
		rt->Release();
	}

	device.EndScene();
	hr=device.Present(NULL,NULL,NULL,NULL);++m_nFrames;
	switch (hr)
	{
	case D3DERR_DEVICELOST:
		m_fDeviceLost=true;_onLostDevice();
		return false;
	case D3DERR_DRIVERINTERNALERROR: // fatal error
		m_fDeviceError=true;
		return false;
	case D3D_OK:
		return true;
	default:
		return false;
	}
}


bool RVDirect3D9::_render(void)
{
	if (m_fDeviceError) return false;
	if (m_fDeviceLost || m_fPaused || (m_fAutoRender && !m_fActive)) Sleep(50);
	{
		LOCK_OBJECT();
		IDirect3DDevice9& device=*(IDirect3DDevice9*)m_pd3dDevice;
		HRESULT hr;
		if (m_fDeviceLost && !m_fPaused)
		{
			hr=device.TestCooperativeLevel();
			if (FAILED(hr))
			{
				switch (hr)
				{
				case D3DERR_DRIVERINTERNALERROR:
					m_fDeviceError=true;
					return false;
				case D3DERR_DEVICENOTRESET:
					if (SUCCEEDED(device.Reset((D3DPRESENT_PARAMETERS*)m_d3dpp)))
					{
						m_fDeviceLost=false;
						_onResetDevice();
						break;
					}else // Reset() failed
					{
						m_fDeviceError=true;
						return false;
					}
				case D3DERR_DEVICELOST: // Wait until it can be reset
				default:
					return false;
				}
			}
		}
		/*_move();*/
		if (!m_fPaused)
			return _draw();
		else
			return false;
	}
}


RVDirect3D9::~RVDirect3D9(void)
{
	// BLOCK EXTERNAL CALLING
	EnterCriticalSection(&(getMutexObject()));
	// KILL WINDOW
	if (m_fInsideMainloop)
	{
		if (m_hWnd!=NULL) SendMessage(m_hWnd,WM_CLOSE,NULL,NULL);
		assert(m_hWnd==NULL);
	}
	// KILL WORKER THREAD
	terminate();
	// DESTROY OBJECTS
	if (m_pd3dDevice!=NULL)
	{
		if (!m_fDeviceLost)
		{
			m_fDeviceLost=true;_onLostDevice();
		}
		SAFE_RELEASE_T<IDirect3DDevice9>(m_pd3dDevice);
	}
	SAFE_FREE(m_d3dpp);
}


RVDirect3D9::operator bool() const
{
	return g_pD3D!=NULL && m_pd3dDevice!=NULL && m_pd3dImageSurface!=NULL && m_hWnd!=NULL;
}


bool RVDirect3D9::init(void)
{
	if (g_pD3D==NULL)
	{
		g_pD3D=Direct3DCreate9(D3D_SDK_VERSION);
	}
	assert(g_pD3D!=NULL);

	WNDCLASS wndClass;
	wndClass.style=CS_DBLCLKS;
	wndClass.lpfnWndProc=_wndentry;
	wndClass.cbClsExtra=wndClass.cbWndExtra=0;
	wndClass.hInstance=GetModuleHandle(NULL);
	wndClass.hIcon=LoadIcon(NULL,IDI_APPLICATION);
	wndClass.hCursor=LoadCursor(NULL,IDC_ARROW);
	wndClass.hbrBackground=(HBRUSH)GetStockObject(LTGRAY_BRUSH);
	wndClass.lpszMenuName=NULL;
	wndClass.lpszClassName=g_szClassName;

	return (g_pD3D!=NULL) && RegisterClass(&wndClass);
}


bool RVDirect3D9::deinit(void)
{
	SAFE_RELEASE_T<IDirect3D9>(g_pD3D);
	return (g_pD3D==NULL) && UnregisterClass(g_szClassName,GetModuleHandle(NULL));
}


LRESULT RVDirect3D9::_wndproc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch (uMsg)
	{
	case WM_PAINT:
		if (m_fPaused && !m_fDeviceLost) _draw(); // handle paint messages when paused
		break;
	case WM_SIZE:
		if (wParam==SIZE_MINIMIZED)
		{
			m_fPaused=true; // pause when minimized
			m_fMinimized=true;m_fMaximized=false;
		}else
		{
			RECT rc;
			GetClientRect(hWnd,&rc);
			if (rc.top==0 && rc.bottom==0)
			{
			}else switch (wParam)
			{
			case SIZE_MAXIMIZED:
				m_fMaximized=true;m_fMinimized=false;m_fPaused=false;
				_onWindowSizeChanged();
				break;
			case SIZE_RESTORED:
				if (m_fMaximized || m_fMinimized)
				{
					if (m_fMinimized) m_fPaused=false;
					m_fMaximized=false;m_fMinimized=false;
					_onWindowSizeChanged();
				}else if (!m_fInSizeMove)
					_onWindowSizeChanged();
				break;
			}
		}
		break;
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x=m_iWidth/2;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y=m_iHeight/2;
		break;
	case WM_ENTERSIZEMOVE:
		m_fPaused=true;m_fInSizeMove=true;
		break;
	case WM_EXITSIZEMOVE:
		m_fPaused=false;m_fInSizeMove=false;
		_onWindowSizeChanged();
		break;
	case WM_ENTERMENULOOP:
		m_fPaused=true;
		break;
	case WM_EXITMENULOOP:
		m_fPaused=false;
		break;
	case WM_ACTIVATEAPP:
		m_fActive=(wParam==TRUE);
		if (m_fActive) m_evtActive.set();
		break;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		m_hWnd=NULL; // The window doesn't exist any longer.
		break;
	case WM_KEYDOWN:
		m_bKeyDown[wParam&0xFF]=true;
		if (!(lParam & 0x40000000))
			m_bKeyPressed[wParam&0xFF]=true;
		break;
	case WM_KEYUP:
		m_bKeyUp[wParam&0xFF]=true;
		m_bKeyDown[wParam&0xFF]=false;
		break;
	}
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}


LRESULT CALLBACK _wndentry( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	RVDirect3D9* Me= (RVDirect3D9*)GetWindowLong(hWnd,GWL_USERDATA);
	if (Me==NULL)
	{
		return DefWindowProc(hWnd,uMsg,wParam,lParam);
	}else
	{
		return Me->_wndproc(hWnd,uMsg,wParam,lParam);
	}
}