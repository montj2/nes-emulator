#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <errno.h>
#include <type_traits>

#ifdef NDEBUG
    #define assert(e) ((void)0)
#else
    #ifndef RC_INVOKED
        #include <_mingw.h>
        extern "C" {
            _CRTIMP void __cdecl __MINGW_NOTHROW _assert (const char*, const char*, int) __MINGW_ATTRIB_NORETURN;
        }
    #endif
    #define assert(e) if (e) {} else {__asm volatile("int3");_assert(#e, __FILE__, __LINE__);}
#endif
