// T must be one of unsigned basic data types
template <typename T,int bits>
class BIT {
    public:
        typedef T DataTp,ValueTp;

        enum :DataTp
        {
            MASK=BIT_MASK(DataTp,bits),
        };

        enum
        {
            __TYPE_CHECK=STATIC_ASSERT(bits>0 && bits<=TYPE_BITS(DataTp) && std::is_unsigned<DataTp>::value )
        };

    #ifdef SAFE_CASTING
        protected:
            DataTp value;
    #else
        DataTp value;
    #endif

    private:
        typedef T CheckTp;

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
        BIT(const BIT&) = default; // copy ctor (w/o checking)
        BIT& operator =(const BIT&) = default; // copy assignment ctor (w/o checking)

        // getters
        GENERATE_GETTER(BIT) // type conversion
        GENERATE_ALTERNATIVE_GETTER(BIT)

        // checked wrapper
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
        GENERATE_SETTER(BIT)
        {
            GENERATE_SETVALUE(rhs);
            assert(0==(value&(~MASK)));
            return *this;
        }

        void bitCopyAndWrap(const DataTp data)
        {
            value=data&MASK;
        }

        // FLAG converter
        template <typename ET>
        FLAG<T,ET,bits>& asFLAG()
        {
            return (FLAG<T,ET,bits>&)*this;
        }

        template <typename ET>
        BIT(const FLAG<T,ET,bits>& f)
        {
            value=ValueOf(f);
        }

        // checker
        void checkAssert()   const {assert(_check());}

        // const functions (don't modify the value)
        bool MSB()           const {return 1&(value>>(bits-1));}

        bool LSB()           const {return 1&value;}

        bool bitAt(const int n) const {vassert(n>=0 && n<bits);return value&(((DataTp)1)<<n);}

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

        friend ValueTp inc(BIT& var) {++var.value;return var.value&=MASK;}

        ValueTp sub(const ValueTp delta) {return value=(value-delta)&MASK;}

        friend ValueTp dec(BIT& var) {--var.value;return var.value&=MASK;}

        // bitXXX functions
        void bitRTrim()             {value=RTRIM(value);}

        void bitDropLowbit()        {value&=(value-1);}

        void bitCompl()
        { // = x86 NOT, 255-x
            value^=MASK; // faster
            /*
                return value=MASK&(~value); // slower but safer
                return value=MASK-value;
            */
        }

        void bitNOT() {bitCompl();}

        void bitSetMax() {value=MASK;}

        void bitNEG() {value=MASK&(-value);}

        void bitAndEqual(const DataTp rhs) {value&=rhs;}

        void bitOrEqual(const DataTp rhs)  {value|=rhs;}

        void bitXorEqual(const DataTp rhs) {value^=rhs;}

        // ShiftLeft
        void bitShl(const int n) {vassert(n>=0 && n<=bits);value=(value<<n)&MASK;}

        // ShiftRight
        void bitShr(const int n) {vassert(n>=0 && n<=bits);value>>=n;}

        // RotateLeftWithCarry
        void bitRcl(const bool carry) {value=((value<<1)&MASK)|(carry?1:0);}

        // RotateLeft
        void bitRol() {bitRcl(MSB());}

        // RotateRightWithCarry
        void bitRcr(const bool carry) {value=(value>>1)|(carry?(1<<(bits-1)):0);}

        // RotateRight
        void bitRor() {bitRcr(LSB());}
};

template <typename T,int bits,T constValue>
class CONST_BIT: public BIT<T,bits> {
public:
    enum {
        __VALUE_CHECK=STATIC_ASSERT(0==(constValue&(~(BIT<T,bits>::MASK))))
    };
    CONST_BIT() {BIT<T,bits>::value=constValue;}
};
