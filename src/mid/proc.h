#include "param_yonix.h"
#include "yotypes.h"
struct context {
	uint edi;		//目标索引寄存器
	uint esi;		//变址寄存器 源索引寄存器
	uint ebx;		//"基地址"(base)寄存器, 在内存寻址时存放基地址
	uint ebp;		//"基址指针"(BASE POINTER), 它最经常被用作高级语言函数调用的"框架指针"(frame pointer).
	uint eip;		//每次cpu执行都要先读取eip寄存器的值，然后定位eip指向的内存地址，并且读取汇编指令，最后执行
};
//进程可能处的各种状态

#define __STAT_LOV__ \
X(SUNUSED)                                \
X(SEMBRYO)                                \
X(SWAIT)                                  \
X(SSLEEPING)                              \
X(READY)                                  \
X(SRUN)                                   \
X(SZOMB)

enum procstat {
#define X(name) name,
  __STAT_LOV__
#undef X
};
#define __FLAG_LOV__ \
X(SLOAD)\
X(SSYS)\
X(SLOCK)\
X(SSWAP)\
X(STRC)\
X(SWTED)
/* WARNING flag borrowed from v6 */
enum procflag {
#define X(name) name,
  __FLAG_LOV__
#undef X
  /*
	SLOAD, // in core.
	SSYS,  //scheduling proc 负责调度的进程
	SLOCK, //proc cannot be swapped.
	SSWAP, //proc is being swapped out.
	STRC,  //proc is being traced. TODO 什么叫trace？
	SWTED // another tracing flag. TODO wit?
  */
};

enum sched_method {
  SCHEME_FIFO,
  SCHEME_RR,
  SCHEME_PRI,
};



// Per-CPU state
// BORROWED from xv6
// TODO 这些东西都不太懂。
struct cpu {
	struct context *scheduler;   // swtch() here to enter scheduler
	struct taskstate ts;         // Used by x86 to find stack for interrupt
	struct segdesc gdt[NSEGS];   // x86 global descriptor table
	volatile uint started;       // Has the CPU started?

								 // 在调用强制不允许中断发生函数之前是否允许中断？？

								 // Cpu-local storage variables; see below
	struct cpu *cpu;
	struct proc *proc;           // The currently-running process.
  enum sched_method scheme ; //当前执行的调度算法索引

};


//PCB
struct proc {
	enum procstat p_stat;
	enum procflag p_flag;


	int p_pid;
	struct proc * p_prt; // pointer to parent

	paget * p_page; // page table
	uint p_size; // size of swappable image (process memory)进程大小，以字节为单位


	struct trapframe *p_tf;//中断前信息，内核状态
	char * p_kstack;//bottom of kernel stack
	struct context * p_ctxt; //swtch() here to run proc.上下文
	void * p_chan; // event process is awaiting -> 进程或者是SSLEEPING或者是SWAIT，这个位标志了导致进程睡眠的原因。

				/* WARNING in v6 says int, while xv6 says void * */
	int p_killed; // if non-zero this proc is killed. TODO is this necessary?

	struct file * p_of[P_NOFILE]; //opened files.
	struct inode * p_cdir; // current directory

	char p_name[16]; //proc name (for debugging) TODO remove if not necessary

				   // for scheduling, borrowed from v6.
				   // p_pri = f(p_nice, p_time, p_pu)


  int p_time_slice; // time slice per proc.

	int p_nice; // nice for scheduling
	int p_spri; // static priority, negative is high.
  int p_dpri; // dynamic priority.
	//int p_time; // resident time for schedule int
	//int p_pu; // cpu usage for scheduling.
};


extern struct cpu cpus[1];// 先处理单线程问题

//指示当前时刻的CPU状态、进程状态的指针
//
extern struct cpu *cpu asm("%gs:0");       // &cpus[cpunum()]
extern struct proc *proc asm("%gs:4");     // cpus[cpunum()].proc


struct protab {
  struct proc proc[NPROC];
} ;

// scheme: alias for sched_method

struct sched_refstruct {
  void (*scheme) (void);
  void (*after) (void); //当进程时间片用完的时候，这个方法将会被调用。
  void  (*timeslice) (struct proc *); //用于计算时间片的函数
} ;

extern struct sched_refstruct sched_reftable[SCHEME_NUMS];//调度器算法查找表




										   //进程表结构


extern struct protab ptable;
extern struct proc * initproc;


#define search_through_ptablef(name)                            \
  struct proc * name;int ittt=0;                                          \
  for(name = ptable.proc;name < &ptable.proc[PROC_NUM]; name++,ittt++)

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) > (b) ? (a) : (b))

#define STATIC_PRI(nice) 120+nice
#define TIME_SLICE(stpri) ((stpri>120)?\
                           MAX((140-stpri)*5, MIN_TIMESLICE):\
                           MAX((140-stpri)*20, MIN_TIMESLICE))
#define DYNAMIC_PRI(stpri, bns) MAX(100,MIN(stpri-bns+5, 139))

