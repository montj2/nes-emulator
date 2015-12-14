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
static _alutemp_t	temp;

// alias of registers with wrapping
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

		// clear stack
		memset(ramSt, 0, sizeof(ramSt));
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

namespace interrupt
{
	static const maddr_t VECTOR_NMI(0xFFFA);
	static const maddr_t VECTOR_RESET(0xFFFC);
	static const maddr_t VECTOR_IRQ(0xFFFE);

	static flag_set<_reg8_t, IRQTYPE, 8> pendingIRQs;

	static void clearAll()
	{
		pendingIRQs.clearAll();
	}

	static void clear(IRQTYPE type)
	{
		assert(pendingIRQs[type]);
		pendingIRQs.clear(type);
	}

	void request(const IRQTYPE type)
	{
		vassert(type != IRQTYPE::NONE);
		ERROR_IF(pendingIRQs[type], ILLEGAL_OPERATION, IRQ_ALREADY_PENDING);

		pendingIRQs.set(type);
		STAT_ADD(totInterrupts, 1);
	}

	// get address of interrupt handler
	static maddr_t handler(const IRQTYPE type)
	{
		vassert(type != IRQTYPE::NONE);

		maddr_t vector;
		switch (type)
		{
		case IRQTYPE::RST: vector=VECTOR_RESET; break;
		case IRQTYPE::NMI: vector=VECTOR_NMI; break;

		case IRQTYPE::IRQ:
		case IRQTYPE::BRK:
			vector=VECTOR_IRQ;
			break;
		}
		return mmc::fetchWordOperand(vector);
	}

	static bool pending()
	{
		return pendingIRQs.any();
	}

	bool pending(const IRQTYPE type)
	{
		vassert(type != IRQTYPE::NONE);
		return pendingIRQs[type];
	}

	// return the highest-priority IRQ that is currently pending
	static IRQTYPE current()
	{
		if (pendingIRQs[IRQTYPE::RST]) return IRQTYPE::RST;
		if (pendingIRQs[IRQTYPE::NMI]) return IRQTYPE::NMI;
		if (pendingIRQs[IRQTYPE::IRQ]) return IRQTYPE::IRQ;
		if (pendingIRQs[IRQTYPE::BRK]) return IRQTYPE::BRK;
		return IRQTYPE::NONE;
	}

