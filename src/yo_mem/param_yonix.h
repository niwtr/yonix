#define P_NOFILE 16 //maximum open files per proc. xv6:NOFILE
#define TV_ENTRIES 256 // x86的中断描述符表
#define true 1
#define false 0

#define SCHEME_NUMS 3 //支持的schedule算法的数目
#define PRI_NUM 140 //优先级的数目
#define PROC_NUM 64  //进程的最大数目
#define K_STACKSZ 4096  //每个进程的内核栈大小
#define TIMER_INTERVAL 10 // 10ms一次时钟中断。
#define ETERNAL -255 // forever
#define MAX_RT_PRI 100
#define SCHED_RR_TIMESLICE 10 // 设置RR调度的时间片为100ms
#define SCHED_FIFO_TIMESLICE ETERNAL
#define MIN_TIMESLICE 100 //最小的时间片就是RR的时间片。
#define MAX_NICE 19
#define MIN_NICE 20
#define MAX_AVGSLP 100 // 100ticks

#define private static

#define MAX_PID 0xfff   // 进程最大pid号，页队列标识符限制

#define MEMTOP 0xffffffff
#define SLABSIZE 32     // 小内存池块大小
#define SWAPSIZE 0x8000000  //128MB swap area
#define SLOTSIZE SWAPSIZE/PGSIZE  // number of slots
#define PR_FIFO 1
#define PR_SCND 2
#define PR_ALGO PR_FIFO

