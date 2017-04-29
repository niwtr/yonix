#define P_NOFILE 16 //maximum open files per proc. xv6:NOFILE
#define TV_ENTRIES 256 // x86的中断描述符表
#define true 1
#define false 0


#define PROC_NUM 64  //进程的最大数目
#define K_STACKSZ 4096  //每个进程的内核栈大小

#define MAX_PID 0xfff   // 进程最大pid号，页队列标识符限制

#define SLABSIZE 32     // 小内存池块大小
#define SWAPSIZE 0x8000000  //128MB swap area
#define SLOTSIZE SWAPSIZE/PGSIZE  // number of slots
#define PR_FIFO 1
#define PR_SCND 2
#define PR_ALGO PR_FIFO