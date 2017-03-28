#pragma once
#include "params.h"
#include "yotypes.h"
#include "mmu.h"
#include "x86.h"
#include "params.h"

//������
/*
EAX ��"�ۼ���"(accumulator), ���Ǻܶ�ӷ��˷�ָ���ȱʡ�Ĵ�����
ECX �Ǽ�����(counter), ���ظ�(REP)ǰ׺ָ���LOOPָ����ڶ���������
EDX �����Ǳ���������������������������
EBP��"��ַָ��"(BASE POINTER), ������������߼����Ժ������õ�"���ָ��"(frame pointer). ���ƽ��ʱ��,�������Կ���һ����׼�ĺ�����ʼ����:
  ����push ebp ;���浱ǰebp
	����mov ebp,esp ;EBP��Ϊ��ǰ��ջָ��
	  ����sub esp, xxx ;Ԥ��xxx�ֽڸ�������ʱ����.
		����...
			��������һ��,EBP �����˸ú�����һ�����, ��EBP�Ϸ��ֱ���ԭ����EBP, ���ص�ַ�Ͳ���. EBP�·�������ʱ����. ��������ʱ�� mov esp,ebp/pop ebp/ret ����.
ESP ר��������ջָ�룬������س�Ϊջ��ָ�룬��ջ�Ķ����ǵ�ַС������ѹ���ջ������Խ�࣬ESPҲ��Խ��ԽС����32λƽ̨�ϣ�ESPÿ�μ���4�ֽڡ�
*/
struct context {
	uint edi;		//Ŀ�������Ĵ���
	uint esi;		//��ַ�Ĵ��� Դ�����Ĵ���
	uint ebx;		//"����ַ"(base)�Ĵ���, ���ڴ�Ѱַʱ��Ż���ַ
	uint ebp;		//"��ַָ��"(BASE POINTER), ������������߼����Ժ������õ�"���ָ��"(frame pointer). 
	uint eip;		//ÿ��cpuִ�ж�Ҫ�ȶ�ȡeip�Ĵ�����ֵ��Ȼ��λeipָ����ڴ��ַ�����Ҷ�ȡ���ָ����ִ��
};

//���̿��ܴ��ĸ���״̬ 
enum procstat {
	SUNUSED, // unused.
	SEMBRYO, // TODO wit?����
	SWAIT, //waiting.
	SSLEEPING, // sleep in high priority
	RUNNABLE, // idle? TODO check
	SRUN, // running.
	SZOMB // zombie.
};

/* WARNING flag borrowed from v6 */
enum procflag {
	SLOAD, // in core.
	SSYS,  //scheduling proc ������ȵĽ���
	SLOCK, //proc cannot be swapped.
	SSWAP, //proc is being swapped out.
	STRC,  //proc is being traced. TODO ʲô��trace��
	SWTED // another tracing flag. TODO wit?
};

// Per-CPU state
// BORROWED from xv6
// TODO ��Щ��������̫����
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
	uint p_size; // size of swappable image (process memory)���̴�С�����ֽ�Ϊ��λ


	struct trapframe *p_tf;//�ж�ǰ��Ϣ���ں�״̬
	char * p_kstack;//bottom of kernel stack
	struct context * p_ctxt; //swtch() here to run proc.������
	int p_chan; // event process is awaiting -> ���̻�����SSLEEPING������SWAIT�����λ��־�˵��½���˯�ߵ�ԭ��

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


extern struct cpu cpus[NCPU]; �ȴ����߳�����

//ָʾ��ǰʱ�̵�CPU״̬������״̬��ָ��
//
extern struct cpu *cpu asm("%gs:0");       // &cpus[cpunum()]
extern struct proc *proc asm("%gs:4");     // cpus[cpunum()].proc

