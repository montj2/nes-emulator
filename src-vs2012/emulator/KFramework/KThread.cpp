#include "stdafx.h"
#include "KFramework.h"
#include "KThread.h"

KThread::KThread(const LPTHREAD_START_ROUTINE routine,const bool suspended): \
	m_fTerminated(false),KHandle(CreateThread(NULL,0,routine,(LPVOID)this,(suspended?CREATE_SUSPENDED:0),&m_dwThreadId))
{
}


KThread::~KThread(void)
{
	terminate(1);
}


bool KThread::resume(void)
{
	return -1!=ResumeThread(getHandle());
}


bool KThread::suspend(void)
{
	return -1!=SuspendThread(getHandle());
}


bool KThread::terminate(const DWORD dwExitCode)
{
	AUTO_LOCK(m_mtx_termination);
	if (m_fTerminated) return true;
	assert(GetCurrentThreadId()!=m_dwThreadId);
	m_fTerminated=(TerminateThread(getHandle(),dwExitCode)==TRUE);
	return WaitForSingleObject(getHandle(),INFINITE)==WAIT_OBJECT_0;
}


DWORD KThread::exit(const DWORD dwExitCode)
{
	AUTO_LOCK(m_mtx_termination);
	assert(GetCurrentThreadId()==m_dwThreadId);
	m_fTerminated=true;
	return dwExitCode;
}