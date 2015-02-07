#include "macro.h"
#include "datatype.h"
#include "optable.h"

// GLOBALS /////////////////

static M6502_OPCODE    opdata[256];
static bool            usualOp[256]={false};
static const char*     adrmodeDesc[(int)M6502_ADDRMODE::_ADR_MAX];
static const char*     instName[(int)M6502_INST::_INS_MAX];

// FUNCTIONS ///////////////

M6502_OPCODE cpuGetOpData(const opcode_t opcode)
{
	return opdata[opcode];
}

const char* cpuGetInstNameByInst(const M6502_INST inst)
{
	// assert(inst>=0 && inst<_INS_MAX); // unnecessary in C++
	return instName[(int)inst];
}

const char* cpuGetInstNameByOpcode(const opcode_t opcode)
{
	return cpuGetInstNameByInst(opdata[opcode].inst);
}

const char* cpuGetAddrModeDesc(const M6502_ADDRMODE adrmode)
{
	// assert(adrmode>=0 && adrmode<_ADR_MAX);
	return adrmodeDesc[(int)adrmode];
}

bool cpuIsUsualOp(const opcode_t opcode)
{
    return usualOp[opcode];
}

void cpu_initTable()
{
    adrmodeDesc[(int)M6502_ADDRMODE::ADR_ABS]="Absolute";
    adrmodeDesc[(int)M6502_ADDRMODE::ADR_ABSX]="Absolute,X";
    adrmodeDesc[(int)M6502_ADDRMODE::ADR_ABSY]="Absolute,Y";
    adrmodeDesc[(int)M6502_ADDRMODE::ADR_IMM]="Immediate";
    adrmodeDesc[(int)M6502_ADDRMODE::ADR_IMP]="Implied";
    adrmodeDesc[(int)M6502_ADDRMODE::ADR_INDABSX]="M6502_ADDRMODE::ADR_INDABSX JMP 7C";
    adrmodeDesc[(int)M6502_ADDRMODE::ADR_IND]="Indirect Absolute (JMP)";
    adrmodeDesc[(int)M6502_ADDRMODE::ADR_INDX]="(IND,X) Preindexed Indirect";
    adrmodeDesc[(int)M6502_ADDRMODE::ADR_INDY]="(IND),Y Post-indexed Indirect mode";
    adrmodeDesc[(int)M6502_ADDRMODE::ADR_INDZP]="M6502_ADDRMODE::ADR_INDZP";
    adrmodeDesc[(int)M6502_ADDRMODE::ADR_REL]="Relative (Branch)";
    adrmodeDesc[(int)M6502_ADDRMODE::ADR_ZP]="Zero Page";
    adrmodeDesc[(int)M6502_ADDRMODE::ADR_ZPX]="Zero Page,X";
    adrmodeDesc[(int)M6502_ADDRMODE::ADR_ZPY]="Zero Page,Y";

    instName[(int)M6502_INST::INS_ADC]="ADC";
    instName[(int)M6502_INST::INS_AND]="AND";
    instName[(int)M6502_INST::INS_ASL]="ASL";
    instName[(int)M6502_INST::INS_ASLA]="ASLA";
    instName[(int)M6502_INST::INS_BCC]="BCC";
    instName[(int)M6502_INST::INS_BCS]="BCS";
    instName[(int)M6502_INST::INS_BEQ]="BEQ";
    instName[(int)M6502_INST::INS_BIT]="BIT";
    instName[(int)M6502_INST::INS_BMI]="BMI";
    instName[(int)M6502_INST::INS_BNE]="BNE";
    instName[(int)M6502_INST::INS_BPL]="BPL";
    instName[(int)M6502_INST::INS_BRK]="BRK";
    instName[(int)M6502_INST::INS_BVC]="BVC";
    instName[(int)M6502_INST::INS_BVS]="BVS";
    instName[(int)M6502_INST::INS_CLC]="CLC";
    instName[(int)M6502_INST::INS_CLD]="CLD";
    instName[(int)M6502_INST::INS_CLI]="CLI";
    instName[(int)M6502_INST::INS_CLV]="CLV";
    instName[(int)M6502_INST::INS_CMP]="CMP";
    instName[(int)M6502_INST::INS_CPX]="CPX";
    instName[(int)M6502_INST::INS_CPY]="CPY";
    instName[(int)M6502_INST::INS_DEC]="DEC";
    instName[(int)M6502_INST::INS_DEA]="DEA";
    instName[(int)M6502_INST::INS_DEX]="DEX";
    instName[(int)M6502_INST::INS_DEY]="DEY";
    instName[(int)M6502_INST::INS_EOR]="EOR";
    instName[(int)M6502_INST::INS_INC]="INC";
    instName[(int)M6502_INST::INS_INX]="INX";
    instName[(int)M6502_INST::INS_INY]="INY";
    instName[(int)M6502_INST::INS_JMP]="JMP";
    instName[(int)M6502_INST::INS_JSR]="JSR";
    instName[(int)M6502_INST::INS_LDA]="LDA";
    instName[(int)M6502_INST::INS_LDX]="LDX";
    instName[(int)M6502_INST::INS_LDY]="LDY";
    instName[(int)M6502_INST::INS_LSR]="LSR";
    instName[(int)M6502_INST::INS_LSRA]="LSRA";
    instName[(int)M6502_INST::INS_NOP]="NOP";
    instName[(int)M6502_INST::INS_ORA]="ORA";
    instName[(int)M6502_INST::INS_PHA]="PHA";
    instName[(int)M6502_INST::INS_PHP]="PHP";
    instName[(int)M6502_INST::INS_PLA]="PLA";
    instName[(int)M6502_INST::INS_PLP]="PLP";
    instName[(int)M6502_INST::INS_ROL]="ROL";
    instName[(int)M6502_INST::INS_ROLA]="ROLA";
    instName[(int)M6502_INST::INS_ROR]="ROR";
    instName[(int)M6502_INST::INS_RORA]="RORA";
    instName[(int)M6502_INST::INS_RTI]="RTI";
    instName[(int)M6502_INST::INS_RTS]="RTS";
    instName[(int)M6502_INST::INS_SBC]="SBC";
    instName[(int)M6502_INST::INS_SEC]="SEC";
    instName[(int)M6502_INST::INS_SED]="SED";
    instName[(int)M6502_INST::INS_SEI]="SEI";
    instName[(int)M6502_INST::INS_STA]="STA";
    instName[(int)M6502_INST::INS_STX]="STX";
    instName[(int)M6502_INST::INS_STY]="STY";
    instName[(int)M6502_INST::INS_TAX]="TAX";
    instName[(int)M6502_INST::INS_TAY]="TAY";
    instName[(int)M6502_INST::INS_TSX]="TSX";
    instName[(int)M6502_INST::INS_TXA]="TXA";
    instName[(int)M6502_INST::INS_TXS]="TXS";
    instName[(int)M6502_INST::INS_TYA]="TYA";
    instName[(int)M6502_INST::INS_BRA]="BRA";
    instName[(int)M6502_INST::INS_INA]="INA";
    instName[(int)M6502_INST::INS_PHX]="PHX";
    instName[(int)M6502_INST::INS_PLX]="PLX";
    instName[(int)M6502_INST::INS_PHY]="PHY";
    instName[(int)M6502_INST::INS_PLY]="PLY";

    memset(opdata,-1,sizeof(opdata));
    opdata[0x0].cycles=7;
    opdata[0x0].inst=M6502_INST::INS_BRK;
    opdata[0x0].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x1].cycles=6;
    opdata[0x1].inst=M6502_INST::INS_ORA;
    opdata[0x1].addrmode=M6502_ADDRMODE::ADR_INDX;
    opdata[0x2].cycles=2;
    opdata[0x2].inst=M6502_INST::INS_NOP;
    opdata[0x2].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x3].cycles=2;
    opdata[0x3].inst=M6502_INST::INS_NOP;
    opdata[0x3].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x4].cycles=3;
    opdata[0x4].inst=M6502_INST::INS_NOP;
    opdata[0x4].addrmode=M6502_ADDRMODE::ADR_ZP;
    opdata[0x5].cycles=3;
    opdata[0x5].inst=M6502_INST::INS_ORA;
    opdata[0x5].addrmode=M6502_ADDRMODE::ADR_ZP;
    opdata[0x6].cycles=5;
    opdata[0x6].inst=M6502_INST::INS_ASL;
    opdata[0x6].addrmode=M6502_ADDRMODE::ADR_ZP;
    opdata[0x7].cycles=2;
    opdata[0x7].inst=M6502_INST::INS_NOP;
    opdata[0x7].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x8].cycles=3;
    opdata[0x8].inst=M6502_INST::INS_PHP;
    opdata[0x8].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x9].cycles=3;
    opdata[0x9].inst=M6502_INST::INS_ORA;
    opdata[0x9].addrmode=M6502_ADDRMODE::ADR_IMM;
    opdata[0xA].cycles=2;
    opdata[0xA].inst=M6502_INST::INS_ASLA;
    opdata[0xA].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xB].cycles=2;
    opdata[0xB].inst=M6502_INST::INS_NOP;
    opdata[0xB].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xC].cycles=4;
    opdata[0xC].inst=M6502_INST::INS_NOP;
    opdata[0xC].addrmode=M6502_ADDRMODE::ADR_ABS;
    opdata[0xD].cycles=4;
    opdata[0xD].inst=M6502_INST::INS_ORA;
    opdata[0xD].addrmode=M6502_ADDRMODE::ADR_ABS;
    opdata[0xE].cycles=6;
    opdata[0xE].inst=M6502_INST::INS_ASL;
    opdata[0xE].addrmode=M6502_ADDRMODE::ADR_ABS;
    opdata[0xF].cycles=2;
    opdata[0xF].inst=M6502_INST::INS_NOP;
    opdata[0xF].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x10].cycles=2;
    opdata[0x10].inst=M6502_INST::INS_BPL;
    opdata[0x10].addrmode=M6502_ADDRMODE::ADR_REL;
    opdata[0x11].cycles=5;
    opdata[0x11].inst=M6502_INST::INS_ORA;
    opdata[0x11].addrmode=M6502_ADDRMODE::ADR_INDY;
    opdata[0x12].cycles=3;
    opdata[0x12].inst=M6502_INST::INS_ORA;
    opdata[0x12].addrmode=M6502_ADDRMODE::ADR_INDZP;
    opdata[0x13].cycles=2;
    opdata[0x13].inst=M6502_INST::INS_NOP;
    opdata[0x13].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x14].cycles=3;
    opdata[0x14].inst=M6502_INST::INS_NOP;
    opdata[0x14].addrmode=M6502_ADDRMODE::ADR_ZP;
    opdata[0x15].cycles=4;
    opdata[0x15].inst=M6502_INST::INS_ORA;
    opdata[0x15].addrmode=M6502_ADDRMODE::ADR_ZPX;
    opdata[0x16].cycles=6;
    opdata[0x16].inst=M6502_INST::INS_ASL;
    opdata[0x16].addrmode=M6502_ADDRMODE::ADR_ZPX;
    opdata[0x17].cycles=2;
    opdata[0x17].inst=M6502_INST::INS_NOP;
    opdata[0x17].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x18].cycles=2;
    opdata[0x18].inst=M6502_INST::INS_CLC;
    opdata[0x18].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x19].cycles=4;
    opdata[0x19].inst=M6502_INST::INS_ORA;
    opdata[0x19].addrmode=M6502_ADDRMODE::ADR_ABSY;
    opdata[0x1A].cycles=2;
    opdata[0x1A].inst=M6502_INST::INS_INA;
    opdata[0x1A].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x1B].cycles=2;
    opdata[0x1B].inst=M6502_INST::INS_NOP;
    opdata[0x1B].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x1C].cycles=4;
    opdata[0x1C].inst=M6502_INST::INS_NOP;
    opdata[0x1C].addrmode=M6502_ADDRMODE::ADR_ABS;
    opdata[0x1D].cycles=4;
    opdata[0x1D].inst=M6502_INST::INS_ORA;
    opdata[0x1D].addrmode=M6502_ADDRMODE::ADR_ABSX;
    opdata[0x1E].cycles=7;
    opdata[0x1E].inst=M6502_INST::INS_ASL;
    opdata[0x1E].addrmode=M6502_ADDRMODE::ADR_ABSX;
    opdata[0x1F].cycles=2;
    opdata[0x1F].inst=M6502_INST::INS_NOP;
    opdata[0x1F].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x20].cycles=6;
    opdata[0x20].inst=M6502_INST::INS_JSR;
    opdata[0x20].addrmode=M6502_ADDRMODE::ADR_ABS;
    opdata[0x21].cycles=6;
    opdata[0x21].inst=M6502_INST::INS_AND;
    opdata[0x21].addrmode=M6502_ADDRMODE::ADR_INDX;
    opdata[0x22].cycles=2;
    opdata[0x22].inst=M6502_INST::INS_NOP;
    opdata[0x22].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x23].cycles=2;
    opdata[0x23].inst=M6502_INST::INS_NOP;
    opdata[0x23].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x24].cycles=3;
    opdata[0x24].inst=M6502_INST::INS_BIT;
    opdata[0x24].addrmode=M6502_ADDRMODE::ADR_ZP;
    opdata[0x25].cycles=3;
    opdata[0x25].inst=M6502_INST::INS_AND;
    opdata[0x25].addrmode=M6502_ADDRMODE::ADR_ZP;
    opdata[0x26].cycles=5;
    opdata[0x26].inst=M6502_INST::INS_ROL;
    opdata[0x26].addrmode=M6502_ADDRMODE::ADR_ZP;
    opdata[0x27].cycles=2;
    opdata[0x27].inst=M6502_INST::INS_NOP;
    opdata[0x27].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x28].cycles=4;
    opdata[0x28].inst=M6502_INST::INS_PLP;
    opdata[0x28].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x29].cycles=3;
    opdata[0x29].inst=M6502_INST::INS_AND;
    opdata[0x29].addrmode=M6502_ADDRMODE::ADR_IMM;
    opdata[0x2A].cycles=2;
    opdata[0x2A].inst=M6502_INST::INS_ROLA;
    opdata[0x2A].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x2B].cycles=2;
    opdata[0x2B].inst=M6502_INST::INS_NOP;
    opdata[0x2B].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x2C].cycles=4;
    opdata[0x2C].inst=M6502_INST::INS_BIT;
    opdata[0x2C].addrmode=M6502_ADDRMODE::ADR_ABS;
    opdata[0x2D].cycles=4;
    opdata[0x2D].inst=M6502_INST::INS_AND;
    opdata[0x2D].addrmode=M6502_ADDRMODE::ADR_ABS;
    opdata[0x2E].cycles=6;
    opdata[0x2E].inst=M6502_INST::INS_ROL;
    opdata[0x2E].addrmode=M6502_ADDRMODE::ADR_ABS;
    opdata[0x2F].cycles=2;
    opdata[0x2F].inst=M6502_INST::INS_NOP;
    opdata[0x2F].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x30].cycles=2;
    opdata[0x30].inst=M6502_INST::INS_BMI;
    opdata[0x30].addrmode=M6502_ADDRMODE::ADR_REL;
    opdata[0x31].cycles=5;
    opdata[0x31].inst=M6502_INST::INS_AND;
    opdata[0x31].addrmode=M6502_ADDRMODE::ADR_INDY;
    opdata[0x32].cycles=3;
    opdata[0x32].inst=M6502_INST::INS_AND;
    opdata[0x32].addrmode=M6502_ADDRMODE::ADR_INDZP;
    opdata[0x33].cycles=2;
    opdata[0x33].inst=M6502_INST::INS_NOP;
    opdata[0x33].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x34].cycles=4;
    opdata[0x34].inst=M6502_INST::INS_BIT;
    opdata[0x34].addrmode=M6502_ADDRMODE::ADR_ZPX;
    opdata[0x35].cycles=4;
    opdata[0x35].inst=M6502_INST::INS_AND;
    opdata[0x35].addrmode=M6502_ADDRMODE::ADR_ZPX;
    opdata[0x36].cycles=6;
    opdata[0x36].inst=M6502_INST::INS_ROL;
    opdata[0x36].addrmode=M6502_ADDRMODE::ADR_ZPX;
    opdata[0x37].cycles=2;
    opdata[0x37].inst=M6502_INST::INS_NOP;
    opdata[0x37].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x38].cycles=2;
    opdata[0x38].inst=M6502_INST::INS_SEC;
    opdata[0x38].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x39].cycles=4;
    opdata[0x39].inst=M6502_INST::INS_AND;
    opdata[0x39].addrmode=M6502_ADDRMODE::ADR_ABSY;
    opdata[0x3A].cycles=2;
    opdata[0x3A].inst=M6502_INST::INS_DEA;
    opdata[0x3A].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x3B].cycles=2;
    opdata[0x3B].inst=M6502_INST::INS_NOP;
    opdata[0x3B].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x3C].cycles=4;
    opdata[0x3C].inst=M6502_INST::INS_BIT;
    opdata[0x3C].addrmode=M6502_ADDRMODE::ADR_ABSX;
    opdata[0x3D].cycles=4;
    opdata[0x3D].inst=M6502_INST::INS_AND;
    opdata[0x3D].addrmode=M6502_ADDRMODE::ADR_ABSX;
    opdata[0x3E].cycles=7;
    opdata[0x3E].inst=M6502_INST::INS_ROL;
    opdata[0x3E].addrmode=M6502_ADDRMODE::ADR_ABSX;
    opdata[0x3F].cycles=2;
    opdata[0x3F].inst=M6502_INST::INS_NOP;
    opdata[0x3F].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x40].cycles=6;
    opdata[0x40].inst=M6502_INST::INS_RTI;
    opdata[0x40].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x41].cycles=6;
    opdata[0x41].inst=M6502_INST::INS_EOR;
    opdata[0x41].addrmode=M6502_ADDRMODE::ADR_INDX;
    opdata[0x42].cycles=2;
    opdata[0x42].inst=M6502_INST::INS_NOP;
    opdata[0x42].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x43].cycles=2;
    opdata[0x43].inst=M6502_INST::INS_NOP;
    opdata[0x43].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x44].cycles=2;
    opdata[0x44].inst=M6502_INST::INS_NOP;
    opdata[0x44].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x45].cycles=3;
    opdata[0x45].inst=M6502_INST::INS_EOR;
    opdata[0x45].addrmode=M6502_ADDRMODE::ADR_ZP;
    opdata[0x46].cycles=5;
    opdata[0x46].inst=M6502_INST::INS_LSR;
    opdata[0x46].addrmode=M6502_ADDRMODE::ADR_ZP;
    opdata[0x47].cycles=2;
    opdata[0x47].inst=M6502_INST::INS_NOP;
    opdata[0x47].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x48].cycles=3;
    opdata[0x48].inst=M6502_INST::INS_PHA;
    opdata[0x48].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x49].cycles=3;
    opdata[0x49].inst=M6502_INST::INS_EOR;
    opdata[0x49].addrmode=M6502_ADDRMODE::ADR_IMM;
    opdata[0x4A].cycles=2;
    opdata[0x4A].inst=M6502_INST::INS_LSRA;
    opdata[0x4A].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x4B].cycles=2;
    opdata[0x4B].inst=M6502_INST::INS_NOP;
    opdata[0x4B].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x4C].cycles=3;
    opdata[0x4C].inst=M6502_INST::INS_JMP;
    opdata[0x4C].addrmode=M6502_ADDRMODE::ADR_ABS;
    opdata[0x4D].cycles=4;
    opdata[0x4D].inst=M6502_INST::INS_EOR;
    opdata[0x4D].addrmode=M6502_ADDRMODE::ADR_ABS;
    opdata[0x4E].cycles=6;
    opdata[0x4E].inst=M6502_INST::INS_LSR;
    opdata[0x4E].addrmode=M6502_ADDRMODE::ADR_ABS;
    opdata[0x4F].cycles=2;
    opdata[0x4F].inst=M6502_INST::INS_NOP;
    opdata[0x4F].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x50].cycles=2;
    opdata[0x50].inst=M6502_INST::INS_BVC;
    opdata[0x50].addrmode=M6502_ADDRMODE::ADR_REL;
    opdata[0x51].cycles=5;
    opdata[0x51].inst=M6502_INST::INS_EOR;
    opdata[0x51].addrmode=M6502_ADDRMODE::ADR_INDY;
    opdata[0x52].cycles=3;
    opdata[0x52].inst=M6502_INST::INS_EOR;
    opdata[0x52].addrmode=M6502_ADDRMODE::ADR_INDZP;
    opdata[0x53].cycles=2;
    opdata[0x53].inst=M6502_INST::INS_NOP;
    opdata[0x53].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x54].cycles=2;
    opdata[0x54].inst=M6502_INST::INS_NOP;
    opdata[0x54].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x55].cycles=4;
    opdata[0x55].inst=M6502_INST::INS_EOR;
    opdata[0x55].addrmode=M6502_ADDRMODE::ADR_ZPX;
    opdata[0x56].cycles=6;
    opdata[0x56].inst=M6502_INST::INS_LSR;
    opdata[0x56].addrmode=M6502_ADDRMODE::ADR_ZPX;
    opdata[0x57].cycles=2;
    opdata[0x57].inst=M6502_INST::INS_NOP;
    opdata[0x57].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x58].cycles=2;
    opdata[0x58].inst=M6502_INST::INS_CLI;
    opdata[0x58].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x59].cycles=4;
    opdata[0x59].inst=M6502_INST::INS_EOR;
    opdata[0x59].addrmode=M6502_ADDRMODE::ADR_ABSY;
    opdata[0x5A].cycles=3;
    opdata[0x5A].inst=M6502_INST::INS_PHY;
    opdata[0x5A].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x5B].cycles=2;
    opdata[0x5B].inst=M6502_INST::INS_NOP;
    opdata[0x5B].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x5C].cycles=2;
    opdata[0x5C].inst=M6502_INST::INS_NOP;
    opdata[0x5C].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x5D].cycles=4;
    opdata[0x5D].inst=M6502_INST::INS_EOR;
    opdata[0x5D].addrmode=M6502_ADDRMODE::ADR_ABSX;
    opdata[0x5E].cycles=7;
    opdata[0x5E].inst=M6502_INST::INS_LSR;
    opdata[0x5E].addrmode=M6502_ADDRMODE::ADR_ABSX;
    opdata[0x5F].cycles=2;
    opdata[0x5F].inst=M6502_INST::INS_NOP;
    opdata[0x5F].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x60].cycles=6;
    opdata[0x60].inst=M6502_INST::INS_RTS;
    opdata[0x60].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x61].cycles=6;
    opdata[0x61].inst=M6502_INST::INS_ADC;
    opdata[0x61].addrmode=M6502_ADDRMODE::ADR_INDX;
    opdata[0x62].cycles=2;
    opdata[0x62].inst=M6502_INST::INS_NOP;
    opdata[0x62].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x63].cycles=2;
    opdata[0x63].inst=M6502_INST::INS_NOP;
    opdata[0x63].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x64].cycles=3;
    opdata[0x64].inst=M6502_INST::INS_NOP;
    opdata[0x64].addrmode=M6502_ADDRMODE::ADR_ZP;
    opdata[0x65].cycles=3;
    opdata[0x65].inst=M6502_INST::INS_ADC;
    opdata[0x65].addrmode=M6502_ADDRMODE::ADR_ZP;
    opdata[0x66].cycles=5;
    opdata[0x66].inst=M6502_INST::INS_ROR;
    opdata[0x66].addrmode=M6502_ADDRMODE::ADR_ZP;
    opdata[0x67].cycles=2;
    opdata[0x67].inst=M6502_INST::INS_NOP;
    opdata[0x67].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x68].cycles=4;
    opdata[0x68].inst=M6502_INST::INS_PLA;
    opdata[0x68].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x69].cycles=3;
    opdata[0x69].inst=M6502_INST::INS_ADC;
    opdata[0x69].addrmode=M6502_ADDRMODE::ADR_IMM;
    opdata[0x6A].cycles=2;
    opdata[0x6A].inst=M6502_INST::INS_RORA;
    opdata[0x6A].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x6B].cycles=2;
    opdata[0x6B].inst=M6502_INST::INS_NOP;
    opdata[0x6B].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x6C].cycles=5;
    opdata[0x6C].inst=M6502_INST::INS_JMP;
    opdata[0x6C].addrmode=M6502_ADDRMODE::ADR_IND;
    opdata[0x6D].cycles=4;
    opdata[0x6D].inst=M6502_INST::INS_ADC;
    opdata[0x6D].addrmode=M6502_ADDRMODE::ADR_ABS;
    opdata[0x6E].cycles=6;
    opdata[0x6E].inst=M6502_INST::INS_ROR;
    opdata[0x6E].addrmode=M6502_ADDRMODE::ADR_ABS;
    opdata[0x6F].cycles=2;
    opdata[0x6F].inst=M6502_INST::INS_NOP;
    opdata[0x6F].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x70].cycles=2;
    opdata[0x70].inst=M6502_INST::INS_BVS;
    opdata[0x70].addrmode=M6502_ADDRMODE::ADR_REL;
    opdata[0x71].cycles=5;
    opdata[0x71].inst=M6502_INST::INS_ADC;
    opdata[0x71].addrmode=M6502_ADDRMODE::ADR_INDY;
    opdata[0x72].cycles=3;
    opdata[0x72].inst=M6502_INST::INS_ADC;
    opdata[0x72].addrmode=M6502_ADDRMODE::ADR_INDZP;
    opdata[0x73].cycles=2;
    opdata[0x73].inst=M6502_INST::INS_NOP;
    opdata[0x73].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x74].cycles=4;
    opdata[0x74].inst=M6502_INST::INS_NOP;
    opdata[0x74].addrmode=M6502_ADDRMODE::ADR_ZPX;
    opdata[0x75].cycles=4;
    opdata[0x75].inst=M6502_INST::INS_ADC;
    opdata[0x75].addrmode=M6502_ADDRMODE::ADR_ZPX;
    opdata[0x76].cycles=6;
    opdata[0x76].inst=M6502_INST::INS_ROR;
    opdata[0x76].addrmode=M6502_ADDRMODE::ADR_ZPX;
    opdata[0x77].cycles=2;
    opdata[0x77].inst=M6502_INST::INS_NOP;
    opdata[0x77].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x78].cycles=2;
    opdata[0x78].inst=M6502_INST::INS_SEI;
    opdata[0x78].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x79].cycles=4;
    opdata[0x79].inst=M6502_INST::INS_ADC;
    opdata[0x79].addrmode=M6502_ADDRMODE::ADR_ABSY;
    opdata[0x7A].cycles=4;
    opdata[0x7A].inst=M6502_INST::INS_PLY;
    opdata[0x7A].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x7B].cycles=2;
    opdata[0x7B].inst=M6502_INST::INS_NOP;
    opdata[0x7B].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x7C].cycles=6;
    opdata[0x7C].inst=M6502_INST::INS_JMP;
    opdata[0x7C].addrmode=M6502_ADDRMODE::ADR_INDABSX;
    opdata[0x7D].cycles=4;
    opdata[0x7D].inst=M6502_INST::INS_ADC;
    opdata[0x7D].addrmode=M6502_ADDRMODE::ADR_ABSX;
    opdata[0x7E].cycles=7;
    opdata[0x7E].inst=M6502_INST::INS_ROR;
    opdata[0x7E].addrmode=M6502_ADDRMODE::ADR_ABSX;
    opdata[0x7F].cycles=2;
    opdata[0x7F].inst=M6502_INST::INS_NOP;
    opdata[0x7F].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x80].cycles=2;
    opdata[0x80].inst=M6502_INST::INS_BRA;
    opdata[0x80].addrmode=M6502_ADDRMODE::ADR_REL;
    opdata[0x81].cycles=6;
    opdata[0x81].inst=M6502_INST::INS_STA;
    opdata[0x81].addrmode=M6502_ADDRMODE::ADR_INDX;
    opdata[0x82].cycles=2;
    opdata[0x82].inst=M6502_INST::INS_NOP;
    opdata[0x82].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x83].cycles=2;
    opdata[0x83].inst=M6502_INST::INS_NOP;
    opdata[0x83].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x84].cycles=2;
    opdata[0x84].inst=M6502_INST::INS_STY;
    opdata[0x84].addrmode=M6502_ADDRMODE::ADR_ZP;
    opdata[0x85].cycles=2;
    opdata[0x85].inst=M6502_INST::INS_STA;
    opdata[0x85].addrmode=M6502_ADDRMODE::ADR_ZP;
    opdata[0x86].cycles=2;
    opdata[0x86].inst=M6502_INST::INS_STX;
    opdata[0x86].addrmode=M6502_ADDRMODE::ADR_ZP;
    opdata[0x87].cycles=2;
    opdata[0x87].inst=M6502_INST::INS_NOP;
    opdata[0x87].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x88].cycles=2;
    opdata[0x88].inst=M6502_INST::INS_DEY;
    opdata[0x88].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x89].cycles=2;
    opdata[0x89].inst=M6502_INST::INS_BIT;
    opdata[0x89].addrmode=M6502_ADDRMODE::ADR_IMM;
    opdata[0x8A].cycles=2;
    opdata[0x8A].inst=M6502_INST::INS_TXA;
    opdata[0x8A].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x8B].cycles=2;
    opdata[0x8B].inst=M6502_INST::INS_NOP;
    opdata[0x8B].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x8C].cycles=4;
    opdata[0x8C].inst=M6502_INST::INS_STY;
    opdata[0x8C].addrmode=M6502_ADDRMODE::ADR_ABS;
    opdata[0x8D].cycles=4;
    opdata[0x8D].inst=M6502_INST::INS_STA;
    opdata[0x8D].addrmode=M6502_ADDRMODE::ADR_ABS;
    opdata[0x8E].cycles=4;
    opdata[0x8E].inst=M6502_INST::INS_STX;
    opdata[0x8E].addrmode=M6502_ADDRMODE::ADR_ABS;
    opdata[0x8F].cycles=2;
    opdata[0x8F].inst=M6502_INST::INS_NOP;
    opdata[0x8F].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x90].cycles=2;
    opdata[0x90].inst=M6502_INST::INS_BCC;
    opdata[0x90].addrmode=M6502_ADDRMODE::ADR_REL;
    opdata[0x91].cycles=6;
    opdata[0x91].inst=M6502_INST::INS_STA;
    opdata[0x91].addrmode=M6502_ADDRMODE::ADR_INDY;
    opdata[0x92].cycles=3;
    opdata[0x92].inst=M6502_INST::INS_STA;
    opdata[0x92].addrmode=M6502_ADDRMODE::ADR_INDZP;
    opdata[0x93].cycles=2;
    opdata[0x93].inst=M6502_INST::INS_NOP;
    opdata[0x93].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x94].cycles=4;
    opdata[0x94].inst=M6502_INST::INS_STY;
    opdata[0x94].addrmode=M6502_ADDRMODE::ADR_ZPX;
    opdata[0x95].cycles=4;
    opdata[0x95].inst=M6502_INST::INS_STA;
    opdata[0x95].addrmode=M6502_ADDRMODE::ADR_ZPX;
    opdata[0x96].cycles=4;
    opdata[0x96].inst=M6502_INST::INS_STX;
    opdata[0x96].addrmode=M6502_ADDRMODE::ADR_ZPY;
    opdata[0x97].cycles=2;
    opdata[0x97].inst=M6502_INST::INS_NOP;
    opdata[0x97].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x98].cycles=2;
    opdata[0x98].inst=M6502_INST::INS_TYA;
    opdata[0x98].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x99].cycles=5;
    opdata[0x99].inst=M6502_INST::INS_STA;
    opdata[0x99].addrmode=M6502_ADDRMODE::ADR_ABSY;
    opdata[0x9A].cycles=2;
    opdata[0x9A].inst=M6502_INST::INS_TXS;
    opdata[0x9A].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x9B].cycles=2;
    opdata[0x9B].inst=M6502_INST::INS_NOP;
    opdata[0x9B].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0x9C].cycles=4;
    opdata[0x9C].inst=M6502_INST::INS_NOP;
    opdata[0x9C].addrmode=M6502_ADDRMODE::ADR_ABS;
    opdata[0x9D].cycles=5;
    opdata[0x9D].inst=M6502_INST::INS_STA;
    opdata[0x9D].addrmode=M6502_ADDRMODE::ADR_ABSX;
    opdata[0x9E].cycles=5;
    opdata[0x9E].inst=M6502_INST::INS_NOP;
    opdata[0x9E].addrmode=M6502_ADDRMODE::ADR_ABSX;
    opdata[0x9F].cycles=2;
    opdata[0x9F].inst=M6502_INST::INS_NOP;
    opdata[0x9F].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xA0].cycles=3;
    opdata[0xA0].inst=M6502_INST::INS_LDY;
    opdata[0xA0].addrmode=M6502_ADDRMODE::ADR_IMM;
    opdata[0xA1].cycles=6;
    opdata[0xA1].inst=M6502_INST::INS_LDA;
    opdata[0xA1].addrmode=M6502_ADDRMODE::ADR_INDX;
    opdata[0xA2].cycles=3;
    opdata[0xA2].inst=M6502_INST::INS_LDX;
    opdata[0xA2].addrmode=M6502_ADDRMODE::ADR_IMM;
    opdata[0xA3].cycles=2;
    opdata[0xA3].inst=M6502_INST::INS_NOP;
    opdata[0xA3].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xA4].cycles=3;
    opdata[0xA4].inst=M6502_INST::INS_LDY;
    opdata[0xA4].addrmode=M6502_ADDRMODE::ADR_ZP;
    opdata[0xA5].cycles=3;
    opdata[0xA5].inst=M6502_INST::INS_LDA;
    opdata[0xA5].addrmode=M6502_ADDRMODE::ADR_ZP;
    opdata[0xA6].cycles=3;
    opdata[0xA6].inst=M6502_INST::INS_LDX;
    opdata[0xA6].addrmode=M6502_ADDRMODE::ADR_ZP;
    opdata[0xA7].cycles=2;
    opdata[0xA7].inst=M6502_INST::INS_NOP;
    opdata[0xA7].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xA8].cycles=2;
    opdata[0xA8].inst=M6502_INST::INS_TAY;
    opdata[0xA8].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xA9].cycles=3;
    opdata[0xA9].inst=M6502_INST::INS_LDA;
    opdata[0xA9].addrmode=M6502_ADDRMODE::ADR_IMM;
    opdata[0xAA].cycles=2;
    opdata[0xAA].inst=M6502_INST::INS_TAX;
    opdata[0xAA].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xAB].cycles=2;
    opdata[0xAB].inst=M6502_INST::INS_NOP;
    opdata[0xAB].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xAC].cycles=4;
    opdata[0xAC].inst=M6502_INST::INS_LDY;
    opdata[0xAC].addrmode=M6502_ADDRMODE::ADR_ABS;
    opdata[0xAD].cycles=4;
    opdata[0xAD].inst=M6502_INST::INS_LDA;
    opdata[0xAD].addrmode=M6502_ADDRMODE::ADR_ABS;
    opdata[0xAE].cycles=4;
    opdata[0xAE].inst=M6502_INST::INS_LDX;
    opdata[0xAE].addrmode=M6502_ADDRMODE::ADR_ABS;
    opdata[0xAF].cycles=2;
    opdata[0xAF].inst=M6502_INST::INS_NOP;
    opdata[0xAF].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xB0].cycles=2;
    opdata[0xB0].inst=M6502_INST::INS_BCS;
    opdata[0xB0].addrmode=M6502_ADDRMODE::ADR_REL;
    opdata[0xB1].cycles=5;
    opdata[0xB1].inst=M6502_INST::INS_LDA;
    opdata[0xB1].addrmode=M6502_ADDRMODE::ADR_INDY;
    opdata[0xB2].cycles=3;
    opdata[0xB2].inst=M6502_INST::INS_LDA;
    opdata[0xB2].addrmode=M6502_ADDRMODE::ADR_INDZP;
    opdata[0xB3].cycles=2;
    opdata[0xB3].inst=M6502_INST::INS_NOP;
    opdata[0xB3].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xB4].cycles=4;
    opdata[0xB4].inst=M6502_INST::INS_LDY;
    opdata[0xB4].addrmode=M6502_ADDRMODE::ADR_ZPX;
    opdata[0xB5].cycles=4;
    opdata[0xB5].inst=M6502_INST::INS_LDA;
    opdata[0xB5].addrmode=M6502_ADDRMODE::ADR_ZPX;
    opdata[0xB6].cycles=4;
    opdata[0xB6].inst=M6502_INST::INS_LDX;
    opdata[0xB6].addrmode=M6502_ADDRMODE::ADR_ZPY;
    opdata[0xB7].cycles=2;
    opdata[0xB7].inst=M6502_INST::INS_NOP;
    opdata[0xB7].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xB8].cycles=2;
    opdata[0xB8].inst=M6502_INST::INS_CLV;
    opdata[0xB8].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xB9].cycles=4;
    opdata[0xB9].inst=M6502_INST::INS_LDA;
    opdata[0xB9].addrmode=M6502_ADDRMODE::ADR_ABSY;
    opdata[0xBA].cycles=2;
    opdata[0xBA].inst=M6502_INST::INS_TSX;
    opdata[0xBA].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xBB].cycles=2;
    opdata[0xBB].inst=M6502_INST::INS_NOP;
    opdata[0xBB].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xBC].cycles=4;
    opdata[0xBC].inst=M6502_INST::INS_LDY;
    opdata[0xBC].addrmode=M6502_ADDRMODE::ADR_ABSX;
    opdata[0xBD].cycles=4;
    opdata[0xBD].inst=M6502_INST::INS_LDA;
    opdata[0xBD].addrmode=M6502_ADDRMODE::ADR_ABSX;
    opdata[0xBE].cycles=4;
    opdata[0xBE].inst=M6502_INST::INS_LDX;
    opdata[0xBE].addrmode=M6502_ADDRMODE::ADR_ABSY;
    opdata[0xBF].cycles=2;
    opdata[0xBF].inst=M6502_INST::INS_NOP;
    opdata[0xBF].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xC0].cycles=3;
    opdata[0xC0].inst=M6502_INST::INS_CPY;
    opdata[0xC0].addrmode=M6502_ADDRMODE::ADR_IMM;
    opdata[0xC1].cycles=6;
    opdata[0xC1].inst=M6502_INST::INS_CMP;
    opdata[0xC1].addrmode=M6502_ADDRMODE::ADR_INDX;
    opdata[0xC2].cycles=2;
    opdata[0xC2].inst=M6502_INST::INS_NOP;
    opdata[0xC2].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xC3].cycles=2;
    opdata[0xC3].inst=M6502_INST::INS_NOP;
    opdata[0xC3].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xC4].cycles=3;
    opdata[0xC4].inst=M6502_INST::INS_CPY;
    opdata[0xC4].addrmode=M6502_ADDRMODE::ADR_ZP;
    opdata[0xC5].cycles=3;
    opdata[0xC5].inst=M6502_INST::INS_CMP;
    opdata[0xC5].addrmode=M6502_ADDRMODE::ADR_ZP;
    opdata[0xC6].cycles=5;
    opdata[0xC6].inst=M6502_INST::INS_DEC;
    opdata[0xC6].addrmode=M6502_ADDRMODE::ADR_ZP;
    opdata[0xC7].cycles=2;
    opdata[0xC7].inst=M6502_INST::INS_NOP;
    opdata[0xC7].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xC8].cycles=2;
    opdata[0xC8].inst=M6502_INST::INS_INY;
    opdata[0xC8].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xC9].cycles=3;
    opdata[0xC9].inst=M6502_INST::INS_CMP;
    opdata[0xC9].addrmode=M6502_ADDRMODE::ADR_IMM;
    opdata[0xCA].cycles=2;
    opdata[0xCA].inst=M6502_INST::INS_DEX;
    opdata[0xCA].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xCB].cycles=2;
    opdata[0xCB].inst=M6502_INST::INS_NOP;
    opdata[0xCB].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xCC].cycles=4;
    opdata[0xCC].inst=M6502_INST::INS_CPY;
    opdata[0xCC].addrmode=M6502_ADDRMODE::ADR_ABS;
    opdata[0xCD].cycles=4;
    opdata[0xCD].inst=M6502_INST::INS_CMP;
    opdata[0xCD].addrmode=M6502_ADDRMODE::ADR_ABS;
    opdata[0xCE].cycles=6;
    opdata[0xCE].inst=M6502_INST::INS_DEC;
    opdata[0xCE].addrmode=M6502_ADDRMODE::ADR_ABS;
    opdata[0xCF].cycles=2;
    opdata[0xCF].inst=M6502_INST::INS_NOP;
    opdata[0xCF].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xD0].cycles=2;
    opdata[0xD0].inst=M6502_INST::INS_BNE;
    opdata[0xD0].addrmode=M6502_ADDRMODE::ADR_REL;
    opdata[0xD1].cycles=5;
    opdata[0xD1].inst=M6502_INST::INS_CMP;
    opdata[0xD1].addrmode=M6502_ADDRMODE::ADR_INDY;
    opdata[0xD2].cycles=3;
    opdata[0xD2].inst=M6502_INST::INS_CMP;
    opdata[0xD2].addrmode=M6502_ADDRMODE::ADR_INDZP;
    opdata[0xD3].cycles=2;
    opdata[0xD3].inst=M6502_INST::INS_NOP;
    opdata[0xD3].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xD4].cycles=2;
    opdata[0xD4].inst=M6502_INST::INS_NOP;
    opdata[0xD4].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xD5].cycles=4;
    opdata[0xD5].inst=M6502_INST::INS_CMP;
    opdata[0xD5].addrmode=M6502_ADDRMODE::ADR_ZPX;
    opdata[0xD6].cycles=6;
    opdata[0xD6].inst=M6502_INST::INS_DEC;
    opdata[0xD6].addrmode=M6502_ADDRMODE::ADR_ZPX;
    opdata[0xD7].cycles=2;
    opdata[0xD7].inst=M6502_INST::INS_NOP;
    opdata[0xD7].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xD8].cycles=2;
    opdata[0xD8].inst=M6502_INST::INS_CLD;
    opdata[0xD8].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xD9].cycles=4;
    opdata[0xD9].inst=M6502_INST::INS_CMP;
    opdata[0xD9].addrmode=M6502_ADDRMODE::ADR_ABSY;
    opdata[0xDA].cycles=3;
    opdata[0xDA].inst=M6502_INST::INS_PHX;
    opdata[0xDA].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xDB].cycles=2;
    opdata[0xDB].inst=M6502_INST::INS_NOP;
    opdata[0xDB].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xDC].cycles=2;
    opdata[0xDC].inst=M6502_INST::INS_NOP;
    opdata[0xDC].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xDD].cycles=4;
    opdata[0xDD].inst=M6502_INST::INS_CMP;
    opdata[0xDD].addrmode=M6502_ADDRMODE::ADR_ABSX;
    opdata[0xDE].cycles=7;
    opdata[0xDE].inst=M6502_INST::INS_DEC;
    opdata[0xDE].addrmode=M6502_ADDRMODE::ADR_ABSX;
    opdata[0xDF].cycles=2;
    opdata[0xDF].inst=M6502_INST::INS_NOP;
    opdata[0xDF].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xE0].cycles=3;
    opdata[0xE0].inst=M6502_INST::INS_CPX;
    opdata[0xE0].addrmode=M6502_ADDRMODE::ADR_IMM;
    opdata[0xE1].cycles=6;
    opdata[0xE1].inst=M6502_INST::INS_SBC;
    opdata[0xE1].addrmode=M6502_ADDRMODE::ADR_INDX;
    opdata[0xE2].cycles=2;
    opdata[0xE2].inst=M6502_INST::INS_NOP;
    opdata[0xE2].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xE3].cycles=2;
    opdata[0xE3].inst=M6502_INST::INS_NOP;
    opdata[0xE3].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xE4].cycles=3;
    opdata[0xE4].inst=M6502_INST::INS_CPX;
    opdata[0xE4].addrmode=M6502_ADDRMODE::ADR_ZP;
    opdata[0xE5].cycles=3;
    opdata[0xE5].inst=M6502_INST::INS_SBC;
    opdata[0xE5].addrmode=M6502_ADDRMODE::ADR_ZP;
    opdata[0xE6].cycles=5;
    opdata[0xE6].inst=M6502_INST::INS_INC;
    opdata[0xE6].addrmode=M6502_ADDRMODE::ADR_ZP;
    opdata[0xE7].cycles=2;
    opdata[0xE7].inst=M6502_INST::INS_NOP;
    opdata[0xE7].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xE8].cycles=2;
    opdata[0xE8].inst=M6502_INST::INS_INX;
    opdata[0xE8].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xE9].cycles=3;
    opdata[0xE9].inst=M6502_INST::INS_SBC;
    opdata[0xE9].addrmode=M6502_ADDRMODE::ADR_IMM;
    opdata[0xEA].cycles=2;
    opdata[0xEA].inst=M6502_INST::INS_NOP;
    opdata[0xEA].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xEB].cycles=2;
    opdata[0xEB].inst=M6502_INST::INS_NOP;
    opdata[0xEB].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xEC].cycles=4;
    opdata[0xEC].inst=M6502_INST::INS_CPX;
    opdata[0xEC].addrmode=M6502_ADDRMODE::ADR_ABS;
    opdata[0xED].cycles=4;
    opdata[0xED].inst=M6502_INST::INS_SBC;
    opdata[0xED].addrmode=M6502_ADDRMODE::ADR_ABS;
    opdata[0xEE].cycles=6;
    opdata[0xEE].inst=M6502_INST::INS_INC;
    opdata[0xEE].addrmode=M6502_ADDRMODE::ADR_ABS;
    opdata[0xEF].cycles=2;
    opdata[0xEF].inst=M6502_INST::INS_NOP;
    opdata[0xEF].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xF0].cycles=2;
    opdata[0xF0].inst=M6502_INST::INS_BEQ;
    opdata[0xF0].addrmode=M6502_ADDRMODE::ADR_REL;
    opdata[0xF1].cycles=5;
    opdata[0xF1].inst=M6502_INST::INS_SBC;
    opdata[0xF1].addrmode=M6502_ADDRMODE::ADR_INDY;
    opdata[0xF2].cycles=3;
    opdata[0xF2].inst=M6502_INST::INS_SBC;
    opdata[0xF2].addrmode=M6502_ADDRMODE::ADR_INDZP;
    opdata[0xF3].cycles=2;
    opdata[0xF3].inst=M6502_INST::INS_NOP;
    opdata[0xF3].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xF4].cycles=2;
    opdata[0xF4].inst=M6502_INST::INS_NOP;
    opdata[0xF4].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xF5].cycles=4;
    opdata[0xF5].inst=M6502_INST::INS_SBC;
    opdata[0xF5].addrmode=M6502_ADDRMODE::ADR_ZPX;
    opdata[0xF6].cycles=6;
    opdata[0xF6].inst=M6502_INST::INS_INC;
    opdata[0xF6].addrmode=M6502_ADDRMODE::ADR_ZPX;
    opdata[0xF7].cycles=2;
    opdata[0xF7].inst=M6502_INST::INS_NOP;
    opdata[0xF7].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xF8].cycles=2;
    opdata[0xF8].inst=M6502_INST::INS_SED;
    opdata[0xF8].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xF9].cycles=4;
    opdata[0xF9].inst=M6502_INST::INS_SBC;
    opdata[0xF9].addrmode=M6502_ADDRMODE::ADR_ABSY;
    opdata[0xFA].cycles=4;
    opdata[0xFA].inst=M6502_INST::INS_PLX;
    opdata[0xFA].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xFB].cycles=2;
    opdata[0xFB].inst=M6502_INST::INS_NOP;
    opdata[0xFB].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xFC].cycles=2;
    opdata[0xFC].inst=M6502_INST::INS_NOP;
    opdata[0xFC].addrmode=M6502_ADDRMODE::ADR_IMP;
    opdata[0xFD].cycles=4;
    opdata[0xFD].inst=M6502_INST::INS_SBC;
    opdata[0xFD].addrmode=M6502_ADDRMODE::ADR_ABSX;
    opdata[0xFE].cycles=7;
    opdata[0xFE].inst=M6502_INST::INS_INC;
    opdata[0xFE].addrmode=M6502_ADDRMODE::ADR_ABSX;
    opdata[0xFF].cycles=2;
    opdata[0xFF].inst=M6502_INST::INS_NOP;
    opdata[0xFF].addrmode=M6502_ADDRMODE::ADR_IMP;
}

