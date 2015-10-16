// always use nullptr
#ifdef NULL
    #undef NULL
#endif

#define NULL nullptr

// static assertion
#define STATIC_ASSERT(expr) sizeof(int[(bool)(expr)?1:-1])

// verbose assertion
#ifdef VERBOSE
    #define vassert(e) assert(e)
#else
    #define vassert(e) ((void)0)
#endif

// Type info
template <typename T>
class __BITSOF_CLASS {
public:
    __BITSOF_CLASS();
    enum :int{
        NUM_BITS=sizeof(T)<<3
    };
};

template <>
class __BITSOF_CLASS<bool> {
public:
    __BITSOF_CLASS();
    enum :int{
        NUM_BITS=1
    };
};

// e.g. SIZE_IN_BITS(int)=32 TYPE_BITS(bool)=1
#define SIZE_IN_BITS(TP) (__BITSOF_CLASS<TP>::NUM_BITS)

// e.g. BIT_MASK(3)=7
// bits must be positive
#define BIT_MASK(TP, bits) (1|((((TP)1<<((bits)-1))-1)<<1))

// e.g. TYPE_MAX(int)=0xFFFFFFFF
#define TYPE_MAX(TP) BIT_MASK(TP,SIZE_IN_BITS(TP))

// bit manipulation
#define LOW_BIT(x) ((x)&(-(x)))
#define RTRIM(x) ((x)/LOW_BIT(x))

#define SINGLE_BIT(x) (((x)&((x)-1))==0)

#define SELECT_FIELD(x, f) (((x)&(f))/LOW_BIT(f))
#define UPDATE_FIELD(x, f, y) x=((x)&(~f))|(((y)*LOW_BIT(f))&(f))
#define INC_FIELD(x, f) x=((x)&(~(f))) | ( (((x)&(f)) + LOW_BIT(f)) & (f) );

template <typename destType, typename srcType>
inline destType& safe_cast(srcType& source)
{
    static_assert(sizeof(srcType)==sizeof(destType), "Casting between types of different size is not allowed.");
    return *(destType*)(&source);
}

#ifdef FORCE_SAFE_CASTING
	// override fast cast
    #define fast_cast(VAR,TP) (safe_cast<TP>(VAR))
    #define fast_constcast(VAR,TP) (safe_cast<const TP>(VAR))
#else
    #define fast_cast(VAR,TP) ((TP&)*(TP*)(&(VAR)))
    #define fast_constcast(VAR,TP) ((const TP&)*(const TP*)(&(VAR)))
#endif

template <typename T>
void SAFE_DELETE(T*& mem)
{
	delete mem;
	mem = nullptr;
}

template <typename T>
inline T min(const T& x,const T& y) {return x<y?x:y;}

template <typename T>
inline T max(const T& x,const T& y) {return x>y?x:y;}

#define CASE_ENUM_RETURN_STRING(ENUM) case ENUM: return L#ENUM
