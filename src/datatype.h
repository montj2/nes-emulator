template <typename T,int bits=TYPE_BITS(T)> class BIT;
template <typename T,typename ET,int bits=TYPE_BITS(T)> class FLAG;

#include "BIT.H"
//#include "TOKEN.H"
#include "FLAG.H"

#ifdef FASTTYPE
    typedef unsigned _addr16_t;
    typedef unsigned _addr15_t;
    typedef unsigned _addr14_t;
    typedef unsigned _addr8_t;
    typedef unsigned _reg8_t;
    typedef unsigned _alutemp_t;
    typedef unsigned byte_t;
    typedef unsigned word_t;
#endif // FASTTYPE

#ifdef EXACTTYPE
    typedef uint16_t _addr16_t;
    typedef uint16_t _addr15_t;
    typedef uint16_t _addr14_t;
    typedef uint8_t  _addr8_t;
    typedef uint8_t  _reg8_t;
    typedef uint16_t _alutemp_t;
    typedef uint8_t  byte_t;
    typedef uint16_t word_t;
#endif // EXACTTYPE

// addr
typedef BIT<_addr16_t,16>   maddr_t;
typedef BIT<_addr15_t,15>   scroll_t,addr15_t;
typedef BIT<_addr14_t,14>   vaddr_t,addr14_t;
typedef BIT<_addr8_t,8>     addr8_t,saddr_t;

// cpu
typedef BIT<_reg8_t,8>      reg_bit_t;
typedef byte_t              opcode_t;

// alu
typedef BIT<byte_t,8>       value_t;
typedef BIT<_alutemp_t,8>   alu_t;

// ppu
typedef _reg8_t             ioreg_t;
typedef BIT<byte_t,8>       tileid_t;
typedef BIT<uint8_t,6>      colorindex_t;
typedef BIT<uint8_t,5>      palindex_t;

// color
typedef uint32_t            rgb32_t;
typedef uint16_t            rgb16_t,rgb15_t;

// others
typedef BIT<unsigned,3>     off3_t;
typedef BIT<unsigned,10>    off10_t;

inline word_t makeWord(const byte_t pageoffset,const byte_t pagenumber) {
    return pageoffset|(pagenumber<<8);
}

inline rgb32_t Rgb32(const byte_t r,const byte_t g,const byte_t b) {
    return b|(g<<8)|(r<<16);
}
