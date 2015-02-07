/* 6502 Addressing modes */
enum class M6502_ADDRMODE : uint8_t
{
    ADR_ABS=0,
    ADR_ABSX,
    ADR_ABSY,
    ADR_IMM,
    ADR_IMP,
    ADR_ACC=ADR_IMP,
    ADR_INDABSX,
    ADR_IND, // JMP bug
    ADR_INDX,
    ADR_PREIDXIND=ADR_INDX,
    ADR_INDY,
    ADR_POSTIDXIND=ADR_INDY,
    ADR_INDZP,
    ADR_REL,
    ADR_ZP,
    ADR_ZPX,
    ADR_ZPY,
    _ADR_MAX
};

/* 6502 Instructions */
enum class M6502_INST : uint8_t
{
    INS_ADC=0,
    INS_AND,
    INS_ASL,
    INS_ASLA,
    INS_BCC,
    INS_BCS,
    INS_BEQ,
    INS_BIT,
    INS_BMI,
    INS_BNE,
    INS_BPL,
    INS_BRK,
    INS_BVC,
    INS_BVS,
    INS_CLC,
    INS_CLD,
    INS_CLI,
    INS_CLV,
    INS_CMP,
    INS_CPX,
    INS_CPY,
    INS_DEC,
    INS_DEA,
    INS_DEX,
    INS_DEY,
    INS_EOR,
    INS_INC,
    INS_INX,
    INS_INY,
    INS_JMP,
    INS_JSR,
    INS_LDA,
    INS_LDX,
    INS_LDY,
    INS_LSR,
    INS_LSRA,
    INS_NOP,
    INS_ORA,
    INS_PHA,
    INS_PHP,
    INS_PLA,
    INS_PLP,
    INS_ROL,
    INS_ROLA,
    INS_ROR,
    INS_RORA,
    INS_RTI,
    INS_RTS,
    INS_SBC,
    INS_SEC,
    INS_SED,
    INS_SEI,
    INS_STA,
    INS_STX,
    INS_STY,
    INS_TAX,
    INS_TAY,
    INS_TSX,
    INS_TXA,
    INS_TXS,
    INS_TYA,
    INS_BRA,
    INS_INA,
    INS_PHX,
    INS_PLX,
    INS_PHY,
    INS_PLY,
    _INS_MAX
};

/* 6502 Opcodes */
struct M6502_OPCODE
{
	/* ENUM<uint8_t,M6502_INST,_INS_MAX> inst; */
	M6502_INST inst;
    uint8_t cycles;
    /* ENUM<uint8_t,M6502_ADDRMODE,_ADR_MAX> addrmode; */
    M6502_ADDRMODE addrmode;
    uint8_t size;
};

void cpu_initTable();
void cpu_testTable();

M6502_OPCODE cpuGetOpData(const opcode_t opcode);
const char* cpuGetInstNameByInst(const M6502_INST inst);
const char* cpuGetAddrModeDesc(const M6502_ADDRMODE adrmode);
const char* cpuGetInstNameByOpcode(const opcode_t opcode);
bool cpuIsUsualOp(const opcode_t opcode);
