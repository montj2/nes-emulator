#include "macro.h"
#include "datatype.h"
#include "optable.h"
#include "nes.h"
#include "mmc.h"
#include "ppu.h"
#include "cpu.h"

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
    F_SIGN=F_NEGATIVE
};

const CONST_BIT<_addr16_t,16,0xFFFA> VECTOR_NMI;
const CONST_BIT<_addr16_t,16,0xFFFC> VECTOR_RESET;
const CONST_BIT<_addr16_t,16,0xFFFE> VECTOR_IRQ;

// ++ 6502 Registers ++
_reg8_t             A;   // accumulator
_reg8_t             X,Y; // index
addr8_t             SP;  // stack pointer
FLAG<_reg8_t,PSW,8> P;   // status
maddr_t             PC;  // program counter

// wrappers
#define regA FASTCAST(A,reg_bit_t)
#define regX FASTCAST(X,reg_bit_t)
#define regY FASTCAST(Y,reg_bit_t)

// -- 6502 Registers --

/* Sync */
unsigned long cycles;
const unsigned long maxCycles=114;

/* Statistics */
long long insCount;
long long opCount[(int)M6502_INST::_INS_MAX];
long long adrCount[(int)M6502_ADDRMODE::_ADR_MAX];

/* IRQ */
bool irqRequested;
enum IRQ_TYPE irqType;

bool DebugMode=false;
#define asmprintf if (DebugMode) printf
//#define asmprintf(...) (void)0

#define PUSH_8(byte) do { \
    printf("* PushReg %02X to %02X *\n",byte,valueOf(SP)); \
    stack[valueOf(SP)]=(byte); \
    dec(SP); \
    assert(SP.isNotMax()); \
    } while(0)

#define PUSH_16(word) do { \
    dec(SP); \
    printf("* PushWord %04X to %02X *\n",word,valueOf(SP)); \
    *(uint16_t*)&(stack[valueOf(SP)])=(word); \
    dec(SP); \
    assert(SP.isNotMax()); \
    } while(0)

#define PUSH_REG(byte)  do {PUSH_8(byte);        } while(0)
#define PUSH_PC()       do {PUSH_16(valueOf(PC));} while(0)

static inline byte_t pop() {
    assert(SP.isNotMax());
    return stack[inc(SP)];
}

static inline word_t pop16bit()
{
	assert(SP.isNotMax() && SP.plus(1).isNotMax());
	SP.add(2);
	return *(uint16_t*)&(stack[SP.minus(1)]);
}

#define SET_Z(result)   do {P.set(F_ZERO,(valueOf(result)==0));     } while(0)
#define SET_N(result)   do {P.set(F_NEGATIVE,result.isNegative());  } while(0)
#define SET_V(val)      do {P.set(F_OVERFLOW,val);                  } while(0)
#define SET_NZ(result)  do {SET_N(result);SET_Z(result);            } while(0)

template <class T,int bits>
static inline void copyNV(const BIT<T,bits>& operand)
{
    P.set(F_NEGATIVE,operand[7]);
    P.set(F_OVERFLOW,operand[6]);
}

template <class T,int bits>
static inline void ASL(BIT<T,bits>& operand) {
    // Arithmetic Shift Left
    P.set(F_CARRY,operand.MSB());
    operand.bitShl(1);
    SET_NZ(operand);
}

template <class T,int bits>
static inline void LSR(BIT<T,bits>& operand) {
    // Logical Shift Right
    P.set(F_CARRY,operand.LSB());
    operand.bitShr(1);
    SET_Z(operand);
    P.clear(F_SIGN);
}

template <class T,int bits>
static inline void ROL(BIT<T,bits>& operand) {
    // Rotate Left With Carry
    const bool newCarry=operand.MSB();
    operand.bitRcl(P[F_CARRY]);
    P.set(F_CARRY,newCarry);
    SET_NZ(operand);
}

template <class T,int bits>
static inline void ROR(BIT<T,bits>& operand) {
    // Rotate Right With Carry
    const bool newCarry=operand.LSB();
    operand.bitRcr(P[F_CARRY]);
    P.set(F_CARRY,newCarry);
    SET_NZ(operand);
}

