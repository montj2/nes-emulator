template <typename T,typename ET,ET maxValue>
class ENUM {
    public:
        typedef T       DataTp;
        typedef ET      EnumTp,ValueTp,CheckTp;

    protected:
        DataTp value;

    public:
        // getter
        GENERATE_GETTER(ENUM)// conversion from DataTp to EnumTp needs extra care
        GENERATE_ALTERNATIVE_GETTER(ENUM)

        // setter
        GENERATE_SETTER(ENUM) {
            GENERATE_SETVALUE(rhs);
            return *this;
        }
};
