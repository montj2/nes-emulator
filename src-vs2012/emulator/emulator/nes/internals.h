// internal type definitions
#ifdef FAST_TYPE
    typedef unsigned _addr16_t;
    typedef unsigned _addr15_t;
    typedef unsigned _addr14_t;
    typedef unsigned _addr8_t;
    typedef unsigned _reg8_t;
    typedef unsigned _alutemp_t;
    typedef unsigned byte_t;
    typedef unsigned word_t;
	typedef unsigned uint_t;
#endif // FAST_TYPE

#ifdef EXACT_TYPE
    typedef uint16_t _addr16_t;
    typedef uint16_t _addr15_t;
    typedef uint16_t _addr14_t;
    typedef uint8_t  _addr8_t;
    typedef uint8_t  _reg8_t;
    typedef uint16_t _alutemp_t;
    typedef uint8_t  byte_t;
    typedef uint16_t word_t;
	typedef uint32_t uint_t;
#endif // EXACT_TYPE

// address
typedef bit_field<_addr16_t,16> maddr_t;
typedef bit_field<_addr15_t,15> scroll_t, addr15_t;
typedef bit_field<_addr14_t,14> vaddr_t, addr14_t;
typedef bit_field<_addr8_t,8> maddr8_t, saddr_t;

// cpu
typedef byte_t opcode_t;
typedef byte_t operand_t;

// alu
typedef bit_field<byte_t,8> operandb_t;
typedef bit_field<word_t,16> operandw_t;
typedef bit_field<_alutemp_t,8> alu_t;

// ppu
typedef _reg8_t ioreg_t;
typedef bit_field<byte_t,8> tileid_t;
typedef bit_field<uint8_t,6> colorindex_t;
typedef bit_field<uint8_t,5> palindex_t;

// color
typedef uint32_t rgb32_t;
typedef uint16_t rgb16_t, rgb15_t;

// others
typedef bit_field<unsigned,3> offset3_t;
typedef bit_field<unsigned,10> offset10_t;

// utils
inline word_t makeWord(byte_t lo, byte_t hi)
{
	vassert(lo<=0xFF && hi<=0xFF);
	return ((word_t)lo)|(((word_t)hi)<<8);
}

inline rgb32_t Rgb32(const byte_t r,const byte_t g,const byte_t b)
{
    return b|(g<<8)|(r<<16);
}

// errors
enum EMUERROR {
	INVALID_ROM=1,
	INVALID_MEMORY_ACCESS,
	INVALID_INSTRUCTION,
	ILLEGAL_OPERATION
};

enum EMUERRORSUBTYPE {
	// INVALID_ROM
	INVALID_FILE_SIGNATURE,
	INVALID_ROM_CONFIG,
	UNEXPECTED_END_OF_FILE,
	UNSUPPORTED_MAPPER_TYPE,

	// INVALID_MEMORY_ACCESS
	MAPPER_FAILURE,
	ADDRESS_OUT_OF_RANGE,
	ILLEGAL_ADDRESS_WARP,
	MEMORY_NOT_EXECUTABLE,
	MEMORY_CANT_BE_READ,
	MEMORY_CANT_BE_WRITTEN,
	MEMORY_CANT_BE_COPIED,

	// INVALID_INSTRUCTION
	INVALID_OPCODE,
	INVALID_ADDRESS_MODE,

	// ILLEGAL_OPERATION
	IRQ_ALREADY_PENDING
};