static int cpu_setOp(const M6502_INST inst, \
                     const opcode_t opcode, \
                     const M6502_ADDRMODE adrmode, \
                     uint8_t size, \
                     uint8_t cycles)
{
    assert(inst==opdata[opcode].inst);
    opdata[opcode].size=size;
    usualOp[opcode]=true;
    if (adrmode!=opdata[opcode].addrmode || cycles!=opdata[opcode].cycles)
    {
        printf("[CPU] Test failed: Data Mismatch\n");
        printf("\t opcode=%02X(%s)\n",opcode,cpuGetInstNameByOpcode(opcode));
        if (inst!=opdata[opcode].inst)
        {
            printf("\tinst=%s[%u] (ref=%s[%u])\n", \
                   cpuGetInstNameByInst(inst),inst, \
                   cpuGetInstNameByInst(opdata[opcode].inst),(M6502_INST)opdata[opcode].inst);
        }
        if (adrmode!=opdata[opcode].addrmode)
        {
            printf("\taddrmode=%u (ref=%u)\n",adrmode,(M6502_ADDRMODE)opdata[opcode].addrmode);
        }
        if (cycles!=opdata[opcode].cycles)
        {
            printf("\tcycles=%u (ref=%u)\n",cycles,opdata[opcode].cycles);
        }
		return 0;
    }else return 1;
}

