#pragma once
// Object manger
class KFRAMEWORK_API KObject
{
public:
	// update statistics
	KObject(void);
	~KObject(void);

	// these methods must be inherited and implemented by instances
	virtual const TCHAR* toString(void) const =0; // retrieve the name of the object

	// these methods can be overrided by instances
	virtual const bool equals(const KObject& rhs) const {return this==&rhs;}
	
	// automatically generated
	clock_t obj_getCreationTimestamp(void) const {return this->m_timeCreation;} // retrieve the creation time of the object

	static void obj_atExit(void);

private:
	static ULONG g_cntObjects;
	static ULONG g_cntObjectCreation;

	const clock_t m_timeCreation;
};

template <class T>
void SAFE_DELETE(T* &p)
{
	assert(p!=NULL);
	delete p;
	p=NULL;
}


#ifndef SAFE_RELEASE
template <class T>
void SAFE_RELEASE_T(void* &p)
{
	assert(p!=NULL);
	if (p!=NULL)
	{
		ULONG ret=((T*)p)->Release();
		assert(ret==0);
		if (ret==0) p=NULL;
	}
}
template <class T>
void SAFE_RELEASE(T* &p)
{
	assert(p!=NULL);
	if (p!=NULL)
	{
		ULONG ret=p->Release();
		assert(ret==0);
		if (ret==0) p=NULL;
	}
}
#endif


template <typename T>
void SAFE_FREE(T* &p)
{
	assert(p!=NULL);
	if (p!=NULL)
	{
		free(p);
		p=NULL;
	}
}

template <typename T>
void SAFE_FREE_ALIGNED(T* &p)
{
	assert(p!=NULL);
	if (p!=NULL)
	{
		_aligned_free(p);
		p=NULL;
	}
}