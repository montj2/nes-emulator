#pragma once
class KFRAMEWORK_API IObjectMutex
{
protected:
	CRITICAL_SECTION m_mtx;
public:
	IObjectMutex()
	{
		memset(&m_mtx,0,sizeof(m_mtx));
		InitializeCriticalSection(&m_mtx);
	}
	~IObjectMutex()
	{
		DeleteCriticalSection(&m_mtx);
#ifdef _DEBUG
		memset(&m_mtx,0,sizeof(m_mtx));
#endif
	}
	CRITICAL_SECTION& getMutexObject(void) {return m_mtx;}
};


class KFRAMEWORK_API KMutex :
	public IObjectMutex
{
public:
	BOOL tryEnter()
	{
		return TryEnterCriticalSection(&m_mtx);
	}
	void Enter()
	{
		EnterCriticalSection(&m_mtx);
	}
	void Leave()
	{
		LeaveCriticalSection(&m_mtx);
	}
};


class KFRAMEWORK_API KAutoMutex
{
public:
	KAutoMutex(CRITICAL_SECTION& mtx):m_mtx(mtx) // it must be initialized
	{
		EnterCriticalSection(&m_mtx);
	}
	KAutoMutex(IObjectMutex& ptrIMutex):m_mtx(ptrIMutex.getMutexObject())
	{
		EnterCriticalSection(&m_mtx);
	}
	~KAutoMutex(void)
	{
		LeaveCriticalSection(&m_mtx);
	}
private:
	KAutoMutex(void);
	CRITICAL_SECTION& m_mtx;
};


#define AUTO_LOCK(__mtxobject) KAutoMutex __lock__auto(__mtxobject);
#define LOCK_OBJECT() KAutoMutex __lock__object(this->getMutexObject());
#define LOCK_GLOBAL() static IObjectMutex __lock__global;KAutoMutex __lock__object(__lock__global);