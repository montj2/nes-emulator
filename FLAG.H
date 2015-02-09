template <typename T,typename ET,int bits>
class FLAG {
    public:
        typedef T DataTp,ValueTp;

        enum :DataTp
        {
            MASK=BIT_MASK(DataTp,bits)
        };

    protected:
        DataTp value;

    private:
        typedef ET CheckTp,EnumTp;

    public:
        FLAG() {clearAll();}
        FLAG(const FLAG& rhs) = delete; // copy ctor rarely used
        FLAG& operator =(const FLAG& rhs) = delete; // rarely used

        // bit converter
        BIT<T,bits>& asBIT()
        {
            return (BIT<T,bits>&)*this;
        }

        // setter
        FLAG& operator =(const DataTp data)
        {
            assert(0==(data&(~MASK)));
            this->value=data;
            return *this;
        }

        FLAG& operator =(const BIT<T,bits>& rhs) // safe assignment
        {
            this->value=valueOf(rhs);
            return *this;
        }

        // getter (no default getter provided)
        GENERATE_ALTERNATIVE_GETTER(FLAG)

        // manipulation
        bool any() const
        {
            return 0!=this->value;
        }

        bool test(const EnumTp e) const
        {
            return (this->value&e)==e;
        }

        void flip(const EnumTp e)
        {
            this->value^=e;
        }

        void set(const EnumTp e)
        {
            this->value|=e;
        }

        void clear(const EnumTp e)
        {
            this->value&=(~e);
        }

        void change(const EnumTp e,const bool enabled) {
            if (enabled)
                this->value|=e;
            else
                this->value&=(~e);
        }

        ValueTp change(const EnumTp e,const DataTp newValue) = delete;

        void setAll()
        {
            this->value=MASK;
        }

        void clearAll()
        {
            this->value=0;
        }

        ValueTp query(const EnumTp e) const
        {
            const DataTp d=(DataTp)e;
            return (this->value&d)/LOW_BIT(d);
        }

        ValueTp update(const EnumTp _pos,const DataTp newValue)
        {
            const DataTp pos=(DataTp)_pos;
            assert(0==(newValue&(~RTRIM(pos))));
            return this->value=(this->value&(~pos))|((newValue*LOW_BIT(pos))&pos);
        }

        ValueTp inc(const EnumTp _pos)
        {
            const DataTp pos=(DataTp)_pos;
            vassert(0!=(pos&(pos-1)));
            this->value=(this->value&(~pos))|(((this->value&pos)+LOW_BIT(pos))&pos);
            return query(_pos);
        }

        ValueTp copy(const EnumTp pos,const DataTp src,const int shift,const int length=1)
        {
            vassert(0==(BIT_MASK(DataTp,length)&(~RTRIM((DataTp)pos))));
            return update(pos,(src>>shift)&BIT_MASK(DataTp,length));
        }

        // overloaded operators
        FLAG& operator |=(const EnumTp rhs)
        {
            set(rhs);
            return *this;
        }

        FLAG& operator ^=(const EnumTp rhs)
        {
            flip(rhs);
            return *this;
        }

        FLAG& operator -=(const EnumTp rhs)
        {
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

        bool operator [](const EnumTp e) const
        {
            vassert(0==(((DataTp)e)&(((DataTp)e)-1)));
            return test(e);
        }

        ValueTp operator ()(const EnumTp e) const
        {
            vassert(0!=(((DataTp)e)&(((DataTp)e)-1)));
            return query(e);
        }
};
