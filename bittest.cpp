#include "macro.h"
#include "datatype.h"

static void testBIT()
{
    addr14_t videoAddr;

    static_assert(sizeof(_addr14_t)==sizeof(videoAddr),"BIT size error");

    //videoAddr=0xFFFF;
    videoAddr=0x3FFF;
    inc(videoAddr);
    assert(valueOf(videoAddr)==0 && videoAddr.isZero());
    printf("%X\n",valueOf(videoAddr));
    dec(videoAddr);
    assert(valueOf(videoAddr)==0x3FFF && !videoAddr.isZero());
    assert(videoAddr.isMax());
    printf("%X\n",valueOf(videoAddr));
    assert(videoAddr.MSB() && videoAddr.LSB() && videoAddr.isNegative());
    videoAddr.bitShr(2);
    assert(valueOf(videoAddr)==0xFFF);
    assert(!videoAddr.MSB() && !videoAddr.isNegative());

    videoAddr.bitShl(1);
    assert(!videoAddr.LSB());
    assert(valueOf(videoAddr)==0x1FFE);
    videoAddr.bitShl(14);
    assert(videoAddr.isZero());

    videoAddr.bitXorEqual(0x3FFF);
    videoAddr.bitAndEqual(0x2001);
    assert(videoAddr.isNegative() && videoAddr.MSB() && videoAddr.LSB());

    videoAddr=0x1010;
    videoAddr.bitRol();
    assert(videoAddr.MSB() && !videoAddr.LSB());

    videoAddr.bitRol();
    assert(!videoAddr.MSB() && videoAddr.LSB());
    assert(valueOf(videoAddr)==0x0041);

    videoAddr.bitRor();
    assert(valueOf(videoAddr)==0x2020);
    videoAddr.bitRor();
    assert(valueOf(videoAddr)==0x1010);

    videoAddr.bitNOT();
    assert(valueOf(videoAddr)==0x2FEF);

    videoAddr.bitDropLowbit();
    assert(valueOf(videoAddr)==0x2FEE);
    videoAddr.bitShl(8);
    assert(valueOf(videoAddr)==0x2E00);
    assert(videoAddr.lowbit()==0x200);
    videoAddr.bitDropLowbit();
    assert(valueOf(videoAddr)==0x2C00);

    videoAddr.bitRTrim();
    assert(valueOf(videoAddr)==0b1011);
    videoAddr.bitXorEqual(0b0011);
    videoAddr.bitRTrim();
    assert(valueOf(videoAddr)==0b1);

    videoAddr.sub(0xFFFFFFFF);
    assert(valueOf(videoAddr)==2);

    videoAddr.bitShl(12);
    assert(videoAddr.MSB());
    videoAddr.bitRcl(true);
    assert(valueOf(videoAddr)==1);

    videoAddr.bitRcr(true);
    assert(videoAddr.MSB());

    puts("*** testBIT() passed! ***");

    _alutemp_t sum=0x1FF;
    assert(safe_cast<alu_t>(sum).isOverflow());
    safe_cast<alu_t>(sum).bitShr(1);
    assert(safe_cast<alu_t>(sum).isOverflow()==false);
}

enum PSW {
    F_CARRY=0x1,
    F_ZERO=0x2,
    F_INTERRUPT_OFF=0x4,
    F_BCD=0x8,
    F_DECIMAL=F_BCD,
    F_BREAK=0x10,
    F_NOTUSED=0x20,
    F_OVERFLOW=0x40,
    F_NEGATIVE=0x80,
    F_SIGN=F_NEGATIVE,
    F_MULTIPLE=0x11,
    F_MULTIPLE2=0x44,
    F_MULTIPLE3=0x70,
    F_MAX=0x80
};

static void testFLAG()
{
    FLAG<_reg8_t,PSW,8> P;
    static_assert(sizeof(_reg8_t)==sizeof(P),"FLAG size error");
    printf("P=%X\n",ValueOf(P));
    P.clearAll();
    assert(!P.any());

    P.set(F_ZERO);
    P-=F_BCD;
    assert(ValueOf(P)==F_ZERO);
    P^=F_MULTIPLE;
    assert(ValueOf(P)==(F_ZERO|F_MULTIPLE));
    P^=F_ZERO;
    assert(ValueOf(P)==F_MULTIPLE);
    P.update(F_BCD,1);
    //P.update(F_BCD,2);
    P.update(F_BCD,0);

    P.clearAll();
    P|=F_NEGATIVE;
    P.set(F_BREAK);
    assert(P.query(F_MULTIPLE)==0x10);
    P=0xFF;
    assert(P.query(F_MULTIPLE)==0x11);
    assert(P.query(F_MULTIPLE2)==0b10001);
    P.update(F_MULTIPLE2,0);
    assert(ValueOf(P)==0xFF-F_MULTIPLE2);

    P.clearAll();
    P.update(F_MULTIPLE,0x11);
    assert(ValueOf(P)==F_MULTIPLE);

    P.setAll();
    P.copy(F_MULTIPLE3,0b1101,1);
    assert(ValueOf(P)==0x8F);

    P.setAll();
    assert(P.query(F_MULTIPLE2)==0x11);
    assert(P[F_MAX] && P.test(F_MULTIPLE));
    P.update(F_MULTIPLE2,0x10);
    assert(ValueOf(P)==0xFF-4);
    assert(P(F_MULTIPLE2)==0x10);
    assert(!P.test(F_MULTIPLE2));

    P.clearAll();
    P.set(F_MULTIPLE3);
    P.inc(F_MULTIPLE3);
    assert(!P.any());
    P.inc(F_MULTIPLE3);
    assert(P(F_MULTIPLE3)==1);
    assert(P.inc(F_MULTIPLE3)==2);
    puts("*** testFLAG() passed! ***");
}

void BITTests()
{
    testBIT();
    testFLAG();
}
