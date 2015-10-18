#include "../stdafx.h"

// local header files
#include "../macros.h"
#include "../types/types.h"
#include "../unittest/framework.h"

#include "internals.h"
#include "debug.h"
#include "mmc.h"
#include "opcodes.h"
#include "cpu.h"

// Register file
__declspec(align(32)) // try to make registers fit into a cache line of host CPU
static _reg8_t		A; // accumulator
static _reg8_t		X, Y; // index
static maddr8_t		SP; // stack pointer
static flag_set<_reg8_t, PSW, 8> P; // status
static maddr_t		PC; // program counter

static maddr_t		addr; // effective address
#define EA addr

static byte_t		value; // operand

// alias with wrapping for register file
typedef bit_field<_reg8_t, 8> reg_bit_field_t;
#define regA fast_cast(A, reg_bit_field_t)
#define regX fast_cast(X, reg_bit_field_t)
#define regY fast_cast(Y, reg_bit_field_t)
#define M fast_cast(value, operandb_t)
#define SUM fast_cast(temp, alu_t)

// Run-time statistics
#ifdef WANT_STATISTICS
	static long long totInstructions;
	static long long totCycles;
	static long long totInterrupts;
	static long long numInstructionsPerOpcode[(int)_INS_MAX];
	static long long numInstructionsPerAdrMode[(int)_ADR_MAX];
	#define STAT_ADD(VAR, INC) VAR += INC
#else
	#define STAT_ADD(VAR, INC) (void)0
#endif

namespace interrupt
{
	static const maddr_t VECTOR_NMI(0xFFFA);
	static const maddr_t VECTOR_RESET(0xFFFC);
	static const maddr_t VECTOR_BREAK(0xFFFE);

	static flag_set<_reg8_t, IRQ, 8> pendingIRQs;

	static void clearAll()
	{
		pendingIRQs.clearAll();
	}

	static void clear(IRQ type)
	{
		assert(pendingIRQs[type]);
		pendingIRQs.clear(type);
	}

	void request(const IRQ type)
	{
		vassert(type != IRQ::NONE);
		ERROR_IF(pendingIRQs[type], ILLEGAL_OPERATION, IRQ_ALREADY_PENDING);

		pendingIRQs.set(type);
		STAT_ADD(totInterrupts, 1);
	}

	// return address of interrupt handler
	static maddr_t handler(const IRQ type)
	{
		vassert(type != IRQ::NONE);

		maddr_t vector;
		switch (type)
		{
		case IRQ::RST: vector=VECTOR_RESET; break;
		case IRQ::BRK: vector=VECTOR_BREAK; break;
		case IRQ::NMI: vector=VECTOR_NMI; break;
		}
		return maddr_t(mmc::fetchWordOperand(vector));
	}

	static bool pending()
	{
		return pendingIRQs.any();
	}

	bool pending(const IRQ type)
	{
		vassert(type != IRQ::NONE);
		return pendingIRQs[type];
	}

	// return current highest-priority IRQ
	static IRQ current()
	{
		if (pendingIRQs[IRQ::RST]) return IRQ::RST;
		if (pendingIRQs[IRQ::NMI]) return IRQ::NMI;
		if (pendingIRQs[IRQ::BRK]) return IRQ::BRK;
		return IRQ::NONE;
	}
}

namespace stack
{
	static inline void pushByte(const byte_t byte)
	{
#ifdef MONITOR_STACK
		printf("[S] Push 0x%02X to $%02X\n",byte,valueOf(SP));
#endif

		ramSt[SP]=byte;

		dec(SP);
#ifndef ALLOW_ADDRESS_WRAP
		ERROR_IF(SP.reachMax(), INVALID_MEMORY_ACCESS, ILLEGAL_ADDRESS_WARP);
#endif
	}

	static inline void pushReg(const reg_bit_field_t& reg)
	{
		pushByte(reg);
	}

	static inline void pushWord(const word_t word)
	{
		dec(SP);
#ifdef MONITOR_STACK
		printf("[S] Push 0x%04X to $%02X\n",word,valueOf(SP));
#endif
		FATAL_ERROR_IF(SP.reachMax(), INVALID_MEMORY_ACCESS, ILLEGAL_ADDRESS_WARP);

		*(uint16_t*)&(ramSt[SP])=(word);

		dec(SP);
#ifndef ALLOW_ADDRESS_WRAP
		ERROR_IF(SP.reachMax(), INVALID_MEMORY_ACCESS, ILLEGAL_ADDRESS_WARP);
#endif
	}

