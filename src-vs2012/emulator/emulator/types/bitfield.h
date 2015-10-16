template <typename T, int bits>
// integral type of known, fixed bit-width
class bit_field: public value_object<T> {
public:
	enum :T
	{
		MASK=BIT_MASK(T,bits),
		MAX=MASK
	};

	enum
    {
        __TYPE_CHECK=STATIC_ASSERT(bits>0 && bits<=SIZE_IN_BITS(T))
    };

protected:
	static bit_field _unchecked_wrapper(const T data)
    {
        bit_field ret;
        ret._value = data;
        return ret;
    }

public:
	// ctors
	bit_field() {};
	bit_field(const bit_field& other) {_value = other._value;} // copy ctor (w/o checking)
    bit_field& operator =(const bit_field& other) {_value = other._value;} // copy assignment ctor (w/o checking)

	template <typename ET>
	bit_field(const flag_set<T, ET, bits>& fs)
	{
		_value = valueOf(fs);
	}

	// restricted auto boxing
	explicit bit_field(const T value)
    {
        assert(0==(value&(~MASK)));
		_value = value;
    }
	
	#ifndef DISABLE_BITFIELD_AUTO_UNBOXING
		// transparent value getter (auto unboxing)
		operator T() const {return _value;}
	#endif
	
	// checked setter
	bit_field& operator = (const T value)
	{
		assert(0==(value&(~MASK)));
		_value = value;
		return *this;
	}

	// auto-wrap value setter (no warning when value out of range)
	bit_field& operator () (const T data)
	{
		_value = data&MASK;
		return *this;
	}

	// flag set converter (need to specify enum type)
	template <typename ET>
	flag_set<T, ET, bits>& asFlagSet()
	{
		return *(flag_set<T,ET,bits>*)this;
	}

	// out of range checker
	bool error() const
    {
        #ifdef VERBOSE
		if (bits==SIZE_IN_BITS(T)) return false;
        #endif // VERBOSE
        return _value&(~MASK);
    }

	void check() const
	{
		assert(!error());
	}

	// query
	friend bool MSB(const bit_field& bf)
	{
		return 1&(bf._value>>(bits-1));
	}

	friend bool LSB(const bit_field& bf)
	{
		return 1&bf._value;
	}

	bool bitAt(const int n) const
	{
		vassert(n>=0 && n<bits);
		return 0!=(_value&(((T)1)<<n));
	}
	
	bool operator [](const int n) const
	{
		return bitAt(n);
	}

	bool negative() const
	{
		return MSB(*this);
	}

	bool zero() const
	{
		return !_value;
	}

	bool belowMax() const
	{
		return 0!=((~_value)&MASK);
	}

	bool reachMax() const
	{
		return _value==MASK;
	}

	bool overflow() const
	{
		static_assert(bits<SIZE_IN_BITS(T), "overflow check is not available for this type");
		return 0!=(_value>>bits);
	}

	T lowbit() const
	{
		return LOW_BIT(_value);
	}

	bit_field plus(const T delta) const
	{
		return _unchecked_wrapper((_value+delta)&MASK);
	}

	bit_field minus(const T delta) const
	{
		return _unchecked_wrapper((_value-delta)&MASK);
	}

	// edit
	bit_field& operator += (const T delta)
	{
		_value=(_value+delta)&MASK;
		return *this;
	}

	bit_field& operator -= (const T delta)
	{
		_value=(_value-delta)&MASK;
		return *this;
	}

	bit_field& operator &= (const bit_field& other)
	{
		_value&=other._value;
		return *this;
	}

	bit_field& operator |= (const bit_field& other)
	{
		_value|=other._value;
		return *this;
	}

	bit_field& operator ^= (const bit_field& other)
	{
		_value^=other._value;
		return *this;
	}

	friend T inc(bit_field& bf)
	{
		++bf._value;
		return bf._value&=MASK;
	}

	friend T dec(bit_field& bf)
	{
		--bf._value;
		return bf._value&=MASK;
	}

	void selfRTrim()
	{
		_value=RTRIM(_value);
	}

	void selfDropLowbit()
	{
		_value&=(_value-1);
	}

	void selfNOT()
	{
		_value^=MASK; // faster
		/*
		return value=MASK&(~value); // slower but safer
		return value=MASK-value;
		*/
	}

	void selfSetMax()
	{
		_value=MASK;
	}

	void selfNEG()
	{
		_value=MASK&(-_value);
	}

	// ShiftLeft
	void selfShl(const int n)
	{
		assert(n>=0 && n<=bits);
		_value=(_value<<n)&MASK;
	}

	void selfShl1()
	{
		_value=(_value<<1)&MASK;
	}

	// ShiftRight
	void selfShr(const int n)
	{
		assert(n>=0 && n<=bits);
		_value>>=n;
	}

	void selfShr1()
	{
		_value>>=1;
	}

	// RotateLeftWithCarry
	void selfRcl(const bool carry)
	{
		_value=((_value<<1)&MASK)|(carry?1:0);
	}

	// RotateLeft
	void selfRol()
	{
		selfRcl(MSB(*this));
	}

	// RotateRightWithCarry
	void selfRcr(const bool carry)
	{
		_value=(_value>>1)|(carry?(1<<(bits-1)):0);
	}

	// RotateRight
	void selfRor()
	{
		selfRcr(LSB(*this));
	}
};