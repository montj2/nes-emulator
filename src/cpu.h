enum class IRQ
{
    NONE=0,
    NMI,
    NORMAL
};

void CpuReset();
void CpuTests();

int CpuExecOneInst();
int CpuRunFrame();
void CpuRequestIRQ(enum IRQ);