	static inline void pushPC()
	{
		pushWord(PC);
	}

	static inline byte_t popByte()
	{
#ifndef ALLOW_ADDRESS_WRAP
		ERROR_IF(SP.reachMax(), INVALID_MEMORY_ACCESS, ILLEGAL_ADDRESS_WARP);
#endif
		return ramSt[inc(SP)];
	}

	static inline word_t popWord()
	{
#ifndef ALLOW_ADDRESS_WRAP
		ERROR_IF(SP.plus(1).reachMax(), INVALID_MEMORY_ACCESS, ILLEGAL_ADDRESS_WARP);
#endif
		FATAL_ERROR_UNLESS(SP.belowMax(), INVALID_MEMORY_ACCESS, ILLEGAL_ADDRESS_WARP);

		SP+=2;
		return *(uint16_t*)&(ramSt[SP.minus(1)]);
	}

	static void reset()
	{
		// move stack pointer to the top of the stack
		SP.selfSetMax();

		// flush stack
		memset(ramSt, 0, sizeof(ramSt));
	}
}

namespace bitshift
{
	template <class T,int bits>
	static inline void ASL(bit_field<T,bits>& operand) {
		// Arithmetic Shift Left
		P.change<F_CARRY>(MSB(operand));
		operand.selfShl1();
		status::setNZ(operand);
	}

	template <class T,int bits>
	static inline void LSR(bit_field<T,bits>& operand) {
		// Logical Shift Right
		P.change<F_CARRY>(LSB(operand));
		operand.selfShr1();
		status::setZ(operand);
		P.clear(F_SIGN);
	}

	template <class T,int bits>
	static inline void ROL(bit_field<T,bits>& operand) {
		// Rotate Left With Carry
		const bool newCarry=MSB(operand);
		operand.selfRcl(P[F_CARRY]);
		P.change<F_CARRY>(newCarry);
		status::setNZ(operand);
	}

	template <class T,int bits>
	static inline void ROR(bit_field<T,bits>& operand) {
		// Rotate Right With Carry
		const bool newCarry=LSB(operand);
		operand.selfRcr(P[F_CARRY]);
		P.change<F_CARRY>(newCarry);
		status::setNZ(operand);
	}
}

namespace status
{
	template <class T,int bits>
	static inline void setZ(const bit_field<T,bits>& result)
	{
		P.change<F_ZERO>(result.zero());
	}

	template <class T,int bits>
	static inline void setN(const bit_field<T,bits>& result)
	{
		P.change<F_NEGATIVE>(result.negative());
	}

	template <class T,int bits>
	static inline void setNZ(const bit_field<T,bits>& result)
	{
		P.change<F_NEGATIVE>(result.negative());
		P.change<F_ZERO>(result.zero());
	}

	template <class T,int bits>
	static inline void setNV(const bit_field<T,bits>& result)
	{
		STATIC_ASSERT((int)F_NEGATIVE == 1<<7 && (int)F_OVERFLOW == 1<<6);
		P.copy<F__NV, 6, 2>(result);
	}
}

namespace cpu
{
	void reset()
	{
		// reset general purpose registers
		A=0;
		X=0;
		Y=0;

		// reset status register
		P.clearAll();
		P.set(F_RESERVED);
		P.set(F_ZERO);

		// reset stack pointer
		stack::reset();

		// clear IRQ state
		interrupt::clearAll();
		// set PC to the entry point
		interrupt::request(IRQ::RST);
	}

	void start()
	{
	}

