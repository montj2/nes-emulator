enum class IRQ
{
    NONE=0,
    NMI,
    BRK,
	RST
};

namespace interrupt
{
	// global functions
	void clear();
	void request(const IRQ irqType);

	bool pending();
	IRQ type();
}

namespace cpu
{
	// global functions
	void reset();
	void start();

	int nextInstruction();
}