static void doNMI() {
    PUSH_PC();
    PUSH_REG(ValueOf(P));
    PC=loadOperand16bit(VECTOR_NMI); // NMI
}

int cpu_exec() // Emulates a single CPU instruction
{
    // IRQ processing
    if (irqRequested)
    {
        switch (irqType)
        {
            case IRQ_NMI:
                doNMI();
                break;
            default:
                assert(0);
                break;
        }
        irqRequested=false;
        irqType=IRQ_NONE;
    }

	// ------------------------------------------------------
	// Fetch next instruction:
	// ------------------------------------------------------
	const maddr_t               opaddr  =   PC;
	const opcode_t              opcode  =   readCode(PC);
	const struct M6502_OPCODE   opinf   =   ParseOp(opcode);
	maddr_t                     addr;
	int                         cycleAdd=   0;

	inc(PC);
	#ifndef NDEBUG
        ++insCount;
        ++opCount[(int)opinf.inst];
        ++adrCount[(int)opinf.addrmode];
	#endif
	//fprintf(log,"%X <%X> #%I64d\n",opcode,opaddr,opCount);

    //if (valueOf(opaddr)==0xC009 || valueOf(opaddr)==0x0C00C) DebugMode=false;else DebugMode=true;
    asmprintf("[CPU] CIA = %04X %02X\t%s",valueOf(opaddr),opcode,GetInstName(opinf.inst));
    assert(IsUsualOp(opcode));

	switch (opinf.addrmode)
	{
		case M6502_ADDRMODE::ADR_IMP: // Ignore. Address is implied in instruction.
			break;

		case M6502_ADDRMODE::ADR_ZP: // Zero Page mode. Use the address given after the opcode, but without high byte.
			asmprintf(" $%02X",loadOperand(PC));
			addr=loadOperand(PC);
			inc(PC);
			break;

		case M6502_ADDRMODE::ADR_REL: // Relative mode.
			//asmprintf(" +/-%02X",loadOperand(PC));
			addr=loadOperand(PC);
			inc(PC);
			if (valueOf(addr)&0x80)
				addr.add(valueOf(PC)-256);
			else
				addr.add(valueOf(PC));
			asmprintf(" to %04X",valueOf(addr));
		    break;

		case M6502_ADDRMODE::ADR_ABS: // Absolute mode. Use the two bytes following the opcode as an address.
			asmprintf(" $%04X",loadOperand16bit(PC));
			addr=loadOperand16bit(PC);
			PC.add(2);
			break;

		case M6502_ADDRMODE::ADR_IMM: //Immediate mode. The value is given after the opcode.
			asmprintf(" #$%02X",loadOperand(PC));
			addr=PC;
			inc(PC);
			break;

		case M6502_ADDRMODE::ADR_ZPX:
			// Zero Page Indexed mode, X as index. Use the address given
			// after the opcode, then add the
			// X register to it to get the final address.
			asmprintf(" $%02X,X=%02X",loadOperand(PC),(uint8_t)X);
			addr=loadOperand(PC);
			addr.add(X);
			inc(PC);
			break;

		case M6502_ADDRMODE::ADR_ZPY:
			// Zero Page Indexed mode, Y as index. Use the address given
			// after the opcode, then add the
			// Y register to it to get the final address.
			asmprintf(" $%02X,Y=%02X",loadOperand(PC),(uint8_t)Y);
			addr=loadOperand(PC);
			addr.add(Y);addr.bitAndEqual(0xFF);
			inc(PC);
			break;

		case M6502_ADDRMODE::ADR_ABSX:
			// Absolute Indexed Mode, X as index. Same as zero page
			// indexed, but with the high byte.
			asmprintf(" $%04X,X=%02X",loadOperand16bit(PC),(uint8_t)X);
			addr=loadOperand16bit(PC);PC.add(2);
			if ((valueOf(addr)&0xFF00)!=((valueOf(addr)+X)&0xFF00) && opinf.cycles==4) cycleAdd=1;
			addr.add(X);
			break;

		case M6502_ADDRMODE::ADR_ABSY:
			// Absolute Indexed Mode, Y as index. Same as zero page
			// indexed, but with the high byte.
			asmprintf(" $%04X,Y=%02X",loadOperand16bit(PC),(uint8_t)Y);
			addr=loadOperand16bit(PC);PC.add(2);
			if ((valueOf(addr)&0xFF00)!=((valueOf(addr)+Y)&0xFF00) && opinf.cycles==4) cycleAdd=1;
			addr.add(Y);
			break;

		case M6502_ADDRMODE::ADR_INDX:
			asmprintf(" ($%02X,X=%02X)",loadOperand(PC),(uint8_t)X);
			//addr=loadZp16bit((loadOperand(PC++)+X)&0xFF);
			addr=loadOperand(PC);
			addr.add(X);
			addr.bitAndEqual(0xFF);
			addr=loadZp16bit(addr8_t::wrapper(addr));
			inc(PC);
			break;

		case M6502_ADDRMODE::ADR_INDY:
			asmprintf(" ($%02X),Y=%02X",loadOperand(PC),(uint8_t)Y);
			addr=loadZp16bit(addr8_t::wrapper(loadOperand(PC)));
			if ((valueOf(addr)&0xFF00)!=((valueOf(addr)+Y)&0xFF00) && opinf.cycles==5) cycleAdd=1;
			addr.add(Y);
			inc(PC);
			break;

		case M6502_ADDRMODE::ADR_IND:
			// Indirect Absolute mode. Find the 16-bit address contained
			// at the given location.
			asmprintf(" ($%04X)",loadOperand16bit(PC));
			addr=loadOperand16bit(PC);
			addr=makeWord(read6502(addr),read6502(maddr_t::wrapper(((valueOf(addr)+1)&0x00FF)|(valueOf(addr)&0xFF00))));
			PC.add(2);
			break;

		default:
			asmprintf(" UNHANDLED ADDRESSING MODE");
			break;
	}
	if (opinf.addrmode!=M6502_ADDRMODE::ADR_IMP)
    {
        asmprintf("\t// %s\n",ExplainAddrMode(opinf.addrmode));
        addr.checkAssert();
    }
    else
    {
        asmprintf("\n"); // move to new line
    }

    // check instruction size
    assert(valueOf(PC)-valueOf(opaddr)==opinf.size);

	// ------------------------------------------------------
	// Decode & execution instruction:
	// ------------------------------------------------------
	byte_t          value;
	_alutemp_t      temp;
	#define M       FASTCAST(value,value_t)
	#define SUM     FASTCAST(temp,alu_t)

	switch (opinf.inst)
	{
		case M6502_INST::INS_ADC: // Add with carry.

		    if (!P[F_BCD]) // binary add
            {
                temp=A;
                value=read6502(addr);
                temp+=value;
                temp+=P[F_CARRY]?1:0;

                P.set(F_OVERFLOW,!((A^value)&0x80) && ((A^temp)&0x80));
                P.set(F_CARRY,SUM.isOverflow());
            }else
            {
                temp=A;
                value=read6502(addr);
                // check BCD
                assert((temp&0xF)<=9 && (temp>>4)<=9);
                assert((value&0xF)<=9 && (value>>4)<=9);
                // add CF
                temp+=P[F_CARRY]?1:0;

                // add low digit
                if ((temp&0xF)+(value&0xF)>9)
                {
                    temp+=(value&15)+6;
                    P.set(F_CARRY,true);
                }else
                {
                    temp+=(value&15);
                    P.set(F_CARRY,false);
                }
                if ((temp>>4)+(value>>4)>9)
                {
                    temp+=(value&0xF0)+6;
                    P.set(F_OVERFLOW,true);
                    P.set(F_CARRY,true);
                }else
                {
                    temp+=(value&0xF0);
                    P.set(F_OVERFLOW,false);
                }
            }

			regA.bitCopyAndWrap(temp);
			SET_NZ(regA);
			break;

		case M6502_INST::INS_AND: // AND memory with accumulator.
			value=read6502(addr);
			regA.bitAndEqual(value);
			SET_NZ(regA);
			break;

		case M6502_INST::INS_ASLA: // Shift left one bit
		    ASL(regA);
			break;

		case M6502_INST::INS_ASL:
			value=read6502(addr);
			ASL(M);
			write6502(addr,value);
			break;

		case M6502_INST::INS_BCC: // Branch on carry clear
			if (!P[F_CARRY])
			{
			jBranch:
				cycleAdd+=((valueOf(opaddr)^valueOf(addr))&0xFF00)?2:1;
				PC=addr;
			}
			break;
		case M6502_INST::INS_BCS: // Branch on carry set
			if (P[F_CARRY]) goto jBranch;else break;
		case M6502_INST::INS_BEQ: // Branch on zero
			if (P[F_ZERO]) goto jBranch;else break;
		case M6502_INST::INS_BMI: // Branch on negative result
			if (P[F_SIGN]) goto jBranch;else break;
		case M6502_INST::INS_BNE: // Branch on not zero
			if (!P[F_ZERO]) goto jBranch;else break;
		case M6502_INST::INS_BPL: // Branch on positive result
			if (!P[F_SIGN]) goto jBranch;else break;
		case M6502_INST::INS_BVC: // Branch on overflow clear
			if (!P[F_OVERFLOW]) goto jBranch;else break;
		case M6502_INST::INS_BVS: // Branch on overflow set
			if (P[F_OVERFLOW]) goto jBranch;else break;

		case M6502_INST::INS_BRK: // Break
			inc(PC);
			PUSH_PC();
			P.set(F_BREAK);
			PUSH_REG(ValueOf(P));
			P.set(F_INTERRUPT_OFF);
			PC=loadOperand16bit(VECTOR_IRQ); // IRQ/BRK
			break;

		case M6502_INST::INS_BIT:
			value=read6502(addr);
			copyNV(M);
			M.bitAndEqual(A);
			SET_Z(M);
			break;

		case M6502_INST::INS_CLC: // Clear carry flag
			P.clear(F_CARRY);
			break;
		case M6502_INST::INS_CLD: // Clear decimal flag
		    P.clear(F_DECIMAL);
			break;
        case M6502_INST::INS_CLI: // Clear interrupt flag
            P.clear(F_INTERRUPT_OFF);
            break;
		case M6502_INST::INS_CLV: // Clear overflow flag
		    P.clear(F_OVERFLOW);
			break;

		case M6502_INST::INS_CMP: // Compare memory and accumulator
		case M6502_INST::INS_CPX: // Compare memory and index X
		case M6502_INST::INS_CPY: // Compare memory and index Y
			switch (opinf.inst)
			{
				case M6502_INST::INS_CMP:
					temp=A;break;
				case M6502_INST::INS_CPX:
					temp=X;break;
				case M6502_INST::INS_CPY:
					temp=Y;break;
                default:
                    break;
			}
			value=read6502(addr);
			temp=temp+0x100-value;
			// if (temp>0xFF) [R]-[M]>=0 C=1;
			P.set(F_CARRY,SUM.isOverflow());
			temp=(temp-0x100)&0xFF;
			SET_NZ(SUM);
			break;

		case M6502_INST::INS_DEC: // Decrement memory by one
			value=read6502(addr);
			dec(M);
			write6502(addr,value);
			SET_NZ(M);
			break;

		case M6502_INST::INS_DEX: // Decrement index X by one
		    dec(regX);
			SET_NZ(regX);
			break;

		case M6502_INST::INS_DEY: // Decrement index Y by one
			dec(regY);
			SET_NZ(regY);
			break;

		case M6502_INST::INS_EOR: // XOR Memory with accumulator, store in accumulator
			value=read6502(addr);
			regA.bitXorEqual(value);
			SET_NZ(regA);
			break;

		case M6502_INST::INS_INC: // Increment memory by one
			value=read6502(addr);
			inc(M);
			write6502(addr,value);
			SET_NZ(M);
			break;

		case M6502_INST::INS_INX: // Increment index X by one
			inc(regX);SET_NZ(regX);
			break;

		case M6502_INST::INS_INY: // Increment index Y by one
			inc(regY);SET_NZ(regY);
			break;

		case M6502_INST::INS_JMP: // Jump to new location
			PC=addr;
			break;

		case M6502_INST::INS_JSR: // Jump to new location, saving return address. Push return address on stack
			dec(PC);
			PUSH_PC();
			PC=addr;
			break;

		case M6502_INST::INS_LDA: // Load accumulator with memory
		case M6502_INST::INS_LDX: // Load index X with memory
		case M6502_INST::INS_LDY: // Load index Y with memory
			value=read6502(addr);
			SET_NZ(M);
			switch (opinf.inst)
			{
				case M6502_INST::INS_LDA:
					A=value;break;
				case M6502_INST::INS_LDX:
					X=value;break;
				case M6502_INST::INS_LDY:
					Y=value;break;
                default:
                    break;
			}
			break;

		case M6502_INST::INS_LSR: // Shift right one bit
			value=read6502(addr);
			LSR(M);
			write6502(addr,value);
			break;

		case M6502_INST::INS_LSRA:
		    LSR(regA);
			break;

		case M6502_INST::INS_NOP: break; // No OPeration
		case M6502_INST::INS_ORA: // OR memory with accumulator, store in accumulator.
			value=read6502(addr);
			regA.bitOrEqual(value);
			SET_NZ(regA);
			break;

		case M6502_INST::INS_PHA: // Push accumulator on stack
			PUSH_REG(A);
			break;

		case M6502_INST::INS_PHP: // Push processor status on stack
			PUSH_REG(ValueOf(P));
			break;

		case M6502_INST::INS_PLA: // Pull accumulator from stack
			A=pop();
			SET_NZ(regA);
			break;

		case M6502_INST::INS_PLP: // Pull processor status from stack
			P.bitCopy(pop());
			break;

		case M6502_INST::INS_ROL: // Rotate one bit left
			value=read6502(addr);
			ROL(M);
			write6502(addr,value);
			break;

		case M6502_INST::INS_ROLA:
		    ROL(regA);
			break;

		case M6502_INST::INS_ROR: // Rotate one bit right
		    value=read6502(addr);
			ROR(M);
			write6502(addr,value);
			break;

		case M6502_INST::INS_RORA:
			ROR(regA);
			break;

		case M6502_INST::INS_RTI: // Return from interrupt. Pull status and PC from stack.
			P.bitCopy(pop());
			PC=pop16bit();
			break;

		case M6502_INST::INS_RTS: // Return from subroutine. Pull PC from stack.
			PC=pop16bit();
			inc(PC);
			break;

		case M6502_INST::INS_SBC: // Subtract
			assert(!P[F_BCD]);

			value=read6502(addr);
			temp=A;
			temp-=value;
			temp-=P[F_CARRY]?0:1;

			P.set(F_OVERFLOW,!((A^value)&0x80) && ((A^temp)&0x80));
			P.set(F_CARRY,!(SUM.isOverflow()));

			regA.bitCopyAndWrap(temp);
			SET_NZ(regA);
			break;

		case M6502_INST::INS_SEC: // Set carry flag
		    P.set(F_CARRY);
			break;
		case M6502_INST::INS_SED: // Set decimal flag
			P.set(F_DECIMAL);
			break;
		case M6502_INST::INS_SEI: // Set interrupt disable status
		    P.set(F_INTERRUPT_OFF);
			break;

		case M6502_INST::INS_STA: // Store accumulator in memccpy
			write6502(addr,A);
			break;
		case M6502_INST::INS_STX: // Store index X in memory
			write6502(addr,X);
			break;
		case M6502_INST::INS_STY: // Store index Y in memory
			write6502(addr,Y);
			break;
		case M6502_INST::INS_TAX: // Transfer accumulator to index X
			X=A;
			SET_NZ(regX);
			break;
		case M6502_INST::INS_TAY: // Transfer accumulator to index Y
			Y=A;
			SET_NZ(regY);
			break;
		case M6502_INST::INS_TSX: // Transfer stack pointer to index X
			X=valueOf(SP);
			SET_NZ(regX);
			break;
		case M6502_INST::INS_TXA: // Transfer index X to accumulator
			A=X;
			SET_NZ(regA);
			break;
		case M6502_INST::INS_TXS: // Transfer index X to stack pointer
			SP=X;
			break;
		case M6502_INST::INS_TYA: // Transfer index Y to accumulator
			A=Y;
			SET_NZ(regA);
			break;
		default:
			printf("[CPU] Game crashed, invalid opcode at address $%04X\n",valueOf(opaddr));
			break;
	}

	asmprintf("NIA =  [%04X] A=%X, X=%X, Y=%X, P=%X, SP=%X\n",valueOf(PC),(uint8_t)A,(uint8_t)X,(uint8_t)Y,ValueOf(P),valueOf(SP));
	asmprintf("\n");
	regA.checkAssert();
	regX.checkAssert();
	regY.checkAssert();
	return cycleAdd+opinf.cycles;
}