	static int readEffectiveAddress(const opcode_t code, const M6502_OPCODE op)
	{
		int cycles=0;
		
		invalidate(EA);
		invalidate(M);

		switch (op.addrmode)
		{
		case ADR_IMP: // Ignore. Address is implied in instruction.
			break;

		case ADR_ZP: // Zero Page mode. Use the address given after the opcode, but without high byte.
			addr=mmc::fetchByteOperand(PC);
			value=mmc::read(addr);
			break;

		case ADR_REL: // Relative mode.
			addr=(char)valueOf(mmc::fetchByteOperand(PC));
			addr+=PC;
			break;

		case ADR_ABS: // Absolute mode. Use the two bytes following the opcode as an address.
			addr=mmc::fetchWordOperand(PC);
			value=mmc::read(addr);
			break;

		case ADR_IMM: //Immediate mode. The value is given after the opcode.
			addr=PC;
			value=mmc::fetchByteOperand(PC);
			break;

		case ADR_ZPX:
			// Zero Page Indexed mode, X as index. Use the address given
			// after the opcode, then add the
			// X register to it to get the final address.
			addr=mmc::fetchByteOperand(PC).plus(X);
			value=mmc::read(addr);
			break;

		case ADR_ZPY:
			// Zero Page Indexed mode, Y as index. Use the address given
			// after the opcode, then add the
			// Y register to it to get the final address.
			addr=mmc::fetchByteOperand(PC).plus(Y);
			value=mmc::read(addr);
			break;

		case ADR_ABSX:
			// Absolute Indexed Mode, X as index. Same as zero page
			// indexed, but with the high byte.
			addr=mmc::fetchWordOperand(PC);
			if ((valueOf(addr)&0xFF00)!=((valueOf(addr)+X)&0xFF00)) ++cycles;
			addr+=X;
			value=mmc::read(addr);
			break;

		case ADR_ABSY:
			// Absolute Indexed Mode, Y as index. Same as zero page
			// indexed, but with the high byte.
			addr=mmc::fetchWordOperand(PC);
			if ((valueOf(addr)&0xFF00)!=((valueOf(addr)+Y)&0xFF00)) ++cycles;
			addr+=Y;
			value=mmc::read(addr);
			break;

		case ADR_INDX:
			addr=mmc::fetchByteOperand(PC).plus(X);
			addr=mmc::loadZPWord(maddr8_t(addr));
			value=mmc::read(addr);
			break;

		case ADR_INDY:
			addr=mmc::loadZPWord(mmc::fetchByteOperand(PC));
			if ((valueOf(addr)&0xFF00)!=((valueOf(addr)+Y)&0xFF00)) ++cycles;
			addr+=Y;
			value=mmc::read(addr);
			break;

		case ADR_IND:
			// Indirect Absolute mode. Find the 16-bit address contained
			// at the given location.
			addr=mmc::fetchWordOperand(PC);
			addr=makeWord(mmc::read(addr), mmc::read(maddr_t(((valueOf(addr)+1)&0x00FF)|(valueOf(addr)&0xFF00))));
			value=mmc::read(addr);
			break;

		default:
			FATAL_ERROR(INVALID_INSTRUCTION, INVALID_ADDRESS_MODE, "opcode", code, "instruction", op.inst, "adrmode", op.addrmode);
			break;
		}

		return cycles;
	}

	int nextInstruction()
	{
		int cycles=0;

		// poll interrupts
		if (interrupt::pending())
		{
			IRQ irq = interrupt::current();
			if (irq != IRQ::BRK || !P[F_INTERRUPT_OFF])
			{
				// jump to interrupt handler
				PC = interrupt::handler(irq);
				interrupt::clear(irq);
			}
		}

		// step1: fetch instruction
		const maddr_t opaddr = PC;
		const opcode_t opcode = mmc::fetchOpcode(PC);

		// step2: decode
		const M6502_OPCODE op = opcode::decode(opcode);
		ERROR_UNLESS(opcode::usual(opcode), INVALID_INSTRUCTION, INVALID_OPCODE, "opcode", opcode, "instruction", op.inst);

		// step3: read effective address & operands
		cycles += readEffectiveAddress(opcode, op);
		assert((valueOf(PC)-valueOf(opaddr)) == op.size);

		debug::printDisassembly(opaddr, opcode, X, Y, EA, M);

		// step4: execution
		_alutemp_t temp;

		assert(P[F_RESERVED]);

		// update statistics
		STAT_ADD(totInstructions, 1);
		STAT_ADD(numInstructionsPerOpcode[(int)op.inst], 1);
		STAT_ADD(numInstructionsPerAdrMode[(int)op.addrmode], 1);
		STAT_ADD(totCycles, cycles);
		return cycles;
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