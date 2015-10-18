#include "../stdafx.h"

#include "../macros.h"
#include "../unittest/framework.h"
#include "types.h"

#include "../nes/internals.h"

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

class BitFieldTest : public TestCase
{
public:
	virtual const char* name()
	{
		return "BitField Unit Test";
	}

	virtual TestResult run()
	{
		addr14_t videoAddr;
		addr14_t typeMax(addr14_t::MAX);

		static_assert(sizeof(_addr14_t)==sizeof(videoAddr),"BIT size error");

		printf("videoAddr=%X\n",videoAddr);
		// out of range test
		// videoAddr=0xFFFF;

		videoAddr=0x3FFF;
		tassert(!videoAddr.belowMax());
		for (int i=0;i<14;i++)
		{
			tassert(videoAddr[i] == (1&(0x3FFF>>i)));
		}
		tassert(videoAddr==typeMax);

		// plus() won't change the value
		tassert(videoAddr.plus(1)==0);

		inc(videoAddr);
		tassert(videoAddr==0 && videoAddr.zero());
		printf("%X\n",videoAddr);

		dec(videoAddr);
		tassert(videoAddr==0x3FFF && !videoAddr.zero());
		tassert(videoAddr.reachMax());
		printf("%X\n",valueOf(videoAddr));
		tassert(MSB(videoAddr) && LSB(videoAddr) && videoAddr.negative());
		videoAddr.selfShr(2);
		tassert(videoAddr==0xFFF);
		tassert(!MSB(videoAddr) && !videoAddr.negative());

		videoAddr.selfShl(1);
		tassert(!LSB(videoAddr));
		tassert(videoAddr==0x1FFE);
		videoAddr.selfShl(14);
		tassert(videoAddr.zero());

		videoAddr^=addr14_t(0x3FFF);
		videoAddr&=addr14_t(0x2001);
		tassert(videoAddr.negative() && MSB(videoAddr) && LSB(videoAddr));

		videoAddr=0x1010;
		videoAddr.selfRol();
		tassert(MSB(videoAddr) && !LSB(videoAddr));

		videoAddr.selfRol();
		tassert(!MSB(videoAddr) && LSB(videoAddr));
		tassert(videoAddr==0x0041);

		videoAddr.selfRor();
		tassert(videoAddr==0x2020);
		videoAddr.selfRor();
		tassert(videoAddr==0x1010);

		videoAddr.selfNOT();
		tassert(videoAddr==0x2FEF);

		videoAddr.selfDropLowbit();
		tassert(videoAddr==0x2FEE);
		videoAddr.selfShl(8);
		tassert(videoAddr==0x2E00);
		tassert(videoAddr.lowbit()==0x200);
		videoAddr.selfDropLowbit();
		tassert(videoAddr==0x2C00);

		videoAddr.selfRTrim();
		tassert(videoAddr==0xB);
		videoAddr^=addr14_t(0x3);
		videoAddr.selfRTrim();
		tassert(videoAddr==0x1);

		videoAddr-=(0xFFFFFFFF);
		tassert(videoAddr==2);

		videoAddr.selfShl(12);
		tassert(MSB(videoAddr));
		videoAddr.selfRcl(true);
		tassert(videoAddr==1);

		videoAddr.selfRcr(true);
		tassert(MSB(videoAddr));

		_alutemp_t sum=0x1FF;
		tassert(safe_cast<alu_t>(sum).overflow());
		safe_cast<alu_t>(sum).selfShr(1);
		tassert(safe_cast<alu_t>(sum).overflow()==false);

		return SUCCESS;
	}
};

class FlagSetTest : public TestCase
{
public:
	virtual const char* name()
	{
		return "FlagSet Unit Test";
	}

	virtual TestResult run()
	{
		flag_set<_reg8_t,PSW,8> P;
		static_assert(sizeof(_reg8_t)==sizeof(P),"FLAG size error");
		printf("P=%X\n",valueOf(P));
		P.clearAll();
		tassert(!P.any());

		P.set(F_ZERO);
		P-=F_BCD;
		tassert(valueOf(P)==F_ZERO);
		P^=F_MULTIPLE;
		tassert(valueOf(P)==(F_ZERO|F_MULTIPLE));
		P^=F_ZERO;
		tassert(valueOf(P)==F_MULTIPLE);
		P.update(F_BCD,1);
		//P.update(F_BCD,2);
		P.update(F_BCD,0);

		P.clearAll();
		P|=F_NEGATIVE;
		P.set(F_BREAK);
		tassert(P.select(F_MULTIPLE)==0x10);
		
		P=bit_field<_reg8_t,8>(0xFF);
		tassert(P.select(F_MULTIPLE)==0x11);
		tassert(P.select(F_MULTIPLE2)==0x11);
		P.update(F_MULTIPLE2,0);
		tassert(valueOf(P)==0xFF-F_MULTIPLE2);

		P.clearAll();
		P.update(F_MULTIPLE,0x11);
		tassert(valueOf(P)==F_MULTIPLE);

		P.setAll();
		P.copy(F_MULTIPLE3,0xD,1);
		tassert(valueOf(P)==0x8F);

		P.setAll();
		tassert(P.select(F_MULTIPLE2)==0x11);
		tassert(P[F_MAX] && P.test(F_MULTIPLE));
		P.update(F_MULTIPLE2,0x10);
		tassert(valueOf(P)==0xFF-4);
		tassert(P(F_MULTIPLE2)==0x10);
		tassert(!P.test(F_MULTIPLE2));

		P.clearAll();
		P.set(F_MULTIPLE3);
		P.inc(F_MULTIPLE3);
		tassert(!P.any());
		P.inc(F_MULTIPLE3);
		tassert(P(F_MULTIPLE3)==1);
		tassert(P.inc(F_MULTIPLE3)==2);

		return SUCCESS;
	}
};

class BFFSInteropTest : public TestCase
{
public:
	virtual const char* name()
	{
		return "BitField&FlagSet Interoperability Test";
	}

	virtual TestResult run()
	{
		maddr8_t bf;
		flag_set<_reg8_t,PSW,8> fs;

		bf=0x81;
		fs=bf;
		tassert(valueOf(fs)==valueOf(bf));

		tassert(valueOf(bf.asFlagSet<PSW>())==fs.asBitField());

		maddr8_t bf2(fs);

		tassert(bf2 == bf);

		return SUCCESS;
	}
};

registerTestCase(BitFieldTest);
registerTestCase(FlagSetTest);
registerTestCase(BFFSInteropTest);