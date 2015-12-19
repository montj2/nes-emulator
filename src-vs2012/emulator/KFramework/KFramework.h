// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the KFRAMEWORK_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// KFRAMEWORK_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef KFRAMEWORK_EXPORTS
#define KFRAMEWORK_API __declspec(dllexport)
#else
#define KFRAMEWORK_API __declspec(dllimport)
#endif

// This class is exported from the KFramework.dll
class KFRAMEWORK_API CKFramework {
public:
	CKFramework(void);
	// TODO: add your methods here.
};

extern KFRAMEWORK_API int nKFramework;

KFRAMEWORK_API int fnKFramework(void);
