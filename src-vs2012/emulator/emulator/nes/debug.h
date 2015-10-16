namespace debug
{
	void error(EMUERROR, EMUERRORSUBTYPE, const wchar_t *, const wchar_t *, unsigned long);
	void fatalError(EMUERROR, EMUERRORSUBTYPE, const wchar_t *, const wchar_t *, unsigned long);
}

#define ERROR(TYPE, SUBTYPE) debug::error(TYPE, SUBTYPE, _CRT_WIDE(__FILE__), _CRT_WIDE(__FUNCTION__), __LINE__)
#define FATAL_ERROR(TYPE, SUBTYPE) debug::fatalError(TYPE, SUBTYPE, _CRT_WIDE(__FILE__), _CRT_WIDE(__FUNCTION__), __LINE__)