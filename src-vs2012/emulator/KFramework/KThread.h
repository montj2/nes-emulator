#pragma once
#include "KObject.h"
#include "KHandle.h"
#include "KMutex.h"
class KFRAMEWORK_API KThread :
	public KObject, public KHandle
{
public:
	virtual const TCHAR* toString(void) const {return _TEXT("Win32 Thread Object");}
	
	KThread(const LPTHREAD_START_ROUTINE routine,const bool suspended=false);
	virtual ~KThread(void);

	DWORD exit(const DWORD exitcode=0);
	bool terminate(const DWORD exitcode=-1);
	bool resume(void);
	bool suspend(void);

	DWORD getThreadId(void) const{return m_dwThreadId;}

private:
	DWORD m_dwThreadId;
	bool m_fTerminated;
	KMutex m_mtx_termination;

	KThread(const KThread&);
};


class KFRAMEWORK_API IRunnable :
	public KThread
{
public:
	static DWORD _threadentry(IRunnable* Me)
	{
		return Me->_run();
	}
	IRunnable(const bool suspended=false):KThread((LPTHREAD_START_ROUTINE)_threadentry,suspended) {}
private:
	virtual DWORD _run() =0;
};