	static void poll()
	{
		if (pending())
		{
			IRQTYPE irq = current();
			if (irq != IRQTYPE::IRQ || !P[F_INTERRUPT_OFF])
			{
				// process IRQ
				stack::pushPC();
				if (irq != IRQTYPE::RST)
				{
					// set or clear Break flag depending on irq type
					if (irq == IRQTYPE::BRK)
						P|=F_BREAK;
					else
						P-=F_BREAK;
					// push status
					stack::pushReg(P);
					// disable other interrupts
					P|=F_INTERRUPT_OFF;
				}
				// jump to interrupt handler
				PC = handler(irq);
				clear(irq);
			}
		}
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

namespace arithmetic
{
	static void ADC()
	{
		// Add with carry. A <- [A]+[M]+C
		if (!P[F_BCD])
		{
			// binary addition
			temp=A+value+(P[F_CARRY]?1:0);
			P.change<F_OVERFLOW>(!((A^value)&0x80) && ((A^temp)&0x80));
			P.change<F_CARRY>(SUM.overflow());
		}else
		{
			// bcd addition
			assert((A&0xF)<=9 && (A>>4)<=9);
			assert((value&0xF)<=9 && (value>>4)<=9);
			// add CF
			temp=A+(P[F_CARRY]?1:0);

			// add low digit
			if ((temp&0xF)+(value&0xF)>9)
			{
				temp+=(value&0xF)+6;
				P|=F_CARRY;
			}else
			{
				temp+=(value&0xF);
				P-=F_CARRY;
			}
			if ((temp>>4)+(value>>4)>9)
			{
				temp+=(value&0xF0)+6;
				P|=F_OVERFLOW;
				P|=F_CARRY;
			}else
			{
				temp+=(value&0xF0);
				P-=F_OVERFLOW;
			}
		}
		status::setNZ(regA(temp));
	}

	static void SBC()
	{
		// can't be used to subtract a bcd number
		assert(!P[F_BCD]);

		temp=A-value-(P[F_CARRY]?0:1);

		P.change<F_CARRY>(!SUM.overflow());
		P.change<F_OVERFLOW>(((A^value)&0x80) && ((A^temp)&0x80));

		status::setNZ(regA(temp));
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
		PC=0;

		// reset status register
		P.clearAll();
		P.set(F_RESERVED);
		P.set(F_INTERRUPT_OFF);

		// reset stack pointer
		stack::reset();

		// reset IRQ state
		interrupt::clearAll();
		// this will set PC to the entry point
		interrupt::request(IRQTYPE::RST);
	}

	// emulate n instructions
	void start(int n)
	{
		while (n<0 || n--)
		{
			if (nextInstruction()<0) break;
		}
	}

	static int readEffectiveAddress(const opcode_t code, const M6502_OPCODE op)
	{
		int cycles=0;
		
		invalidate(EA);
		invalidate(M);

		maddr8_t addr8;

		switch (op.addrmode)
		{
		case ADR_IMP: // Ignore. Address is implied in instruction.
			break;

		case ADR_ZP: // Zero Page mode. Use the address given after the opcode, but without high byte.
			addr8=mmc::fetchByteOperand(PC);
			addr=addr8;
			value=mmc::loadZPByte(addr8);
			break;

		case ADR_REL: // Relative mode.
			addr=PC+(char)valueOf(mmc::fetchByteOperand(PC));
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
			addr8=mmc::fetchByteOperand(PC).plus(X);
			addr=addr8;
			value=mmc::loadZPByte(addr8);
			break;

		case ADR_ZPY:
			// Zero Page Indexed mode, Y as index. Use the address given
			// after the opcode, then add the
			// Y register to it to get the final address.
			addr8=mmc::fetchByteOperand(PC).plus(Y);
			addr=addr8;
			value=mmc::loadZPByte(addr8);
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
			addr8=mmc::fetchByteOperand(PC).plus(X);
			addr=mmc::loadZPWord(addr8);
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

	static bool execute(const M6502_OPCODE op, bool& writeBack, int& cycles)
	{
		switch (op.inst)
		{
		// arithmetic
		case INS_ADC: // Add with carry.
			arithmetic::ADC();
			break;

		case INS_SBC: // Subtract
			arithmetic::SBC();
			break;
			
		case INS_INC: // Increment memory by one
			inc(M);
			status::setNZ(M);
			writeBack = true;
			break;

		case INS_DEC: // Decrement memory by one
			dec(M);
			status::setNZ(M);
			writeBack = true;
			break;

		case INS_DEX: // Decrement index X by one
			dec(regX);
			status::setNZ(regX);
			break;

		case INS_DEY: // Decrement index Y by one
			dec(regY);
			status::setNZ(regY);
			break;

		case INS_INX: // Increment index X by one
			inc(regX);
			status::setNZ(regX);
			break;

		case INS_INY: // Increment index Y by one
			inc(regY);
			status::setNZ(regY);
			break;

		// bit manipulation
		case INS_AND: // AND memory with accumulator.
			status::setNZ(regA&=M);
			break;

		case INS_ASLA: // Shift left one bit
			bitshift::ASL(regA);
			break;

		case INS_ASL:
			bitshift::ASL(M);
			writeBack = true;
			break;

		case INS_EOR: // XOR Memory with accumulator, store in accumulator
			status::setNZ(regA^=M);
			break;

		case INS_LSR: // Shift right one bit
			bitshift::LSR(M);
			writeBack = true;
			break;

		case INS_LSRA:
			bitshift::LSR(regA);
			break;

		case INS_ORA: // OR memory with accumulator, store in accumulator.
			status::setNZ(regA|=M);
			break;

		case INS_ROL: // Rotate one bit left
			bitshift::ROL(M);
			writeBack = true;
			break;

		case INS_ROLA:
			bitshift::ROL(regA);
			break;

		case INS_ROR: // Rotate one bit right
			bitshift::ROR(M);
			writeBack = true;
			break;

		case INS_RORA:
			bitshift::ROR(regA);
			break;

		// branch
		case INS_JMP: // Jump to new location
			PC=addr;
			break;

		case INS_JSR: // Jump to new location, saving return address. Push return address on stack
			dec(PC);
			stack::pushPC();
			PC=addr;
			break;
		
		case INS_RTS: // Return from subroutine. Pull PC from stack.
			PC=stack::popWord();
			inc(PC);
			break;

		case INS_BCC: // Branch on carry clear
			if (!P[F_CARRY])
			{
jBranch:
				cycles+=((valueOf(PC)^valueOf(addr))&0xFF00)?2:1;
				PC=addr;
			}
			break;

		case INS_BCS: // Branch on carry set
			if (P[F_CARRY]) goto jBranch;else break;
		case INS_BEQ: // Branch on zero
			if (P[F_ZERO]) goto jBranch;else break;
		case INS_BMI: // Branch on negative result
			if (P[F_SIGN]) goto jBranch;else break;
		case INS_BNE: // Branch on not zero
			if (!P[F_ZERO]) goto jBranch;else break;
		case INS_BPL: // Branch on positive result
			if (!P[F_SIGN]) goto jBranch;else break;
		case INS_BVC: // Branch on overflow clear
			if (!P[F_OVERFLOW]) goto jBranch;else break;
		case INS_BVS: // Branch on overflow set
			if (P[F_OVERFLOW]) goto jBranch;else break;

		// interrupt
		case INS_BRK: // Break
			inc(PC);
			interrupt::request(IRQTYPE::BRK);
			break;

		case INS_RTI: // Return from interrupt. Pull status and PC from stack.
			P.asBitField()=stack::popByte();
			P|=F_RESERVED;
			PC=stack::popWord();
			break;

		// set/clear flag
		case INS_CLC: // Clear carry flag
			P.clear(F_CARRY);
			break;
		case INS_CLD: // Clear decimal flag
			P.clear(F_DECIMAL);
			break;
		case INS_CLI: // Clear interrupt flag
			P.clear(F_INTERRUPT_OFF);
			break;
		case INS_CLV: // Clear overflow flag
			P.clear(F_OVERFLOW);
			break;
		case INS_SEC: // Set carry flag
			P.set(F_CARRY);
			break;
		case INS_SED: // Set decimal flag
			P.set(F_DECIMAL);
			break;
		case INS_SEI: // Set interrupt disable status
			P.set(F_INTERRUPT_OFF);
			break;

		// compare
		case INS_BIT:
			status::setNV(M);
			status::setZ(M&=A);
			break;

		case INS_CMP: // Compare memory and accumulator
		case INS_CPX: // Compare memory and index X
		case INS_CPY: // Compare memory and index Y
			switch (op.inst)
			{
			case INS_CMP:
				temp=A;break;
			case INS_CPX:
				temp=X;break;
			case INS_CPY:
				temp=Y;break;
			default:
				break;
			}
			temp=temp+0x100-value;
			// if (temp>0xFF) [R]-[M]>=0 C=1;
			P.change<F_CARRY>(SUM.overflow());
			temp=(temp-0x100)&0xFF;
			status::setNZ(SUM);
			break;

		// load/store
		case INS_LDA: // Load accumulator with memory
			status::setNZ(M);
			A=value;
			break;

		case INS_LDX: // Load index X with memory
			status::setNZ(M);
			X=value;
			break;

		case INS_LDY: // Load index Y with memory
			status::setNZ(M);
			Y=value;
			break;

		case INS_STA: // Store accumulator in memory
			value = A;
			writeBack = true;
			break;
		case INS_STX: // Store index X in memory
			value = X;
			writeBack = true;
			break;
		case INS_STY: // Store index Y in memory
			value = Y;
			writeBack = true;
			break;

		// stack
		case INS_PHA: // Push accumulator on stack
			stack::pushReg(regA);
			break;

		case INS_PHP: // Push processor status on stack
			stack::pushReg(P);
			break;

		case INS_PLA: // Pull accumulator from stack
			A=stack::popByte();
			status::setNZ(regA);
			break;

		case INS_PLP: // Pull processor status from stack
			P.asBitField()=stack::popByte();
			P|=F_RESERVED;
			break;
		
		// transfer
		case INS_TAX: // Transfer accumulator to index X
			X=A;
			status::setNZ(regX);
			break;
		case INS_TAY: // Transfer accumulator to index Y
			Y=A;
			status::setNZ(regY);
			break;
		case INS_TSX: // Transfer stack pointer to index X
			X=valueOf(SP);
			status::setNZ(regX);
			break;
		case INS_TXA: // Transfer index X to accumulator
			A=X;
			status::setNZ(regA);
			break;
		case INS_TXS: // Transfer index X to stack pointer
			SP=X;
			break;
		case INS_TYA: // Transfer index Y to accumulator
			A=Y;
			status::setNZ(regA);
			break;

		// other
		case INS_NOP: break; // No OPeration

		// unofficial

		default:
			return false;
		}
		// success
		return true;
	}

	int nextInstruction()
	{
		int cycles=0;

		// handle interrupt request
		interrupt::poll();

		// step1: fetch instruction
		if (PC.zero())
		{
			// program terminates
			assert(SP.reachMax());
			return -1;
		}

		const maddr_t opaddr = PC;
		const opcode_t opcode = mmc::fetchOpcode(PC);

		// step2: decode
		const M6502_OPCODE op = opcode::decode(opcode);
		ERROR_UNLESS(opcode::usual(opcode), INVALID_INSTRUCTION, INVALID_OPCODE, "opcode", opcode, "instruction", op.inst);

		// step3: read effective address & operands
		cycles += readEffectiveAddress(opcode, op);
		assert((valueOf(PC)-valueOf(opaddr)) == op.size);

		debug::printDisassembly(opaddr, opcode, X, Y, EA, M);

		// step4: execute
		bool writeBack = false;
		if (!execute(op, writeBack, cycles))
		{
			// execution failed
			FATAL_ERROR(INVALID_INSTRUCTION, INVALID_OPCODE, "opcode", opcode, "instruction", op.inst);
			return -1;
		}

		// step5: write back (when needed)
		if (writeBack)
		{
			assert(addr != 0xCCCC);
			mmc::write(addr, value);
		}

		cycles+=op.cycles;
		// end of instruction pipeline

		assert(P[F_RESERVED]);
		debug::printCPUState(PC, A, X ,Y, valueOf(P), SP, cycles);

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

		printf("[ ] Register memory from %p to %p\n", &A, &temp+1);

		return SUCCESS;
	}
};

registerTestCase(CPUTest);