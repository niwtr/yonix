#include "param_yonix.h"
#include "yotypes.h"
struct context {
	uint edi;		//Ŀ�������Ĵ���
	uint esi;		//��ַ�Ĵ��� Դ�����Ĵ���
	uint ebx;		//"����ַ"(base)�Ĵ���, ���ڴ�Ѱַʱ��Ż���ַ
	uint ebp;		//"��ַָ��"(BASE POINTER), ������������߼����Ժ������õ�"���ָ��"(frame pointer).
	uint eip;		//ÿ��cpuִ�ж�Ҫ�ȶ�ȡeip�Ĵ�����ֵ��Ȼ��λeipָ����ڴ��ַ�����Ҷ�ȡ���ָ����ִ��
};
//���̿��ܴ��ĸ���״̬

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
	SSYS,  //scheduling proc ������ȵĽ���
	SLOCK, //proc cannot be swapped.
	SSWAP, //proc is being swapped out.
	STRC,  //proc is being traced. TODO ʲô��trace��
	SWTED // another tracing flag. TODO wit?
  */
};

// Per-CPU state
// BORROWED from xv6
// TODO ��Щ��������̫����
struct cpu {
	struct context *scheduler;   // swtch() here to enter scheduler
	struct taskstate ts;         // Used by x86 to find stack for interrupt
	struct segdesc gdt[NSEGS];   // x86 global descriptor table
	volatile uint started;       // Has the CPU started?

								 // �ڵ���ǿ�Ʋ������жϷ�������֮ǰ�Ƿ������жϣ���

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
	uint p_size; // size of swappable image (process memory)���̴�С�����ֽ�Ϊ��λ


	struct trapframe *p_tf;//�ж�ǰ��Ϣ���ں�״̬
	char * p_kstack;//bottom of kernel stack
	struct context * p_ctxt; //swtch() here to run proc.������
	void * p_chan; // event process is awaiting -> ���̻�����SSLEEPING������SWAIT�����λ��־�˵��½���˯�ߵ�ԭ��

				/* WARNING in v6 says int, while xv6 says void * */
	int p_killed; // if non-zero this proc is killed. TODO is this necessary?

	struct file * p_of[P_NOFILE]; //opened files.
	struct inode * p_cdir; // current directory

	char p_name[16]; //proc name (for debugging) TODO remove if not necessary

				   // for scheduling, borrowed from v6.
				   // p_pri = f(p_nice, p_time, p_pu)
	int p_nice; // nice for scheduling
	int p_pri; // priority, negative is high.
	int p_time; // resident time for schedulint
	int p_pu; // cpu usage for scheduling.
};


extern struct cpu cpus[1];// �ȴ������߳�����

//ָʾ��ǰʱ�̵�CPU״̬������״̬��ָ��
//
extern struct cpu *cpu asm("%gs:0");       // &cpus[cpunum()]
extern struct proc *proc asm("%gs:4");     // cpus[cpunum()].proc


struct protab {
  struct proc proc[NPROC];
} ;

										   //���̱��ṹ


extern struct protab ptable;
extern struct proc * initproc;


#define search_through_ptablef(name)                            \
  struct proc * name;                                           \
  for(name = ptable.proc;name < &ptable.proc[PROC_NUM]; name++)