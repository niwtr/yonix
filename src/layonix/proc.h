#include "param_yonix.h"
#include "param.h"
#include "yotypes.h"
#include "queue_netbsd.h"
struct context {
	uint edi;		//Ŀ�������Ĵ���
	uint esi;		//��ַ�Ĵ��� Դ�����Ĵ���
	uint ebx;		//"����ַ"(base)�Ĵ���, ���ڴ�Ѱַʱ���Ż���ַ
	uint ebp;		//"��ַָ��"(BASE POINTER), ������������߼����Ժ������õ�"����ָ��"(frame pointer).
	uint eip;		//ÿ��cpuִ�ж�Ҫ�ȶ�ȡeip�Ĵ�����ֵ��Ȼ����λeipָ�����ڴ���ַ�����Ҷ�ȡ����ָ�����ִ��
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
	SSYS,  //scheduling proc �������ȵĽ���
	SLOCK, //proc cannot be swapped.
	SSWAP, //proc is being swapped out.
	STRC,  //proc is being traced. TODO ʲô��trace��
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
  enum sched_method scheme ; //��ǰִ�еĵ����㷨����

};


//PCB
struct proc {
	enum procstat p_stat;
	enum procflag p_flag;


	int p_pid;
  int p_procp ;
	struct proc * p_prt; // pointer to parent

	paget * p_page; // page table
	uint p_size; // size of swappable image (process memory)���̴�С�����ֽ�Ϊ��λ


	struct trapframe *p_tf;//�ж�ǰ��Ϣ���ں�״̬
	char * p_kstack;//bottom of kernel stack
  int p_stack; // �û�̬��ջ
	struct context * p_ctxt; //swtch() here to run proc.������
	void * p_chan; // event process is awaiting -> ���̻�����SSLEEPING������SWAIT������λ��־�˵��½���˯�ߵ�ԭ����

				/* WARNING in v6 says int, while xv6 says void * */
	int p_killed; // if non-zero this proc is killed. TODO is this necessary?

	struct file * p_of[P_NOFILE]; //opened files.
	struct inode * p_cdir; // current directory

	char p_name[16]; //proc name (for debugging) TODO remove if not necessary

				   // for scheduling, borrowed from v6.
				   // p_pri = f(p_nice, p_time, p_pu)


  int p_time_slice; // time slice per proc.
  int p_creatime; //creation time.
	int p_nice; // nice for scheduling
	int p_spri; // static priority, negative is high.
  int p_dpri; // dynamic priority.
  int p_avgslp; // average sleep time, measured in ticks
	//int p_time; // resident time for schedule int
	//int p_pu; // cpu usage for scheduling.
};





extern struct cpu cpus[1];// �ȴ������߳�����

//ָʾ��ǰʱ�̵�CPU״̬������״̬��ָ��
//
extern struct cpu *cpu asm("%gs:0");       // &cpus[cpunum()]
// extern struct proc *proc asm("%gs:4");     // cpus[cpunum()].proc
extern struct proc *proc;


struct protab {
  struct proc proc[NPROC];
} ;

// scheme: alias for sched_method
// �������࣬�������������ı��ţ����֣�
// ��һ��������������ζ������������
struct sched_class {
  enum sched_method scheme_num;
  const char * scheme_method;
  int (*scheme) (void);
  void (*after) (void); //������ʱ��Ƭ������ʱ���������������ᱻ���á�
  void  (*timeslice) (struct proc *); //���ڼ���ʱ��Ƭ�ĺ���
  void (*init) (void);
  void (*enqueue)(struct proc *);
};


//��scheduler_class�ﶨ�塣
extern struct sched_class sched_reftable[SCHEME_NUMS];







extern struct protab ptable;
extern int nextpid;
extern struct proc * initproc;


#define search_through_ptablef(name)                            \
  struct proc * name;                                          \
  for(name = ptable.proc;name < &ptable.proc[PROC_NUM]; name++)

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))


/* for priority scheduler */
#define STATIC_PRI(nice) MAX_RT_PRI+(nice)+20 // static pri range from 115 (nice = -15) to 125 (nice = 15)
#define TIME_SLICE(stpri) (((stpri)>120)?                     \
                           MAX((PRI_NUM-(stpri))*5, MIN_TIMESLICE): \
                           MAX((PRI_NUM-(stpri))*20, MIN_TIMESLICE))

#define BONUS(avgslp) (avgslp)/10 // simplifyed bonus calculation. bonus range from -5 to 10.
#define DYNAMIC_PRI(stpri, bns) MAX(MAX_RT_PRI, MIN((stpri)-(bns)+5, PRI_NUM-1)) //dpri range from 100 to 139

typedef Q_HEAD(empty_slot_entry_list, slot_entry) es_head;
typedef Q_ENTRY(slot_entry) s_entry;

struct slot_entry
{
  struct proc * slotptr;
  s_entry lnk;
};



es_head esqueue; // 空闲slot队列
typedef Q_HEAD(ready_list, slot_entry) rdy_qhead;


rdy_qhead rdyqueue; // ready队列


