/* IRQ Type */
enum IRQ_TYPE {
    IRQ_NONE=0,
    IRQ_NMI,
    IRQ_NORMAL
};

void cpu_reset();
int cpu_exec();
int cpu_frame();
void cpu_requestIRQ(enum IRQ_TYPE);
