

#include "param_yonix.h"
#include "yotypes.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
int nextpid = 1;

//fork return
extern void forkret(void);
//trap return
extern void trapret(void);


struct protab ptable;
struct proc * initproc;
struct proc * proc;

/* �ҵ�һ�����еĲۣ������µĽ���PCB��Ϊ�������ں˿ռ䡣
 * �µĽ��̵�״̬ΪSEMBRYO��
 * �µĽ��̽������ں�ջ�������ں�ջ����̬Ϊ��
 *   +------------+
 *   |  kstack +  | <------+----------------------+
 *   | kstacksize |        |                      |
 *   +------------+        |      trapframe       |
 *                         |                      |
 *                         +----------------------+
 *                         |     trapret ptr      |
 *                         |                      |
 *                         +----------------------+
 *                         |       context        |
 *                         |                      |
 *                         +----------------------+
 *                         |                      |
 *                         |                      |
 *                         |                      |
 *                         |                      |
 *                         |                      |
 *                         |                      |
 *                         |                      |
 *                         |                      |
 *                         |                      |
 *                         |                      |
 *                         |                      |
 *                         |                      |
 *                         |                      |
 *   +------------+        |                      |
 *   |   kstack   |        |                      |
 *   |   bottom   | <------+----------------------+
 *   +------------+
 * �����Ľ��̳�Ϊѿ�߽��̡��������̵�������ȫ��Ϊ��ʼֵ0.
 */

/* ���¼���ʱ��Ƭ�� */
/* ��ϵͳת��schedule������scheme����ʱ�򣬸ú������뱻���á�*/
void recalc_timeslice (void)
{
  search_through_ptablef(p){
    if(p->p_stat != SUNUSED)
      sched_reftable[cpu->scheme].timeslice(p);
  }
}


static struct proc* procalloc(void)
{

	//ָʾջ��λ��
	char * sp;

	//�ҵ��ڴ��д���SUNUSED״̬�Ľ���
    search_through_ptablef(p)
	{
    //cprintf("searching for unused.:%d,%d\n", p->p_stat, ittt);
		//�����ҵ��ˣ�������״̬��ΪSEMBRYO
		//�����ӽ��̵�pidֵ
		if (p->p_stat == SUNUSED)
		{
      //����״̬��pid����
			p->p_stat = SEMBRYO;
			p->p_pid = nextpid++;
			//nextpid++;

      //����ʱ��Ƭ���á�
      //TODO ��RR����н��̵�ʱ��Ƭ���ö���ͬ�ȵģ��������ǿ��԰����ŵ�������ǡ���
      //�������ĵ����㷨�������Ҫ���ݽ��̵��������ı�ʱ��Ƭ���á������Ƽ�������ת�Ƶ�fork���档

      p->p_nice = 0; //��ʼp_nice����0������ͨ��ϵͳ���������ġ�
      p->p_spri = STATIC_PRI(p->p_nice);
      p->p_creatime = ticks;
      p->p_avgslp = 0; //average sleep time �ĳ�ֵΪ0.
      sched_reftable[cpu->scheme].timeslice(p);//����ʱ��Ƭ
      p->p_dpri = DYNAMIC_PRI(p->p_spri, BONUS(p->p_avgslp));

			//Ϊ���½��̷����ں�ջ�ռ�
			p->p_kstack = kalloc();	//�ں�ջ���亯��

			//������ʧ��,����״̬�Ļ�SUNUSED
			if (p->p_kstack==0)
			{
				p->p_stat = SUNUSED;
				return 0;
			}

			//�޸�ջ��λ��
			sp = p->p_kstack + K_STACKSZ;

			//Ϊtarpframe����λ�ã���
			sp = sp - sizeof(*p->p_tf);
			p->p_tf = (struct trapframe*) sp;


			// Set up new context to start executing at forkret,
			// which returns to trapret.
      //TODO ������ô�����ģ�
			sp = sp - 4;
			*(uint*)sp = (uint)trapret;

			sp = sp - sizeof (*p->p_ctxt);
			p->p_ctxt = (struct context*)sp;
      //TODO require API
			memset(p->p_ctxt, 0, sizeof (*p->p_ctxt));
      //�����½��Ľ���
			return p;

		}

	}
	//��������һȦ��û���ҵ�SUNUESD�Ľ��̣���ֱ�ӷ���
	return 0;
}


