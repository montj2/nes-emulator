// KFramework.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "KFramework.h"


// This is an example of an exported variable
KFRAMEWORK_API int nKFramework=0;

// This is an example of an exported function.
KFRAMEWORK_API int fnKFramework(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see KFramework.h for the class definition
CKFramework::CKFramework()
{
	return;
}