void cpu_reset()
{
    A=0;
    X=0;
    Y=0;
    SP.bitSetMax();
    P.clear();
    P.set(F_NOTUSED);

    cycles=0;

    irqRequested=false;
    irqType=IRQ_NONE;

    PC=loadOperand16bit(VECTOR_RESET); // RESET
    printf("[CPU] PC reset to $%04X\n",valueOf(PC));

	insCount=0;
	memset(opCount,0,sizeof(opCount));
	memset(adrCount,0,sizeof(adrCount));
}

int cpu_frame() {
    int isRunning=1;
    while (isRunning)
    {
        while (cycles>maxCycles)
        {
            cycles-=maxCycles;
            if (ppu_endScanline())
                break; // frame ends
        }
        cycles+=cpu_exec();
    }
    return 1;
}

void cpu_requestIRQ(enum IRQ_TYPE type)
{
    // assert $2000 D7
    assert(type!=IRQ_NONE);
    irqRequested=true;
    irqType=type;
}

void testCPU() {
    P.clear();

    A=0;
    SET_Z(regA);
    assert(P[F_ZERO]);

    X=0xFF;
    SET_NZ(regX);
    assert(!P[F_ZERO] && P[F_NEGATIVE]);

    byte_t value;
    _alutemp_t temp;

    value=0x10;
    temp=value<<4;
    P.set(F_CARRY,SUM.isOverflow());
    assert(P[F_CARRY]);

    M=F_NEGATIVE;
    copyNV(M);
    assert(P[F_NEGATIVE] && !P[F_OVERFLOW]);

    temp=0x100;
    copyNV(SUM);
    assert(!P[F_NEGATIVE] && !P[F_OVERFLOW]);

    SET_V(true);
    assert(P[F_OVERFLOW]);

    Y=0x80;
    ASL(regY);
    assert(Y==0 && P[F_CARRY] && P[F_ZERO] && !P[F_NEGATIVE]);

    value=0x41;
    ASL(M);
    assert(!P[F_CARRY] && !P[F_ZERO] && P[F_NEGATIVE]);

    A=0x80;
    LSR(regA);
    assert(!P[F_CARRY] && !P[F_ZERO] && !P[F_NEGATIVE]);

    value=0x01;
    LSR(M);
    assert(P[F_CARRY] && P[F_ZERO] && !P[F_NEGATIVE]);

    X=0x40;
    ROR(regX);
    assert(X==0xA0);
    assert(!P[F_CARRY] && !P[F_ZERO] && P[F_NEGATIVE]);

    Y=1;
    ROR(regY);
    assert(P[F_ZERO] && P[F_CARRY] && !P[F_NEGATIVE]);

    ROL(regY);
    assert(Y==1 && !P[F_NEGATIVE] && !P[F_CARRY] && !P[F_ZERO]);

    P|=F_CARRY;
    ROL(regY);
    assert(Y==3);

    SP.bitSetMax();
    PUSH_REG(Y);

    PC=0xFFAA;
    PUSH_PC();

    byte_t tmp;
    tmp=pop();assert(tmp==0xAA);
    tmp=pop();assert(tmp==0xFF);
    tmp=pop();assert(tmp==0x03);

    PUSH_PC();
    PUSH_REG(Y);
    word_t tmp16;
    tmp16=pop16bit();assert(tmp16==0xAA03);
    tmp=pop();assert(tmp==0xFF);

    puts("**** testCPU() passed ***");
}
