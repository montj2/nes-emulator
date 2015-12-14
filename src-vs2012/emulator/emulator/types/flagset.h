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
	
	// auto conversion is also fine
	#ifndef DISABLE_FLAGSET_AUTO_UNBOXING
		operator bit_field<T,bits>&() const {return *(bit_field<T,bits>*)this;}
	#endif

	// safe assignment
	flag_set& operator =(const bit_field<T,bits>& rhs)
	{
		_value=valueOf(rhs);
		return *this;
	}

	// safe auto boxing
	flag_set(const bit_field<T,bits>& rhs):value_object(0)
	{
		_value=valueOf(rhs);
	}

	bool any() const
	{
		return _value!=0;
	}

	bool test(const ET e) const
	{
		return (_value&(T)e)==(T)e;
	}

	bool operator [](const ET e) const
	{
		vassert(SINGLE_BIT((T)e));
		return test(e);
	}

	void flip(const ET e)
	{
		vassert(SINGLE_BIT((T)e));
		_value^=e;
	}

	void operator ^=(const ET e)
	{
		return flip(e);
	}

	void set(const ET e)
	{
		_value|=(T)e;
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

	void change(const ET e, const bool enabled)
	{
		_value=(_value&(~(T)e))|((-(int)enabled)&(T)e);
	}

	template <ET e>
	void change(const int enabled)
	{
		_value=(_value&(~(T)e))|((-enabled)&(T)e);
	}

	void setAll()
	{
		_value=MASK;
	}

	void clearAll()
	{
		_value=0;
	}

	// advanced bit manipulation
	T select(const ET e) const
	{
		return SELECT_FIELD(_value, (T)e);
	}

	T operator ()(const ET e) const
	{
		return SELECT_FIELD(_value, (T)e);
	}

	void update(const ET e, const T newValue)
	{
		assert(0==(newValue&(~RTRIM((T)e))));
		UPDATE_FIELD(_value, (T)e, newValue);
	}

	template <ET e>
	void update(const T newValue)
	{
		assert(0==(newValue&(~RTRIM((T)e))));
		UPDATE_FIELD(_value, (T)e, newValue);
	}

	T inc(const ET e)
	{
		vassert(!SINGLE_BIT((T)e));
		INC_FIELD(_value, (T)e);
		return SELECT_FIELD(_value, (T)e);;
	}

	template <ET e>
	T inc()
	{
		STATIC_ASSERT(!SINGLE_BIT((T)e));
		INC_FIELD(_value, (T)e);
		return SELECT_FIELD(_value, (T)e);;
	}

	// set the specified field to the lower `length` bits of (src>>shift)
	void copy(const ET e, const T src, const int shift, const int length=1)
	{
		vassert(0==(BIT_MASK(T, length)&(~RTRIM((T)e))));
		update(e, (src>>shift)&BIT_MASK(T, length));
	}

	template <ET e, int shift, int length>
	void copy(const T src)
	{
		const T field=(T)e;
		STATIC_ASSERT(0==(BIT_MASK(T, length)&(~RTRIM(field))));
		const T newValue = (src>>shift)&BIT_MASK(T, length);
		UPDATE_FIELD(_value, field, newValue);
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
