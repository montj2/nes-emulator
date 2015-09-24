template <typename T, typename ET, int bits=SIZE_IN_BITS(T)>
//  integral type that represents a set of flags
class flag_set: public value_object<T> {
public:
	enum :T
	{
		MASK=BIT_MASK(T,bits)
	};

public:
	// default ctor
	flag_set():value_object(0) {} // value initialized to zero

	// bit field converter
	bit_field<T,bits>& asBitField()
	{
		return *(bit_field<T,bits>*)this;
	}

	// safe assignment
	flag_set& operator =(const bit_field<T,bits>& rhs)
	{
		_value=valueOf(rhs);
		return *this;
	}

	bool any() const
	{
		return _value!=0;
	}

	bool test(const ET e) const
	{
		return (_value&e)==e;
	}

	bool operator [](const ET e) const
	{
		vassert(SINGLE_BIT(e));
		return test(e);
	}

	void flip(const ET e)
	{
		_value^=e;
	}

	void operator ^=(const ET e)
	{
		return flip(e);
	}

	void set(const ET e)
	{
		_value|=e;
	}

	void operator |=(const ET e)
	{
		set(e);
	}

	void clear(const ET e)
	{
		_value&=(~(T)e);
	}

	void operator -=(const ET e)
	{
		clear(e);
	}

	void change(const ET e, const bool enabled) {
		enabled? set(e): clear(e);
	}

	void setAll()
	{
		_value=MASK;
	}

	void clearAll()
	{
		_value=0;
	}

	// advanced bit-field operation
	T select(const ET e) const
	{
		const T d=(T)e;
		return (_value&d)/LOW_BIT(d);
	}

	T operator ()(const ET e) const
	{
		return select(e);
	}

	void update(const ET e, const T newValue)
	{
		const T field=(T)e;
		assert(0==(newValue&(~RTRIM(field))));
		_value=(_value&(~field))|((newValue*LOW_BIT(field))&field);
	}

	T inc(const ET e)
	{
		const T field=(T)e;
		vassert(!SINGLE_BIT(field));
		_value=(_value&(~field))|(((_value&field)+LOW_BIT(field))&field);
		return select(e);
	}

	// set the specified field to the lower `length` bits of (src>>shift)
	void copy(const ET e, const T src, const int shift, const int length=1)
	{
		vassert(0==(BIT_MASK(T, length)&(~RTRIM((T)e))));
		update(e, (src>>shift)&BIT_MASK(T, length));
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
};
