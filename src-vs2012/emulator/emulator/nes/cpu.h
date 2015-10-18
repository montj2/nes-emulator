// status flags
enum PSW
{
	F_CARRY=0x1,
	F_ZERO=0x2,
	F_INTERRUPT_OFF=0x4,
	F_BCD=0x8,
	F_DECIMAL=F_BCD,
	F_BREAK=0x10,
	F_RESERVED=0x20, // not used (always set)
	F_OVERFLOW=0x40,
	F_NEGATIVE=0x80,
	F_SIGN=F_NEGATIVE,
	F__NV=F_NEGATIVE|F_OVERFLOW
};

// irq types
enum IRQ
{
    NONE=0,
    NMI,
    BRK,
	RST
};

namespace interrupt
{
	// global functions
	void clear(const IRQ type);
	void request(const IRQ type);

	bool pending();
	IRQ current();
}

namespace cpu
{
	// global functions
	void reset();
	void start();

	int nextInstruction();
}