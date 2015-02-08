// T should be one of the basic unsigned data types
template <typename T,int bits>
class BIT {
    public:
        typedef T DataTp,ValueTp;
        typedef DataTp CheckTp;

        enum :DataTp
        {
            MASK=BIT_MASK(DataTp,bits),
            MAX=MASK
        };
        enum
        {
            __TYPE_CHECK=STATIC_ASSERT(bits>0 && bits<=TYPE_BITS(DataTp) && std::is_unsigned<DataTp>::value )
        };

    public:
        DataTp value;

    protected:
        // boundary check
        bool _check() const
        {
            #ifdef VERBOSE
                if (bits==TYPE_BITS(DataTp)) return true;
            #endif // VERBOSE
            return (0==(value&(~MASK)));
        }

        void _wrap()
        {
            #ifdef VERBOSE
                if (bits!=TYPE_BITS(DataTp))
            #endif // VERBOSE
            value&=MASK;
        }

        static BIT _unchecked_wrapper(const DataTp data)
        {
            BIT ret;
            ret.value=data;
            return ret;
        }

    public:
        // ctors
        BIT() = default;
        ~BIT() = default;
        BIT(const BIT&) = default; // copy ctor (w/o checking)
        BIT& operator =(const BIT&) = default; // copy assignment ctor (w/o checking)

        // getters
        GENERATE_GETTER(BIT) // type conversion
        // GENERATE_ALTERNATIVE_GETTER(BIT) // overloads operator ()

        DataTp* _addr() {return &this->value;}
        DataTp& _ref() {return this->value;}

        // wrapper
        static BIT wrapper(const CheckTp rhs) {
            BIT ret;
            ret=rhs;
            return ret;
        }

        // auto boxing (restricted)
        explicit BIT(const CheckTp rhs)
        {
            GENERATE_SETVALUE(rhs);
            assert(0==(value&(~MASK)));
        }

        // setter (lhs = CheckType)
        GENERATE_SETTER(BIT) {
            GENERATE_SETVALUE(rhs);
            assert(0==(value&(~MASK)));
            return *this;
        }

        void bitCopyAndWrap(const DataTp data) {
            value=data&MASK;
        }

        // FLAG converter
        template <typename ET>
        FLAG<T,ET,bits>& asFLAG() {
            return (FLAG<T,ET,bits>&)*this;
        }

        // checker
        void checkAssert()   const {assert(_check());}

        // const functions
        bool MSB()           const {return 1&(value>>(bits-1));}

        bool LSB()           const {return 1&value;}

        bool isNegative()    const {return MSB();}

        bool isZero()        const {return !value;}

        bool isNotMax()      const {return (~value)&MASK;}

        bool isMax()         const {return value==MASK;}

        bool isOverflow()    const {
            static_assert(bits<TYPE_BITS(DataTp),"Overflow can't be detected for this type");
            return value>>bits;
        }

        ValueTp lowbit()     const {return LOW_BIT(value);}

        BIT plus(const ValueTp delta)    const {return _unchecked_wrapper((value+delta)&MASK);}

        BIT minus(const ValueTp delta)   const {return _unchecked_wrapper((value-delta)&MASK);}

        // the following functions may change the value, and they return ValueTp
        ValueTp add(const ValueTp delta) {return value=(value+delta)&MASK;}

        /* template <typename TP> friend typename TP::ValueTp inc(TP& var,const typename TP::ValueTp delta); */

        ValueTp sub(const ValueTp delta) {return value=(value-delta)&MASK;}

        /* template <typename TP> friend typename TP::ValueTp dec(TP& var,const typename TP::ValueTp delta); */

        // bitXXX functions
        RETURNTYPE bitRTrim()       {RETURN value=RTRIM(value);}

        RETURNTYPE bitDropLowbit()  {RETURN value&=(value-1);}

        RETURNTYPE bitCompl() { // = x86 NOT, 255-x
            RETURN value^=MASK; // faster
            /*
                return value=MASK&(~value); // slower, safer
                return value=MASK-value;
            */
        }

        RETURNTYPE bitNOT()                      {RETURN bitCompl();}

        RETURNTYPE bitSetMax()                   {RETURN value=MASK;}

        RETURNTYPE bitNEG()                      {RETURN value=MASK&(-value);}

        RETURNTYPE bitAndEqual(const DataTp rhs) {RETURN value&=rhs;}

        RETURNTYPE bitOrEqual(const DataTp rhs)  {RETURN value|=rhs;}

        RETURNTYPE bitXorEqual(const DataTp rhs) {RETURN value^=rhs;}

        // ShiftLeft
        RETURNTYPE bitShl(const int n) {
            #ifdef VERBOSE
                assert(n>=0 && n<=bits);
            #endif // VERBOSE
            RETURN value=(value<<n)&MASK;
        }

        // ShiftRight
        RETURNTYPE bitShr(const int n) {
            #ifdef VERBOSE
                assert(n>=0 && n<=bits);
            #endif // VERBOSE
            RETURN value>>=n;
        }

        // RotateLeftWithCarry
        RETURNTYPE bitRcl(const bool carry) {
            RETURN value=((value<<1)&MASK)|(carry?1:0);
        }

        // RotateLeft
        RETURNTYPE bitRol() {
            RETURN bitRcl(MSB());
        }

        // RotateRightWithCarry
        RETURNTYPE bitRcr(const bool carry) {
            RETURN value=(value>>1)|(carry?(1<<(bits-1)):0);
        }

        // RotateRight
        RETURNTYPE bitRor() {
            RETURN bitRcr(LSB());
        }

        bool operator [](const int n) const {
            #ifdef VERBOSE
                assert(n>=0 && n<bits);
            #endif // VERBOSE
            return value&(((DataTp)1)<<n);
        }
};

template <typename T,int bits,T constValue>
class CONST_BIT: public BIT<T,bits> {
public:
    enum {
        __VALUE_CHECK=STATIC_ASSERT(0==(constValue&(~(BIT<T,bits>::MASK))))
    };
    CONST_BIT() {BIT<T,bits>::value=constValue;}
};

template <class T,int bits>
inline T inc(BIT<T,bits>& var) {
    ++var.value;
    return var.value&=BIT<T,bits>::MASK;
}

template <class T,int bits>
inline T dec(BIT<T,bits>& var) {
    --var.value;
    return var.value&=BIT<T,bits>::MASK;
}
