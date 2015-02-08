template <typename T,typename ET,int bits>
class FLAG {
    public:
        typedef T                   DataTp,ValueTp;
        typedef ET                  CheckTp,EnumTp;
        // typedef FLAG<T,ET,bits>     SelfTp;

        enum {
            MASK=(DataTp)BIT_MASK(DataTp,bits)
        };

        FLAG(const FLAG& rhs) = delete; // rarely used
        FLAG& operator =(const FLAG& rhs) = delete; // rarely used

    private:
        ValueTp set(const EnumTp e,const DataTp newValue); // FORBIDDEN

    protected:
        DataTp value;

    public:
        FLAG() = default;

        // bit converter
        BIT<T,bits>* operator ->() {
            return (BIT<T,bits>*)this;
        }

        BIT<T,bits>& toBIT() {
            return (BIT<T,bits>&)*this;
        }

        // setter
        void bitCopy(const DataTp data) {
            assert(0==(data&(~MASK)));
            value=data;
        }

        void bitCopy(const BIT<T,bits>& rhs) { // w/o checking
            this->value=rhs.value;
        }

        // getter (no default getter provided)
        GENERATE_ALTERNATIVE_GETTER(FLAG)

        // manipulate
        bool any() const                      {return 0!=this->value;}

        bool test(const EnumTp e) const       {return (this->value&e)==e;}

        RETURNTYPE set(const EnumTp e)        {RETURN this->value|=e;}

        RETURNTYPE clear(const EnumTp e)      {RETURN this->value&=(~e);}

        RETURNTYPE set(const EnumTp e,const bool enabled) {
            if (enabled)
                RETURN this->value|=e;
            else
                RETURN this->value&=(~e);
        }

        RETURNTYPE set()                      {RETURN this->value=MASK;}

        RETURNTYPE clear()                    {RETURN this->value=0;}
/*
        RETURNTYPE setAll()                   {RETURN set();}

        RETURNTYPE clearAll()                 {RETURN clear();}
*/
        RETURNTYPE flip(const EnumTp e)       {RETURN this->value^=e;}

        ValueTp query(const EnumTp e) const {
            const DataTp d=(DataTp)e;
            return (this->value&d)/LOW_BIT(d);
        }

        ValueTp update(const EnumTp _pos,const DataTp newValue) {
            const DataTp pos=(DataTp)_pos;
            assert(0==(newValue&(~RTRIM(pos))));
            return this->value=(this->value&(~pos))|((newValue*LOW_BIT(pos))&pos);
        }

        ValueTp inc(const EnumTp _pos) {
            const DataTp pos=(DataTp)_pos;
            #ifdef VERBOSE
                assert(0!=(pos&(pos-1)));
            #endif
            this->value=(this->value&(~pos))|(((this->value&pos)+LOW_BIT(pos))&pos);
            return query(_pos);
        }

        ValueTp copy(const EnumTp pos,const DataTp src,const int shift,const int length=1) {
            #ifdef VERBOSE
                assert(0==(BIT_MASK(DataTp,length)&(~RTRIM((DataTp)pos))));
            #endif
            return update(pos,(src>>shift)&BIT_MASK(DataTp,length));
        }

        // overloaded operators
        FLAG& operator ^=(const EnumTp e) {
            flip(e);
            return *this;
        }

        FLAG& operator |=(const EnumTp rhs) {
            set(rhs);
            return *this;
        }

        FLAG& operator +=(const EnumTp rhs) {
            set(rhs);
            return *this;
        }

        FLAG& operator -=(const EnumTp rhs) {
            clear(rhs);
            return *this;
        }

        // token functions
        /*
        BOOLTOKEN<SelfTp> operator [](const EnumTp e) {
            assert(0==(((DataTp)e)&(((DataTp)e)-1)));
            return BOOLTOKEN<SelfTp>(*this,e);
        }

        BITTOKEN<SelfTp> operator ()(const EnumTp e) {
            return BITTOKEN<SelfTp>(*this,e);
        }
        */

        bool operator [](const EnumTp e) const {
            #ifdef VERBOSE
                assert(0==(((DataTp)e)&(((DataTp)e)-1)));
            #endif
            return test(e);
        }

        ValueTp operator ()(const EnumTp e) const {
            #ifdef VERBOSE
                assert(0!=(((DataTp)e)&(((DataTp)e)-1)));
            #endif
            return query(e);
        }
};