/* ������ǰ���̷����������ڴ��ռ䡣 */
int procgrow(int n){
  uint sz;
  sz = proc->p_size;
  if(n > 0){ //���������ڴ�
    //TODO require api allocuvm
    sz=allocuvm(proc->p_page,sz,sz+n);
    if(sz==0)
      return -1;
  } else if(n<0){ //��С�����ڴ�
    //TODO require api deallocuvm
    sz = deallocuvm(proc->p_page, sz, sz+n, proc->p_pid);
    if(sz==0)
      return -1;
  }

  proc->p_size = sz;
  switchuvm(proc); //FIXME ����ʲô��
  return 0;
}




/* fork�����������ӽ��̡� */
int fork(void)
{

	int i, pid;
	struct proc *np;

	//Ϊ���̷����ں˿ռ�
	np = procalloc();

	if (np == 0)
	{
		return -1;
	}


	//���������̵��û���ַ�ռ䣨�û������ڴ棩���µ������ڴ洦��
    //TODO add api
  //FIXME xv6
	np->p_page = copyuvm(proc->p_page, proc->p_size, np->p_pid);
	if (np->p_page == 0)	//��������ʧ�ܣ����ͷŽ�����ռ�ڴ��ռ�
    //TODO �Ƿ����԰����δ�����������һ�������ĺ�����
	{
    //TODO add api
		kfree(np->p_kstack);
		np->p_kstack = 0;
		np->p_stat = SUNUSED;
		return -1;
	}
	//�����ϲ�����˳�����У��������̵ĸ�����Ϣ����һ�ݵ��ӽ�����
	np->p_size = proc->p_size; //���ý��̵ľ�����С��
	np->p_prt = proc;          //�Ѹ���������Ϊ��ǰ�����ߡ�
	*(np->p_tf) = *(proc->p_tf);//����trapframe��

	//���ռĴ���eax��ֵ �Ա�fork�󷵻ظ��ӽ��̵�ֵΪ0
	np->p_tf->eax = 0;
  np->p_ctxt->eip = (uint)forkret;
  //TODO ������ô�����ģ�����û�п���������ʲô��˼��

	//���������̴򿪵������ļ�,���临��һ�ݵ��ӽ���
	for (i = 0; i < P_NOFILE; i++)
		if (proc->p_of[i])
			np->p_of[i] = filedup(proc->p_of[i]);

	//��������������Ŀ¼
  //TODO require api
  //FIXME xv6
	np->p_cdir = idup(proc->p_cdir);

	//���������̵Ľ�����
  //TODO require api
  //FIXME xv6
	safestrcpy(np->p_name, proc->p_name, sizeof(proc->p_name));

	//�����ӽ��̵�pidֵ���Ա㷵�ظ�������
	pid = np->p_pid;
  np->p_procp = 1; // this is, indeed, a proc.

	//�޸��½��ӽ��̵�״̬
	np->p_stat = READY;


	//���ӽ��̵�pid���ظ�������

	return pid;
}

void exit(void){


  //initproc������proc�ĸ������ý��̲������˳���
  if(proc == initproc)
    panic("init exiting");

  // close all opened files.
  int fd;
  for(fd=0;fd< P_NOFILE;fd++){
    if(proc->p_of[fd]){
      fileclose(proc->p_of[fd]);
      proc->p_of[fd]=0;
    }
  }


  //�ͷŵ�ǰ��������Ŀ¼
  //TODO �ļ�ϵͳ
  //TODO require api
  begin_op();
  iput(proc->p_cdir);
  end_op();
  proc->p_cdir=0;


  wakeup(proc->p_prt); //���Ѹ����̡�

  //TODO ��Щ����̫����
  search_through_ptablef(p){
    if(p->p_prt==proc){
      p->p_prt=initproc;
      if(p->p_stat==SZOMB)
        wakeup(initproc);
    }
  }

  proc->p_stat=SZOMB;
  transform();
  panic("zombie exit.");

}

//mark the proc to kill.
//TODO ʲôʱ���������̱�����ɱ����
int kill(int pid)
{

  search_through_ptablef(p){
    if(p->p_pid==pid){
      p->p_killed=1;
      if(p->p_stat==SSLEEPING)
        p->p_stat=READY;
      return 0;
    }
  }return -1; //not found
}




void forkret(void){
  static int firstproc=1;
  if(firstproc){
    firstproc=0;
    iinit(ROOTDEV);//FIXME
    initlog(ROOTDEV);//FIXME
  }
}



