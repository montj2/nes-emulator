#pragma once
class KFRAMEWORK_API KHandle
{
public:
	KHandle(const HANDLE handle,const bool reserve=false);
	KHandle(const KHandle& rhs);
	~KHandle(void);

	HANDLE getHandle(void) const{return m_Handle;}

	bool wait(const DWORD milliseconds=INFINITE);

	operator bool() const {return (m_Handle!=NULL);}
	bool operator !(void) const{return (m_Handle==NULL);}

	static void hdl_atExit(void);

private:
	const HANDLE m_Handle;
	const bool m_fReserve;

	static ULONG g_cntHandles;
	static ULONG g_cntHandleCreation;
};

