#define P_NOFILE 16 //maximum open files per proc. xv6:NOFILE
#define TV_ENTRIES 256 // x86的中断描述符表
#define true 1
#define false 0

#define CPU_NUM 8//cup的最大数目（可能不需要，如果使用单处理器的话）
#define PROC_NUM 64  //进程的最大数目
#define K_STACKSZ 4096  //每个进程的内核栈大小
