/* IRQ Type */
enum IRQ_TYPE {
    IRQ_NONE=0,
    IRQ_NMI,
    IRQ_NORMAL
};

void cpu_reset();
void cpu_initTable();
void cpu_testTable();
int cpu_exec();
int cpu_frame();
void cpu_requestIRQ(enum IRQ_TYPE);
