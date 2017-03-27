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




struct context {
  uint edi; // 32bit pointers
  uint esi; // 32bit pointers
  uint ebi; // base pointer reg
  uint eip; // TODO wit?
  uint ebp; // base pointer reg
};
enum procstat {
  SUNUSED, // unused.
  SEMBRYO, // TODO wit?
  SWAIT , //waiting.
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
}
struct proc {
  enum procstat p_stat;
  enum procflag p_flag;


  int p_pid;
  struct proc * p_prt; // pointer to parent

  paget * p_page; // page table
  uint p_size; // size of swappable image (process memory)


  struct trapframe *p_tf;
  char * p_kstack;//bottom of kernel stack
  struct context * p_ctxt; //swtch() here to run proc.
  int p_chan; // event process is awaiting -> 进程或者是SSLEEPING或者是SWAIT，这个位标志了导致进程睡眠的原因。

  /* WARNING in v6 says int, while xv6 says void * */
  int p_killed; // if non-zero this proc is killed. TODO is this necessary?

  struct file * p_of [P_NOFILE]; //opened files.
  struct inode * p_cdir; // current directory

  char name [16]; //proc name (for debugging) TODO remove if not necessary

  // for scheduling, borrowed from v6.
  // p_pri = f(p_nice, p_time, p_pu)
  int p_nice; // nice for scheduling
  int p_pri; // priority, negative is high.
  int p_time; // resident time for schedulint
  int p_pu; // cpu usage for scheduling.
};



