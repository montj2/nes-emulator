#include "stdafx.h"
#include "KFramework.h"
#include "KHandle.h"
ULONG KHandle::g_cntHandles=0;
ULONG KHandle::g_cntHandleCreation=0;


KHandle::KHandle(const HANDLE handle,const bool reserve): \
		m_Handle((INVALID_HANDLE_VALUE==handle)?NULL:handle),m_fReserve(reserve)
{
	if (!m_fReserve) InterlockedIncrement(&g_cntHandles);
	InterlockedIncrement(&g_cntHandleCreation);
}


KHandle::KHandle(const KHandle& rhs):m_Handle(rhs.getHandle()),m_fReserve(true)
{
}


KHandle::~KHandle(void)
{
	if (!m_fReserve)
	{
		assert(m_Handle!=NULL);
		if (CloseHandle(m_Handle))
			InterlockedDecrement(&g_cntHandles);
	}
	const_cast<HANDLE>(m_Handle)=NULL;
}

void KHandle::hdl_atExit(void)
{
	assert(g_cntHandles==0);
}


bool KHandle::wait(const DWORD milliseconds)
{
	return WaitForSingleObject(m_Handle,milliseconds)==WAIT_OBJECT_0;
}