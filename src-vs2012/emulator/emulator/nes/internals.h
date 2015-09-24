#ifdef FAST_TYPE
    typedef unsigned _addr16_t;
    typedef unsigned _addr15_t;
    typedef unsigned _addr14_t;
    typedef unsigned _addr8_t;
    typedef unsigned _reg8_t;
    typedef unsigned _alutemp_t;
    typedef unsigned byte_t;
    typedef unsigned word_t;
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
#endif // EXACT_TYPE

// addr
typedef bit_field<_addr16_t,16>   maddr_t;
typedef bit_field<_addr15_t,15>   scroll_t,addr15_t;
typedef bit_field<_addr14_t,14>   vaddr_t,addr14_t;
typedef bit_field<_addr8_t,8>     addr8_t,saddr_t;

// cpu
typedef bit_field<_reg8_t,8>      reg_bit_field_t;
typedef byte_t              opcode_t;

// alu
typedef bit_field<byte_t,8>       value_t;
typedef bit_field<_alutemp_t,8>   alu_t;

// ppu
typedef _reg8_t             ioreg_t;
typedef bit_field<byte_t,8>       tileid_t;
typedef bit_field<uint8_t,6>      colorindex_t;
typedef bit_field<uint8_t,5>      palindex_t;

// color
typedef uint32_t            rgb32_t;
typedef uint16_t            rgb16_t,rgb15_t;

// others
typedef bit_field<unsigned,3>     off3_t;
typedef bit_field<unsigned,10>    off10_t;