// Processor-defined:
#define T_DIVIDE         0      // divide error 除0故障
#define T_DEBUG          1      // debug exception 当进行程序单步调试时，设置标志寄存器eflag的T标志时产生该中断
#define T_NMI            2      // non-maskable interrupt 不可屏蔽中断
#define T_BRKPT          3      // breakpoint 由断点指令int3产生
#define T_OFLOW          4      // overflow eflag的溢出标志OF引起
#define T_BOUND          5      // bounds check 寻址到有效地址以外时引起
#define T_ILLOP          6      // illegal opcode CPU执行时发现一个无效的指令操作码
#define T_DEVICE         7      // device not available 设备不存在，指协处理器(CPU内置的提高浮点运算能力的处理器)
#define T_DBLFLT         8      // double fault 双故障出错 当CPU在调用前一个异常的处理程序而又检测到一个新的异常却又无法串行处理时引发
// #define T_COPROC      9      // reserved (not used since 486) 协处理器段超出出错中断入口点 防止浮点数过大超出数据段的浮点值
#define T_TSS           10      // invalid task switch segment 无效的任务状态段TSS
#define T_SEGNP         11      // segment not present 描述符所指定的段不存在
#define T_STACK         12      // stack exception 堆栈段不存在或寻址越出堆栈段
#define T_GPFLT         13      // general protection fault 没有符合80386保护机制（特权级）的操作引起
#define T_PGFLT         14      // page fault 页错误 页不在内存中
// #define T_RES        15      // reserved 其他intel保留中断入口点
#define T_FPERR         16      // floating point error 
#define T_ALIGN         17      // aligment check 边界对齐检查出错 （内存边界检查时）
#define T_MCHK          18      // machine check
#define T_SIMDERR       19      // SIMD floating point error

// These are arbitrarily chosen, but with care not to overlap
// processor defined exceptions or interrupt vectors.
#define T_SYSCALL       64      // system call
#define T_DEFAULT      500      // catchall

#define T_IRQ0          32      // IRQ 0 corresponds to int T_IRQ时钟中断

#define IRQ_TIMER        0		//时钟中断请求
#define IRQ_KBD          1		//键盘输入设备中断请求
#define IRQ_COM1         4		//串口设备
#define IRQ_PRI			 7		//打印机传输控制请求
#define IRQ_IDE IRQ_IDE0
#define IRQ_IDE0        14		//IDE0传输控制请求
#define IRQ_IDE1		15		//IDE1传输控制请求
#define IRQ_ERROR       19		//错误请求
#define IRQ_SPURIOUS    31

