template <typename FT>
class BOOLTOKEN {
    public:
        typedef FT                      FlagTp;
        typedef typename FT::DataTp     DataTp,CheckTp;
        typedef typename FT::ValueTp    ValueTp;
        typedef typename FT::EnumTp     EnumTp;

    private:
        FlagTp& flag;
        const EnumTp mask;

        // assignment operator is FORBIDDEN
        BOOLTOKEN& operator =(const BOOLTOKEN& rhs);

        BOOLTOKEN& operator =(const CheckTp&);

    public:
        BOOLTOKEN(FlagTp& f,const EnumTp e):flag(f),mask(e) {}

        // getter
        operator bool() const {return ((const FlagTp&)flag).test(mask);}

        // setter
        BOOLTOKEN& operator =(const bool rhs) {
            if (rhs) flag.set(mask);else flag.clear(mask);
            return *this;
        }
};

template <typename FT>
class BITTOKEN {
    public:
        typedef FT                      FlagTp;
        typedef typename FT::DataTp     DataTp,CheckTp;
        typedef typename FT::ValueTp    ValueTp;
        typedef typename FT::EnumTp     EnumTp;

    private:
        FlagTp& flag;
        const EnumTp mask;

        // assignment operator is FORBIDDEN
        BITTOKEN& operator =(const BITTOKEN& rhs);

        BITTOKEN& operator =(const bool);

    public:
        BITTOKEN(FlagTp& f,const EnumTp e):flag(f),mask(e) {}

        // getter
        operator ValueTp() const {
            return flag.query(mask);
        }

        // setter
        BITTOKEN& operator =(const CheckTp rhs) {
            flag.update(mask,rhs);
            return *this;
        }

        // shift
        /*
        Unimplemented
        BITTOKEN& operator >>=(const int bits) {
            value=(value&(~mask))|(((value&mask)>>bits)&mask);
            return *this;
        }

        BITTOKEN& operator <<=(const int bits) {
            value=(value&(~mask))|(((value&mask)<<bits)&mask);
            return *this;
        }
        */
};
