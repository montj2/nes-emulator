#include "../stdafx.h"

// local header files
#include "../macros.h"
#include "../types/types.h"
#include "../unittest/framework.h"

#include "internals.h"
#include "cpu.h"
#include "mmc.h"

enum PSW
{
    F_CARRY=0x1,
    F_ZERO=0x2,
    F_INTERRUPT_OFF=0x4,
    F_BCD=0x8,
	F__DECIMAL=F_BCD,
    F_BREAK=0x10,
    F_NOTUSED=0x20,
    F_OVERFLOW=0x40,
    F_NEGATIVE=0x80,
	F__SIGN=F_NEGATIVE,
	F__NV=F_NEGATIVE|F_OVERFLOW
};

#pragma region 6502RegisterFile

	__declspec(align(32))
	static _reg8_t		A; // accumulator
	static _reg8_t		X, Y; // index
	static addr8_t		SP; // stack pointer
	static flag_set<_reg8_t, PSW, 8> P; // status
	static maddr_t		PC; // program counter

	// wrappers
	#define regA fast_cast(A, reg_bit_field_t)
	#define regX fast_cast(X, reg_bit_field_t)
	#define regY fast_cast(Y, reg_bit_field_t)
	#define M fast_cast(value, operand_t)
	#define SUM fast_cast(temp, alu_t)

#pragma endregion

namespace stack
{
	static inline void pushByte(const byte_t byte)
	{
		#ifdef MONITOR_STACK
			printf("[S] Push 0x%02X to $%02X\n",byte,valueOf(SP));
		#endif
		
		ramSt[valueOf(SP)]=byte;
		
		dec(SP);
		#ifndef ALLOW_ADDRESS_WRAP
			assert(SP.belowMax());
		#endif
	}

	static inline void pushReg(const reg_bit_field_t& reg)
	{
		pushByte(valueOf(reg));
	}

	static inline void pushWord(const word_t word)
	{
		dec(SP);
		#ifdef MONITOR_STACK
			printf("[S] Push 0x%04X to $%02X\n",word,valueOf(SP));
		#endif
		assert(SP.belowMax());

		*(uint16_t*)&(ramSt[valueOf(SP)])=(word);

		dec(SP);
		#ifndef ALLOW_ADDRESS_WRAP
			assert(SP.belowMax());
		#endif
	}

	static inline void pushPC()
	{
		pushWord(PC);
	}

	static inline byte_t popByte()
	{
		#ifndef ALLOW_ADDRESS_WRAP
			assert(SP.belowMax());
		#endif
		return ramSt[inc(SP)];
	}

	static inline word_t popWord()
	{
		#ifndef ALLOW_ADDRESS_WRAP
			assert(SP.belowMax() && SP.plus(1).belowMax());
		#endif
		SP+=2;
		return *(uint16_t*)&(ramSt[SP.minus(1)]);
	}
}

namespace bitshift
{
	template <class T,int bits>
	static inline void ASL(bit_field<T,bits>& operand) {
		// Arithmetic Shift Left
		P.change(F_CARRY, MSB(operand));
		operand.selfShl1();
		status::setNZ(operand);
	}

	template <class T,int bits>
	static inline void LSR(bit_field<T,bits>& operand) {
		// Logical Shift Right
		P.change(F_CARRY, LSB(operand));
		operand.selfShr1();
		status::setZ(operand);
		P.clear(F__SIGN);
	}

	template <class T,int bits>
	static inline void ROL(bit_field<T,bits>& operand) {
		// Rotate Left With Carry
		const bool newCarry=MSB(operand);
		operand.selfRcl(P[F_CARRY]);
		P.change(F_CARRY, newCarry);
		status::setNZ(operand);
	}

	template <class T,int bits>
	static inline void ROR(bit_field<T,bits>& operand) {
		// Rotate Right With Carry
		const bool newCarry=LSB(operand);
		operand.selfRcr(P[F_CARRY]);
		P.change(F_CARRY, newCarry);
		status::setNZ(operand);
	}
}

