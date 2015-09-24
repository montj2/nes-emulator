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
		assert(!videoAddr.belowMax());
		for (int i=0;i<14;i++)
		{
			assert(videoAddr[i] == (1&(0x3FFF>>i)));
		}
		assert(videoAddr==typeMax);

		// plus() won't change the value
		assert(videoAddr.plus(1)==0);

		inc(videoAddr);
		assert(videoAddr==0 && videoAddr.zero());
		printf("%X\n",videoAddr);

		dec(videoAddr);
		assert(videoAddr==0x3FFF && !videoAddr.zero());
		assert(videoAddr.isMax());
		printf("%X\n",valueOf(videoAddr));
		assert(MSB(videoAddr) && LSB(videoAddr) && videoAddr.negative());
		videoAddr.selfShr(2);
		assert(videoAddr==0xFFF);
		assert(!MSB(videoAddr) && !videoAddr.negative());

		videoAddr.selfShl(1);
		assert(!LSB(videoAddr));
		assert(videoAddr==0x1FFE);
		videoAddr.selfShl(14);
		assert(videoAddr.zero());

		videoAddr^=addr14_t(0x3FFF);
		videoAddr&=addr14_t(0x2001);
		assert(videoAddr.negative() && MSB(videoAddr) && LSB(videoAddr));

		videoAddr=0x1010;
		videoAddr.selfRol();
		assert(MSB(videoAddr) && !LSB(videoAddr));

		videoAddr.selfRol();
		assert(!MSB(videoAddr) && LSB(videoAddr));
		assert(videoAddr==0x0041);

		videoAddr.selfRor();
		assert(videoAddr==0x2020);
		videoAddr.selfRor();
		assert(videoAddr==0x1010);

		videoAddr.selfNOT();
		assert(videoAddr==0x2FEF);

		videoAddr.selfDropLowbit();
		assert(videoAddr==0x2FEE);
		videoAddr.selfShl(8);
		assert(videoAddr==0x2E00);
		assert(videoAddr.lowbit()==0x200);
		videoAddr.selfDropLowbit();
		assert(videoAddr==0x2C00);

		videoAddr.selfRTrim();
		assert(videoAddr==0xB);
		videoAddr^=addr14_t(0x3);
		videoAddr.selfRTrim();
		assert(videoAddr==0x1);

		videoAddr-=(0xFFFFFFFF);
		assert(videoAddr==2);

		videoAddr.selfShl(12);
		assert(MSB(videoAddr));
		videoAddr.selfRcl(true);
		assert(videoAddr==1);

		videoAddr.selfRcr(true);
		assert(MSB(videoAddr));

		_alutemp_t sum=0x1FF;
		assert(safe_cast<alu_t>(sum).overflow());
		safe_cast<alu_t>(sum).selfShr(1);
		assert(safe_cast<alu_t>(sum).overflow()==false);

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
		assert(!P.any());

		P.set(F_ZERO);
		P-=F_BCD;
		assert(valueOf(P)==F_ZERO);
		P^=F_MULTIPLE;
		assert(valueOf(P)==(F_ZERO|F_MULTIPLE));
		P^=F_ZERO;
		assert(valueOf(P)==F_MULTIPLE);
		P.update(F_BCD,1);
		//P.update(F_BCD,2);
		P.update(F_BCD,0);

		P.clearAll();
		P|=F_NEGATIVE;
		P.set(F_BREAK);
		assert(P.select(F_MULTIPLE)==0x10);
		
		P=bit_field<_reg8_t,8>(0xFF);
		assert(P.select(F_MULTIPLE)==0x11);
		assert(P.select(F_MULTIPLE2)==0x11);
		P.update(F_MULTIPLE2,0);
		assert(valueOf(P)==0xFF-F_MULTIPLE2);

		P.clearAll();
		P.update(F_MULTIPLE,0x11);
		assert(valueOf(P)==F_MULTIPLE);

		P.setAll();
		P.copy(F_MULTIPLE3,0xD,1);
		assert(valueOf(P)==0x8F);

		P.setAll();
		assert(P.select(F_MULTIPLE2)==0x11);
		assert(P[F_MAX] && P.test(F_MULTIPLE));
		P.update(F_MULTIPLE2,0x10);
		assert(valueOf(P)==0xFF-4);
		assert(P(F_MULTIPLE2)==0x10);
		assert(!P.test(F_MULTIPLE2));

		P.clearAll();
		P.set(F_MULTIPLE3);
		P.inc(F_MULTIPLE3);
		assert(!P.any());
		P.inc(F_MULTIPLE3);
		assert(P(F_MULTIPLE3)==1);
		assert(P.inc(F_MULTIPLE3)==2);

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
		addr8_t bf;
		flag_set<_reg8_t,PSW,8> fs;

		bf=0x81;
		fs=bf;
		assert(valueOf(fs)==valueOf(bf));

		assert(valueOf(bf.asFlagSet<PSW>())==fs.asBitField());

		addr8_t bf2(fs);

		assert(bf2 == bf);

		return SUCCESS;
	}
};

registerTestCase(BitFieldTest);
registerTestCase(FlagSetTest);
registerTestCase(BFFSInteropTest);