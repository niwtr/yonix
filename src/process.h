#pragma once
#include "params.h"
#include "yotypes.h"
#include "mmu.h"
#include "x86.h"
#include "params.h"

//上下文
/*
EAX 是"累加器"(accumulator), 它是很多加法乘法指令的缺省寄存器。
ECX 是计数器(counter), 是重复(REP)前缀指令和LOOP指令的内定计数器。
EDX 则总是被用来放整数除法产生的余数。
EBP是"基址指针"(BASE POINTER), 它最经常被用作高级语言函数调用的"框架指针"(frame pointer). 在破解的时候,经常可以看见一个标准的函数起始代码:
  　　push ebp ;保存当前ebp
	　　mov ebp,esp ;EBP设为当前堆栈指针
	  　　sub esp, xxx ;预留xxx字节给函数临时变量.
		　　...
			　　这样一来,EBP 构成了该函数的一个框架, 在EBP上方分别是原来的EBP, 返回地址和参数. EBP下方则是临时变量. 函数返回时作 mov esp,ebp/pop ebp/ret 即可.
ESP 专门用作堆栈指针，被形象地称为栈顶指针，堆栈的顶部是地址小的区域，压入堆栈的数据越多，ESP也就越来越小。在32位平台上，ESP每次减少4字节。
*/
struct context {
	uint edi;		//目标索引寄存器
	uint esi;		//变址寄存器 源索引寄存器
	uint ebx;		//"基地址"(base)寄存器, 在内存寻址时存放基地址
	uint ebp;		//"基址指针"(BASE POINTER), 它最经常被用作高级语言函数调用的"框架指针"(frame pointer). 
	uint eip;		//每次cpu执行都要先读取eip寄存器的值，然后定位eip指向的内存地址，并且读取汇编指令，最后执行
};

//进程可能处的各种状态 
enum procstat {
	SUNUSED, // unused.
	SEMBRYO, // TODO wit?唤醒
	SWAIT, //waiting.
	SSLEEPING, // sleep in high priority
	RUNNABLE, // idle? TODO check
	SRUN, // running.
	SZOMB // zombie.
};

/* WARNING flag borrowed from v6 */
enum procflag {
	SLOAD, // in core.
	SSYS,  //scheduling proc 负责调度的进程
	SLOCK, //proc cannot be swapped.
	SSWAP, //proc is being swapped out.
	STRC,  //proc is being traced. TODO 什么叫trace？
	SWTED // another tracing flag. TODO wit?
};

// Per-CPU state
// BORROWED from xv6
// TODO 这些东西都不太懂。
struct cpu {
	uchar apicid;                // Local APIC ID
	struct context *scheduler;   // swtch() here to enter scheduler
	struct taskstate ts;         // Used by x86 to find stack for interrupt
	struct segdesc gdt[NSEGS];   // x86 global descriptor table
	volatile uint started;       // Has the CPU started?
	int ncli;                    // Depth of pushcli nesting.
	int intena;                  // Were interrupts enabled before pushcli?

								 // Cpu-local storage variables; see below
	struct cpu *cpu;
	struct proc *proc;           // The currently-running process.
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
	int p_chan; // event process is awaiting -> 进程或者是SSLEEPING或者是SWAIT，这个位标志了导致进程睡眠的原因。

				/* WARNING in v6 says int, while xv6 says void * */
	int p_killed; // if non-zero this proc is killed. TODO is this necessary?

	struct file * p_of[P_NOFILE]; //opened files.
	struct inode * p_cdir; // current directory

	char name[16]; //proc name (for debugging) TODO remove if not necessary

				   // for scheduling, borrowed from v6.
				   // p_pri = f(p_nice, p_time, p_pu)
	int p_nice; // nice for scheduling
	int p_pri; // priority, negative is high.
	int p_time; // resident time for schedulint
	int p_pu; // cpu usage for scheduling.
};


extern struct cpu cpus[NCPU]; 先处理单线程问题

//指示当前时刻的CPU状态、进程状态的指针
//
extern struct cpu *cpu asm("%gs:0");       // &cpus[cpunum()]
extern struct proc *proc asm("%gs:4");     // cpus[cpunum()].proc