namespace status
{
	template <class T,int bits>
	static inline void setZ(const bit_field<T,bits>& result)
	{
		P.change(F_ZERO, result.zero());
	}

	template <class T,int bits>
	static inline void setN(const bit_field<T,bits>& result)
	{
		P.change(F_NEGATIVE, result.negative());
	}

	template <class T,int bits>
	static inline void setNZ(const bit_field<T,bits>& result)
	{
		P.change(F_NEGATIVE, result.negative());
		P.change(F_ZERO, result.zero());
	}

	template <class T,int bits>
	static inline void setV(const bit_field<T,bits>& result)
	{
		P.change(F_OVERFLOW, result.overflow());
	}

	template <class T,int bits>
	static inline void setNV(const bit_field<T,bits>& result)
	{
		STATIC_ASSERT((int)F_NEGATIVE == 1<<7 && (int)F_OVERFLOW == 1<<6);
		P.copy(F__NV, result, 6, 2);
	}
}

// unit tests
class CPUTest : public TestCase
{
public:
	virtual const char* name()
	{
		return "CPU Unit Test";
	}

	virtual TestResult run()
	{
		P.clearAll();

		A=0;
		status::setZ(regA);
		tassert(P[F_ZERO]);

		X=0xFF;
		status::setNZ(regX);
		tassert(!P[F_ZERO] && P[F_NEGATIVE]);

		byte_t value;
		_alutemp_t temp;

		value=0x10;
		temp=value<<4;
		P.change(F_CARRY,SUM.overflow());
		tassert(P[F_CARRY]);

		M=F_NEGATIVE;
		status::setNV(M);
		tassert(P[F_NEGATIVE] && !P[F_OVERFLOW]);

		temp=0x100;
		status::setNV(SUM);
		tassert(!P[F_NEGATIVE] && !P[F_OVERFLOW]);

		P|=F_OVERFLOW;
		tassert(P[F_OVERFLOW]);

		Y=0x80;
		bitshift::ASL(regY);
		tassert(Y==0 && P[F_CARRY] && P[F_ZERO] && !P[F_NEGATIVE]);

		value=0x41;
		bitshift::ASL(M);
		tassert(!P[F_CARRY] && !P[F_ZERO] && P[F_NEGATIVE]);

		A=0x80;
		bitshift::LSR(regA);
		tassert(!P[F_CARRY] && !P[F_ZERO] && !P[F_NEGATIVE]);

		value=0x01;
		bitshift::LSR(M);
		tassert(P[F_CARRY] && P[F_ZERO] && !P[F_NEGATIVE]);

		X=0x40;
		bitshift::ROR(regX);
		tassert(X==0xA0);
		tassert(!P[F_CARRY] && !P[F_ZERO] && P[F_NEGATIVE]);

		Y=1;
		bitshift::ROR(regY);
		tassert(P[F_ZERO] && P[F_CARRY] && !P[F_NEGATIVE]);

		bitshift::ROL(regY);
		tassert(Y==1 && !P[F_NEGATIVE] && !P[F_CARRY] && !P[F_ZERO]);

		P|=F_CARRY;
		bitshift::ROL(regY);
		tassert(Y==3);

		SP.selfSetMax();
		stack::pushReg(regY);

		PC=0xFFAA;
		stack::pushPC();

		byte_t tmp;
		tmp=stack::popByte();
		tassert(tmp==0xAA);

		tmp=stack::popByte();
		tassert(tmp==0xFF);

		tmp=stack::popByte();
		tassert(tmp==0x03);

		stack::pushPC();
		stack::pushReg(regY);
		
		word_t tmp16;
		tmp16=stack::popWord();
		tassert(tmp16==0xAA03);
		
		tmp=stack::popByte();
		tassert(tmp==0xFF);
		
		printf("[ ] Register memory at 0x%p\n", &A);

		return SUCCESS;
	}
};

registerTestCase(CPUTest);