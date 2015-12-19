#pragma once
#include "KObject.h"
#include "khandle.h"
class KFRAMEWORK_API KEvent :
	public KObject, public KHandle
{
public:
	virtual const TCHAR* toString(void) const {return _TEXT("Win32 Event Object");}

	KEvent(const BOOL bManualReset=FALSE,const BOOL bInitialState=FALSE,const LPCTSTR lpName=NULL):KHandle(CreateEvent(NULL,bManualReset,bInitialState,lpName)){}
	KEvent(const KEvent& rhs):KHandle(rhs.getHandle()) {}

	void set(void) {SetEvent(getHandle());}
	void reset(void) {ResetEvent(getHandle());}
	void pulse(void) {PulseEvent(getHandle());}

	~KEvent(void) {reset();}
private:
};

