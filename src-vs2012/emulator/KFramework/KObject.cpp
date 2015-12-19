#include "stdafx.h"
#include "KFramework.h"
#include "KObject.h"

ULONG KObject::g_cntObjects=0;
ULONG KObject::g_cntObjectCreation=0;

KObject::KObject(void):m_timeCreation(clock())
{
	InterlockedIncrement(&g_cntObjects);
	InterlockedIncrement(&g_cntObjectCreation);
}


KObject::~KObject(void)
{
	InterlockedDecrement(&g_cntObjects);
}


void KObject::obj_atExit(void)
{
	printf("<Object manager> %ld objects were created during run time.\n",g_cntObjectCreation);
	printf("<Object manager> %ld left\n",g_cntObjects);
	assert(g_cntObjects==0);
}