void cpu_testTable()
{
    // ADC:
    cpu_setOp(M6502_INST::INS_ADC,0x69,M6502_ADDRMODE::ADR_IMM,2,2);
    cpu_setOp(M6502_INST::INS_ADC,0x65,M6502_ADDRMODE::ADR_ZP,2,3);
    cpu_setOp(M6502_INST::INS_ADC,0x75,M6502_ADDRMODE::ADR_ZPX,2,4);
    cpu_setOp(M6502_INST::INS_ADC,0x6D,M6502_ADDRMODE::ADR_ABS,3,4);
    cpu_setOp(M6502_INST::INS_ADC,0x7D,M6502_ADDRMODE::ADR_ABSX,3,4);
    cpu_setOp(M6502_INST::INS_ADC,0x79,M6502_ADDRMODE::ADR_ABSY,3,4);
    cpu_setOp(M6502_INST::INS_ADC,0x61,M6502_ADDRMODE::ADR_PREIDXIND,2,6);
    cpu_setOp(M6502_INST::INS_ADC,0x71,M6502_ADDRMODE::ADR_POSTIDXIND,2,5);

    // AND:
    cpu_setOp(M6502_INST::INS_AND,0x29,M6502_ADDRMODE::ADR_IMM,2,2);
    cpu_setOp(M6502_INST::INS_AND,0x25,M6502_ADDRMODE::ADR_ZP,2,3);
    cpu_setOp(M6502_INST::INS_AND,0x35,M6502_ADDRMODE::ADR_ZPX,2,4);
    cpu_setOp(M6502_INST::INS_AND,0x2D,M6502_ADDRMODE::ADR_ABS,3,4);
    cpu_setOp(M6502_INST::INS_AND,0x3D,M6502_ADDRMODE::ADR_ABSX,3,4);
    cpu_setOp(M6502_INST::INS_AND,0x39,M6502_ADDRMODE::ADR_ABSY,3,4);
    cpu_setOp(M6502_INST::INS_AND,0x21,M6502_ADDRMODE::ADR_PREIDXIND,2,6);
    cpu_setOp(M6502_INST::INS_AND,0x31,M6502_ADDRMODE::ADR_POSTIDXIND,2,5);

    // ASL:
    cpu_setOp(M6502_INST::INS_ASLA,0x0A,M6502_ADDRMODE::ADR_ACC,1,2);
    cpu_setOp(M6502_INST::INS_ASL,0x06,M6502_ADDRMODE::ADR_ZP,2,5);
    cpu_setOp(M6502_INST::INS_ASL,0x16,M6502_ADDRMODE::ADR_ZPX,2,6);
    cpu_setOp(M6502_INST::INS_ASL,0x0E,M6502_ADDRMODE::ADR_ABS,3,6);
    cpu_setOp(M6502_INST::INS_ASL,0x1E,M6502_ADDRMODE::ADR_ABSX,3,7);

    // BCC:
    cpu_setOp(M6502_INST::INS_BCC,0x90,M6502_ADDRMODE::ADR_REL,2,2);

    // BCS:
    cpu_setOp(M6502_INST::INS_BCS,0xB0,M6502_ADDRMODE::ADR_REL,2,2);

    // BEQ:
    cpu_setOp(M6502_INST::INS_BEQ,0xF0,M6502_ADDRMODE::ADR_REL,2,2);

    // BIT:
    cpu_setOp(M6502_INST::INS_BIT,0x24,M6502_ADDRMODE::ADR_ZP,2,3);
    cpu_setOp(M6502_INST::INS_BIT,0x2C,M6502_ADDRMODE::ADR_ABS,3,4);

    // BMI:
    cpu_setOp(M6502_INST::INS_BMI,0x30,M6502_ADDRMODE::ADR_REL,2,2);

    // BNE:
    cpu_setOp(M6502_INST::INS_BNE,0xD0,M6502_ADDRMODE::ADR_REL,2,2);

    // BPL:
    cpu_setOp(M6502_INST::INS_BPL,0x10,M6502_ADDRMODE::ADR_REL,2,2);

    // BRK:
    cpu_setOp(M6502_INST::INS_BRK,0x00,M6502_ADDRMODE::ADR_IMP,1,7);

    // BVC:
    cpu_setOp(M6502_INST::INS_BVC,0x50,M6502_ADDRMODE::ADR_REL,2,2);

    // BVS:
    cpu_setOp(M6502_INST::INS_BVS,0x70,M6502_ADDRMODE::ADR_REL,2,2);

    // CLC:
    cpu_setOp(M6502_INST::INS_CLC,0x18,M6502_ADDRMODE::ADR_IMP,1,2);

    // CLD:
    cpu_setOp(M6502_INST::INS_CLD,0xD8,M6502_ADDRMODE::ADR_IMP,1,2);

    // CLI:
    cpu_setOp(M6502_INST::INS_CLI,0x58,M6502_ADDRMODE::ADR_IMP,1,2);

    // CLV:
    cpu_setOp(M6502_INST::INS_CLV,0xB8,M6502_ADDRMODE::ADR_IMP,1,2);

    // CMP:
    cpu_setOp(M6502_INST::INS_CMP,0xC9,M6502_ADDRMODE::ADR_IMM,2,2);
    cpu_setOp(M6502_INST::INS_CMP,0xC5,M6502_ADDRMODE::ADR_ZP,2,3);
    cpu_setOp(M6502_INST::INS_CMP,0xD5,M6502_ADDRMODE::ADR_ZPX,2,4);
    cpu_setOp(M6502_INST::INS_CMP,0xCD,M6502_ADDRMODE::ADR_ABS,3,4);
    cpu_setOp(M6502_INST::INS_CMP,0xDD,M6502_ADDRMODE::ADR_ABSX,3,4);
    cpu_setOp(M6502_INST::INS_CMP,0xD9,M6502_ADDRMODE::ADR_ABSY,3,4);
    cpu_setOp(M6502_INST::INS_CMP,0xC1,M6502_ADDRMODE::ADR_PREIDXIND,2,6);
    cpu_setOp(M6502_INST::INS_CMP,0xD1,M6502_ADDRMODE::ADR_POSTIDXIND,2,5);

    // CPX:
    cpu_setOp(M6502_INST::INS_CPX,0xE0,M6502_ADDRMODE::ADR_IMM,2,2);
    cpu_setOp(M6502_INST::INS_CPX,0xE4,M6502_ADDRMODE::ADR_ZP,2,3);
    cpu_setOp(M6502_INST::INS_CPX,0xEC,M6502_ADDRMODE::ADR_ABS,3,4);

    // CPY:
    cpu_setOp(M6502_INST::INS_CPY,0xC0,M6502_ADDRMODE::ADR_IMM,2,2);
    cpu_setOp(M6502_INST::INS_CPY,0xC4,M6502_ADDRMODE::ADR_ZP,2,3);
    cpu_setOp(M6502_INST::INS_CPY,0xCC,M6502_ADDRMODE::ADR_ABS,3,4);

    // DEC:
    cpu_setOp(M6502_INST::INS_DEC,0xC6,M6502_ADDRMODE::ADR_ZP,2,5);
    cpu_setOp(M6502_INST::INS_DEC,0xD6,M6502_ADDRMODE::ADR_ZPX,2,6);
    cpu_setOp(M6502_INST::INS_DEC,0xCE,M6502_ADDRMODE::ADR_ABS,3,6);
    cpu_setOp(M6502_INST::INS_DEC,0xDE,M6502_ADDRMODE::ADR_ABSX,3,7);

    // DEX:
    cpu_setOp(M6502_INST::INS_DEX,0xCA,M6502_ADDRMODE::ADR_IMP,1,2);

    // DEY:
    cpu_setOp(M6502_INST::INS_DEY,0x88,M6502_ADDRMODE::ADR_IMP,1,2);

    // EOR:
    cpu_setOp(M6502_INST::INS_EOR,0x49,M6502_ADDRMODE::ADR_IMM,2,2);
    cpu_setOp(M6502_INST::INS_EOR,0x45,M6502_ADDRMODE::ADR_ZP,2,3);
    cpu_setOp(M6502_INST::INS_EOR,0x55,M6502_ADDRMODE::ADR_ZPX,2,4);
    cpu_setOp(M6502_INST::INS_EOR,0x4D,M6502_ADDRMODE::ADR_ABS,3,4);
    cpu_setOp(M6502_INST::INS_EOR,0x5D,M6502_ADDRMODE::ADR_ABSX,3,4);
    cpu_setOp(M6502_INST::INS_EOR,0x59,M6502_ADDRMODE::ADR_ABSY,3,4);
    cpu_setOp(M6502_INST::INS_EOR,0x41,M6502_ADDRMODE::ADR_PREIDXIND,2,6);
    cpu_setOp(M6502_INST::INS_EOR,0x51,M6502_ADDRMODE::ADR_POSTIDXIND,2,5);

    // INC:
    cpu_setOp(M6502_INST::INS_INC,0xE6,M6502_ADDRMODE::ADR_ZP,2,5);
    cpu_setOp(M6502_INST::INS_INC,0xF6,M6502_ADDRMODE::ADR_ZPX,2,6);
    cpu_setOp(M6502_INST::INS_INC,0xEE,M6502_ADDRMODE::ADR_ABS,3,6);
    cpu_setOp(M6502_INST::INS_INC,0xFE,M6502_ADDRMODE::ADR_ABSX,3,7);

    // INX:
    cpu_setOp(M6502_INST::INS_INX,0xE8,M6502_ADDRMODE::ADR_IMP,1,2);

    // INY:
    cpu_setOp(M6502_INST::INS_INY,0xC8,M6502_ADDRMODE::ADR_IMP,1,2);

    // JMP:
    cpu_setOp(M6502_INST::INS_JMP,0x4C,M6502_ADDRMODE::ADR_ABS,3,3);
    cpu_setOp(M6502_INST::INS_JMP,0x6C,M6502_ADDRMODE::ADR_IND/*ABS*/,3,5);

    // JSR:
    cpu_setOp(M6502_INST::INS_JSR,0x20,M6502_ADDRMODE::ADR_ABS,3,6);

    // LDA:
    cpu_setOp(M6502_INST::INS_LDA,0xA9,M6502_ADDRMODE::ADR_IMM,2,2);
    cpu_setOp(M6502_INST::INS_LDA,0xA5,M6502_ADDRMODE::ADR_ZP,2,3);
    cpu_setOp(M6502_INST::INS_LDA,0xB5,M6502_ADDRMODE::ADR_ZPX,2,4);
    cpu_setOp(M6502_INST::INS_LDA,0xAD,M6502_ADDRMODE::ADR_ABS,3,4);
    cpu_setOp(M6502_INST::INS_LDA,0xBD,M6502_ADDRMODE::ADR_ABSX,3,4);
    cpu_setOp(M6502_INST::INS_LDA,0xB9,M6502_ADDRMODE::ADR_ABSY,3,4);
    cpu_setOp(M6502_INST::INS_LDA,0xA1,M6502_ADDRMODE::ADR_PREIDXIND,2,6);
    cpu_setOp(M6502_INST::INS_LDA,0xB1,M6502_ADDRMODE::ADR_POSTIDXIND,2,5);


    // LDX:
    cpu_setOp(M6502_INST::INS_LDX,0xA2,M6502_ADDRMODE::ADR_IMM,2,2);
    cpu_setOp(M6502_INST::INS_LDX,0xA6,M6502_ADDRMODE::ADR_ZP,2,3);
    cpu_setOp(M6502_INST::INS_LDX,0xB6,M6502_ADDRMODE::ADR_ZPY,2,4);
    cpu_setOp(M6502_INST::INS_LDX,0xAE,M6502_ADDRMODE::ADR_ABS,3,4);
    cpu_setOp(M6502_INST::INS_LDX,0xBE,M6502_ADDRMODE::ADR_ABSY,3,4);

    // LDY:
    cpu_setOp(M6502_INST::INS_LDY,0xA0,M6502_ADDRMODE::ADR_IMM,2,2);
    cpu_setOp(M6502_INST::INS_LDY,0xA4,M6502_ADDRMODE::ADR_ZP,2,3);
    cpu_setOp(M6502_INST::INS_LDY,0xB4,M6502_ADDRMODE::ADR_ZPX,2,4);
    cpu_setOp(M6502_INST::INS_LDY,0xAC,M6502_ADDRMODE::ADR_ABS,3,4);
    cpu_setOp(M6502_INST::INS_LDY,0xBC,M6502_ADDRMODE::ADR_ABSX,3,4);

    // LSR:
    cpu_setOp(M6502_INST::INS_LSRA,0x4A,M6502_ADDRMODE::ADR_ACC,1,2);
    cpu_setOp(M6502_INST::INS_LSR,0x46,M6502_ADDRMODE::ADR_ZP,2,5);
    cpu_setOp(M6502_INST::INS_LSR,0x56,M6502_ADDRMODE::ADR_ZPX,2,6);
    cpu_setOp(M6502_INST::INS_LSR,0x4E,M6502_ADDRMODE::ADR_ABS,3,6);
    cpu_setOp(M6502_INST::INS_LSR,0x5E,M6502_ADDRMODE::ADR_ABSX,3,7);

    // NOP:
    cpu_setOp(M6502_INST::INS_NOP,0xEA,M6502_ADDRMODE::ADR_IMP,1,2);

    // ORA:
    cpu_setOp(M6502_INST::INS_ORA,0x09,M6502_ADDRMODE::ADR_IMM,2,2);
    cpu_setOp(M6502_INST::INS_ORA,0x05,M6502_ADDRMODE::ADR_ZP,2,3);
    cpu_setOp(M6502_INST::INS_ORA,0x15,M6502_ADDRMODE::ADR_ZPX,2,4);
    cpu_setOp(M6502_INST::INS_ORA,0x0D,M6502_ADDRMODE::ADR_ABS,3,4);
    cpu_setOp(M6502_INST::INS_ORA,0x1D,M6502_ADDRMODE::ADR_ABSX,3,4);
    cpu_setOp(M6502_INST::INS_ORA,0x19,M6502_ADDRMODE::ADR_ABSY,3,4);
    cpu_setOp(M6502_INST::INS_ORA,0x01,M6502_ADDRMODE::ADR_PREIDXIND,2,6);
    cpu_setOp(M6502_INST::INS_ORA,0x11,M6502_ADDRMODE::ADR_POSTIDXIND,2,5);

    // PHA:
    cpu_setOp(M6502_INST::INS_PHA,0x48,M6502_ADDRMODE::ADR_IMP,1,3);

    // PHP:
    cpu_setOp(M6502_INST::INS_PHP,0x08,M6502_ADDRMODE::ADR_IMP,1,3);

    // PLA:
    cpu_setOp(M6502_INST::INS_PLA,0x68,M6502_ADDRMODE::ADR_IMP,1,4);

    // PLP:
    cpu_setOp(M6502_INST::INS_PLP,0x28,M6502_ADDRMODE::ADR_IMP,1,4);

    // ROL:
    cpu_setOp(M6502_INST::INS_ROLA,0x2A,M6502_ADDRMODE::ADR_ACC,1,2);
    cpu_setOp(M6502_INST::INS_ROL,0x26,M6502_ADDRMODE::ADR_ZP,2,5);
    cpu_setOp(M6502_INST::INS_ROL,0x36,M6502_ADDRMODE::ADR_ZPX,2,6);
    cpu_setOp(M6502_INST::INS_ROL,0x2E,M6502_ADDRMODE::ADR_ABS,3,6);
    cpu_setOp(M6502_INST::INS_ROL,0x3E,M6502_ADDRMODE::ADR_ABSX,3,7);

    // ROR:
    cpu_setOp(M6502_INST::INS_RORA,0x6A,M6502_ADDRMODE::ADR_ACC,1,2);
    cpu_setOp(M6502_INST::INS_ROR,0x66,M6502_ADDRMODE::ADR_ZP,2,5);
    cpu_setOp(M6502_INST::INS_ROR,0x76,M6502_ADDRMODE::ADR_ZPX,2,6);
    cpu_setOp(M6502_INST::INS_ROR,0x6E,M6502_ADDRMODE::ADR_ABS,3,6);
    cpu_setOp(M6502_INST::INS_ROR,0x7E,M6502_ADDRMODE::ADR_ABSX,3,7);

    // RTI:
    cpu_setOp(M6502_INST::INS_RTI,0x40,M6502_ADDRMODE::ADR_IMP,1,6);

    // RTS:
    cpu_setOp(M6502_INST::INS_RTS,0x60,M6502_ADDRMODE::ADR_IMP,1,6);

    // SBC:
    cpu_setOp(M6502_INST::INS_SBC,0xE9,M6502_ADDRMODE::ADR_IMM,2,2);
    cpu_setOp(M6502_INST::INS_SBC,0xE5,M6502_ADDRMODE::ADR_ZP,2,3);
    cpu_setOp(M6502_INST::INS_SBC,0xF5,M6502_ADDRMODE::ADR_ZPX,2,4);
    cpu_setOp(M6502_INST::INS_SBC,0xED,M6502_ADDRMODE::ADR_ABS,3,4);
    cpu_setOp(M6502_INST::INS_SBC,0xFD,M6502_ADDRMODE::ADR_ABSX,3,4);
    cpu_setOp(M6502_INST::INS_SBC,0xF9,M6502_ADDRMODE::ADR_ABSY,3,4);
    cpu_setOp(M6502_INST::INS_SBC,0xE1,M6502_ADDRMODE::ADR_PREIDXIND,2,6);
    cpu_setOp(M6502_INST::INS_SBC,0xF1,M6502_ADDRMODE::ADR_POSTIDXIND,2,5);

    // SEC:
    cpu_setOp(M6502_INST::INS_SEC,0x38,M6502_ADDRMODE::ADR_IMP,1,2);

    // SED:
    cpu_setOp(M6502_INST::INS_SED,0xF8,M6502_ADDRMODE::ADR_IMP,1,2);

    // SEI:
    cpu_setOp(M6502_INST::INS_SEI,0x78,M6502_ADDRMODE::ADR_IMP,1,2);

    // STA:
    cpu_setOp(M6502_INST::INS_STA,0x85,M6502_ADDRMODE::ADR_ZP,2,3);
    cpu_setOp(M6502_INST::INS_STA,0x95,M6502_ADDRMODE::ADR_ZPX,2,4);
    cpu_setOp(M6502_INST::INS_STA,0x8D,M6502_ADDRMODE::ADR_ABS,3,4);
    cpu_setOp(M6502_INST::INS_STA,0x9D,M6502_ADDRMODE::ADR_ABSX,3,5);
    cpu_setOp(M6502_INST::INS_STA,0x99,M6502_ADDRMODE::ADR_ABSY,3,5);
    cpu_setOp(M6502_INST::INS_STA,0x81,M6502_ADDRMODE::ADR_PREIDXIND,2,6);
    cpu_setOp(M6502_INST::INS_STA,0x91,M6502_ADDRMODE::ADR_POSTIDXIND,2,6);

    // STX:
    cpu_setOp(M6502_INST::INS_STX,0x86,M6502_ADDRMODE::ADR_ZP,2,3);
    cpu_setOp(M6502_INST::INS_STX,0x96,M6502_ADDRMODE::ADR_ZPY,2,4);
    cpu_setOp(M6502_INST::INS_STX,0x8E,M6502_ADDRMODE::ADR_ABS,3,4);

    // STY:
    cpu_setOp(M6502_INST::INS_STY,0x84,M6502_ADDRMODE::ADR_ZP,2,3);
    cpu_setOp(M6502_INST::INS_STY,0x94,M6502_ADDRMODE::ADR_ZPX,2,4);
    cpu_setOp(M6502_INST::INS_STY,0x8C,M6502_ADDRMODE::ADR_ABS,3,4);

    // TAX:
    cpu_setOp(M6502_INST::INS_TAX,0xAA,M6502_ADDRMODE::ADR_IMP,1,2);

    // TAY:
    cpu_setOp(M6502_INST::INS_TAY,0xA8,M6502_ADDRMODE::ADR_IMP,1,2);

    // TSX:
    cpu_setOp(M6502_INST::INS_TSX,0xBA,M6502_ADDRMODE::ADR_IMP,1,2);

    // TXA:
    cpu_setOp(M6502_INST::INS_TXA,0x8A,M6502_ADDRMODE::ADR_IMP,1,2);

    // TXS:
    cpu_setOp(M6502_INST::INS_TXS,0x9A,M6502_ADDRMODE::ADR_IMP,1,2);

    // TYA:
    cpu_setOp(M6502_INST::INS_TYA,0x98,M6502_ADDRMODE::ADR_IMP,1,2);
}

void testOPTABLE() {
    /* extern M6502_OPCODE opdata[256]; */
    static_assert(sizeof(opdata[0])==4,"OpData struct error");
    static_assert(sizeof(opdata)/sizeof(opdata[0])==256,"OpData struct error");
    static_assert((int)M6502_ADDRMODE::_ADR_MAX==14,"error");
    static_assert((int)M6502_INST::_INS_MAX==67,"error");
}
