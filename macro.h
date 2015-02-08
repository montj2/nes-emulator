#ifdef NULL
    #undef NULL
    #define NULL nullptr
#endif // NULL

#define STATIC_ASSERT(expr) sizeof(int[(bool)(expr)?1:-1])

template <typename T>
class __BITSOF_CLASS {
public:
    __BITSOF_CLASS() = delete;
    enum :int{
        NUM_BITS=sizeof(T)<<3
    };
};

template <>
class __BITSOF_CLASS<bool> {
public:
    __BITSOF_CLASS() = delete;
    enum :int{
        NUM_BITS=1
    };
};

// e.g. TYPE_BITS(int)=32 TYPE_BITS(bool)=1
#define TYPE_BITS(TP) (__BITSOF_CLASS<TP>::NUM_BITS)

// e.g. BIT_MASK(3)=7
// bits shouldn't be zero
#define BIT_MASK(TP,bits) (1|((((TP)1<<((bits)-1))-1)<<1))

// e.g. TYPE_MAX(int)=0xFFFFFFFF
#define TYPE_MAX(TP) BIT_MASK(TP,TYPE_BITS(TP))

#define LOW_BIT(x) ((x)&(-(x)))
#define RTRIM(x) ((x)/LOW_BIT(x))

#ifdef COMPILE_ONLY
    #define FASTCAST(VAR,TP) (cast<TP>(VAR))
    #define FASTCONSTCAST(VAR,TP) (cast<const TP>(VAR))
#else
    #define FASTCAST(VAR,TP) ((TP&)*(TP*)(&VAR))
    #define FASTCONSTCAST(VAR,TP) ((const TP&)*(TP*)(&VAR))
#endif // COMPILE_ONLY
template <typename destType,typename srcType>
inline destType& cast(srcType& source)
{
    static_assert(sizeof(srcType)==sizeof(destType),"Casting can only be performed between types of the same size");
    return *(destType*)(&source);
}

#define valueOf(BIT_VAR) ((BIT_VAR).value)

template <typename T>
inline T min(const T& x,const T& y) {return x<y?x:y;}

template <typename T>
inline T max(const T& x,const T& y) {return x>y?x:y;}

#define GENERATE_GETTER(CLASSNAME) operator ValueTp() const {return (ValueTp)(this->value);}
#define GENERATE_ALTERNATIVE_GETTER(CLASSNAME) friend ValueTp ValueOf(const CLASSNAME& x) {return x.value;}
#define GENERATE_SETTER(CLASSNAME) CLASSNAME& operator =(const CheckTp rhs)
// #define GENERATE_CTOR(CLASSNAME,DEFAULTVALUE) CLASSNAME(const CheckTp initialValue=(CheckTp)(DEFAULTVALUE))
#define GENERATE_SETVALUE(NEWVALUE) this->value=(DataTp)(NEWVALUE)
// #define GENERATE_COPY_CTOR(CLASSNAME) CLASSNAME(const CLASSNAME& rhs) {this->value=rhs.value;}