void dbg_procdump(void)
{
  static char * status [] =
    {
#define X(name) [name] #name,
      __STAT_LOV__
#undef X
    };

  char * state;
  uint pc[10];
  search_through_ptablef(p){
    if(p->p_stat==SUNUSED)
      continue;
    if(p->p_stat>=0 && p->p_stat<sizeof(status))
      state=status[p->p_stat];
    else
      state="UNKNOWN";
    cprintf("%d %s %s", p->p_pid, state, p->p_name);
    if(p->p_stat==SSLEEPING){
      // track back the sleeping proc.
      getcallerpcs((uint*)p->p_ctxt->ebp+2, pc);
      int i;
      for (i=0;i<10 && pc[i]!=0;i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }

}

void dbg_lstprocs(void){
  static char * status [] =
    {
#define X(name) [name] #name,
      __STAT_LOV__
#undef X
    };
  char * state;
  cprintf("\n");
  search_through_ptablef(p){
    if(p->p_stat == SUNUSED)
      continue;
    if(p->p_stat >= 0 && p->p_stat <sizeof(status))
      state=status[p->p_stat];
    else
      state="UNKNOWN";

    cprintf("%d %s %s %s ts:%d avgslp:%d dpri:%d\n", p->p_pid, p->p_name, state, p->p_procp?"proc":"thread", p->p_time_slice, p->p_avgslp, p->p_dpri);

  }
}










//��һ���û�������
void userinit(void)
{
	struct proc *p;
	extern char _binary_initcode_start[], _binary_initcode_size[];//���Ǹ����ģ�
	p = procalloc();	//��ҳ���з���һ��proc,����ʼ������״̬ SUNUSED->SEMBRYO
						//��һ���û�����
	initproc = p;
	//Ϊ�����������ڴ�ҳ�� ��ȡһ���µĶ���ҳ�����������ں�����ӳ��
	p->p_page = setupkvm();
	if (!p->p_page)
		panic("userinit: setupkvm failed!");

	//��ʼ���û�����������ַ�ռ�
	inituvm(p->p_page, _binary_initcode_start, (int) _binary_initcode_size);

	//trapframe������
	memset(p->p_tf, 0, sizeof(*p->p_tf));
	p->p_tf->cs = (SEG_UCODE << 3) | DPL_USER;//%cs �Ĵ���������һ����ѡ������ ָ���� SEG_UCODE ��������Ȩ�� DPL_USER�������û�ģʽ�����ں�ģʽ��
	p->p_tf->ds = (SEG_UDATA << 3) | DPL_USER;//ds,es,ss��ѡ����ָ���� SEG_UDATA ��������Ȩ�� DPL_USER
	p->p_tf->es = p->p_tf->ds;
	p->p_tf->ss = p->p_tf->ds;
	p->p_tf->eflags = FL_IF;// FL_IF λ������Ϊ����Ӳ���ж�
	p->p_tf->esp = PGSIZE;//��Ϊ���̵�������Ч�����ڴ�
	p->p_tf->eip = 0;  // beginning of initcode.Sָ����ʼ�����������ڵ㣬����ַ0

	p->p_size = PGSIZE;
	safestrcpy(p->p_name, "initcode", sizeof(p->p_name));
	//��������Ŀ¼Ϊ��Ŀ¼��/��
	p->p_cdir = namei("/");
  p->p_procp = 1;//this is indeed a proc.
	//������״̬����ΪREADY
	p->p_stat = READY;
  p->p_ctxt->eip = (uint)forkret;//���÷��ص�ַΪforkret
}


//wait�����ȴ��ӽ���ִ������exit,������������ID��
int wait(void)
{
	int pid;		//�ӽ���pid
  int have_kid;
	while (true)
	{
    have_kid=0;
		//���������б����ҵ���ǰ�����Ƿ����ӽ���
		search_through_ptablef(p)
		{

			if (p->p_prt == proc)
			{
        have_kid=1;
				//�����ӽ�����һ����ʬ���̡����ӽ����Ѿ�����������������Ϊ̫æ��û�еȴ�����������
				//һ�������ڵ���exit���������Լ���������ʱ������ʵ����û�������ı����٣�
				//��������һ����Ϊ��ʬ���̣�Zombie�������ݽṹ��ϵͳ����exit������������ ʹ�����˳���
				//��Ҳ�������ڽ�һ�������Ľ��̱���һ����ʬ���̣������ܽ�����ȫ���٣�
				if (p->p_stat == SZOMB)
				{
					//�����ý�����Ϣ
					pid = p->p_pid;
					kfree(p->p_kstack);	//�ͷ��ں�ջ
          p->p_kstack=0;
					freeuvm(p->p_page, proc->p_pid);	//�ͷ������ڴ�
					p->p_pid = 0;
					p->p_prt = 0;		//��������Ϊ0
					p->p_name[0] = 0;
					p->p_killed = 0;
					p->p_stat = SUNUSED;
					return pid;
				}
      }
    }
    if(!have_kid || proc->p_killed){
      return -1;
    }
    //���ҵ�һ�����ǽ�ʬ���̵��ӽ��̣��򸸽������ߣ��ȴ��ӽ��̱�Ϊ��ʬ����
    sleep(proc);
  }
}




/* create a light-weight proc. */
int lwp_create (void * task, void * arg, void * stack, int stksz){
  int i, pid;
  struct proc * lwp;
  lwp = procalloc();
  if(lwp == 0) return -1;

  lwp->p_page = proc->p_page;
  lwp->p_size = proc->p_size;

  if(lwp->p_page == 0) return -1;

  if (proc->p_procp){
    lwp->p_prt = proc;
  } else {
    lwp->p_prt = proc->p_prt;
  }
  *(lwp->p_tf) = *(proc->p_tf);//����trapframe��
  //����Ӧ���������������C֧��ֱ�ӿ���һ���ṹ�壿

	//���ռĴ���eax��ֵ �Ա�fork�󷵻ظ��ӽ��̵�ֵΪ0
	lwp->p_tf->eax = 0;
  lwp->p_ctxt->eip=(uint)forkret;
  lwp->p_tf->eip = (int)task;//��������Ҫִ��task�����ݡ�
  lwp->p_stack = (int)stack;
  lwp->p_tf->esp = lwp->p_stack + stksz - 4 ; //TODO Ū������4092�Ǹ�������������
  *((int *)(lwp->p_tf->esp)) = (int)arg; // push the argument
  *((int *)(lwp->p_tf->esp-4))=0xFFFFFFFF;  //return addr.
  lwp->p_tf->esp -=4;

	//���������̴򿪵������ļ�,���临��һ�ݵ��ӽ���
	for (i = 0; i < P_NOFILE; i++)
		if (proc->p_of[i])
			lwp->p_of[i] = filedup(proc->p_of[i]);

  lwp->p_cdir = idup(proc->p_cdir);

  safestrcpy(lwp->p_name, proc->p_name, sizeof(proc->p_name));
	//�����ӽ��̵�pidֵ���Ա㷵�ظ�������
	pid = lwp->p_pid;
  lwp->p_procp = 0; // this is,not a proc.

	//�޸��½��ӽ��̵�״̬
	lwp->p_stat = READY;
	//���ӽ��̵�pid���ظ�������

	return pid;
}
/* �൱��fork��wait��Ψһ�Ĳ�ͬ�ǣ�lwp��stack���ᱻ�������������á�*/
int
lwp_join(void **stack)
{


  int have_kid, pid;

  for(;;){
    // Scan through table looking for zombie children.
    have_kid = 0;
    search_through_ptablef(p){
      // wait for the child thread, but not the child process
      if(p->p_prt != proc || p->p_procp != 0)
        continue;
      have_kid = 1;
      if(p->p_stat == SZOMB){
        // Found one.
        pid = p->p_pid;
        kfree(p->p_kstack);
        p->p_kstack = 0;
        p->p_stat = SUNUSED;
        p->p_pid = 0;
        p->p_prt = 0;
        p->p_name[0] = 0;
        p->p_killed = 0;
        *(int*)stack = p->p_stack; //mark the stack.
        return pid;
      }
    }

    // No point waiting if we don't have any children thread.
    if(!have_kid || proc->p_killed)
      return -1;


    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc);  //DOC: wait-sleep
  }
}



/* 提供了动态调转功能 */
void dynamic_sstore(void * stack, struct trapframe * tf, int stksz){
  memmove((char *)stack,(char *) proc->p_stack, stksz);
  memmove((char *)tf, (char *)proc->p_tf, sizeof(*tf));
}

void dynamic_restart(void * stack, struct trapframe * tf, int stksz){
  memmove((char *)proc->p_stack, (char *)stack, stksz);
  memmove((char *)proc->p_tf, (char *)tf, sizeof(*tf));
}
