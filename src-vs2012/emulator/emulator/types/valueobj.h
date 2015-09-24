template <class VT, class DT = VT>
class value_object
{
public:
	typedef DT DataTp;
	typedef VT ValueTp;

	value_object() {}
	value_object(const VT& value): _value(value) {}

	// value getter
	friend const VT& valueOf(const value_object& vo) {return vo._value;}

	// value setter is not available in this base class

protected:
	DT _value;
};

template <class VT, class DT = VT>
class transparent_value_object : public value_object<VT, DT>
{
public:
	transparent_value_object() {}
	transparent_value_object(const VT& value) {}

	// transparent value getter
	operator const VT&() const {return _value;}

	// transparent value setter
	transparent_value_object& operator = (const value_object& other) {_value = other._value}
};