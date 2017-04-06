

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

/* �ҵ�һ�����еĲۣ������µĽ���PCB��Ϊ������ں˿ռ䡣
 * �µĽ��̵�״̬ΪSEMBRYO��
 * �µĽ��̽������ں�ջ������ں�ջ����̬Ϊ��
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
 * �����Ľ��̳�Ϊѿ�߽��̡�������̵�������ȫ��Ϊ��ʼֵ0.
 */


static struct proc* procalloc(void)
{

	//ָʾջ��λ��
	char * sp;

	//�ҵ��ڴ��д���SUNUSED״̬�Ľ���
    search_through_ptablef(p)
	{
		//����ҵ��ˣ�����״̬��ΪSEMBRYO
		//�����ӽ��̵�pidֵ
		if (p->p_stat == SUNUSED)
		{
			p->p_stat = SEMBRYO;
			p->p_pid = nextpid;
			nextpid++;


			//Ϊ���½��̷����ں�ջ�ռ�
			p->p_kstack = kalloc();	//�ں�ջ���亯��
			//������ʧ��,����״̬�Ļ�SUNUSED
			if (!p->p_kstack)
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
	//�������һȦ��û���ҵ�SUNUESD�Ľ��̣���ֱ�ӷ���

	return 0;
}


/* ������ǰ���̷���������ڴ�ռ䡣 */
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
    sz = deallocuvm(proc->p_page, sz, sz+n);
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
	np->p_page = copyuvm(proc->p_page, proc->p_size);
	if (np->p_page == 0)	//������ʧ�ܣ����ͷŽ�����ռ�ڴ�ռ�
    //TODO �Ƿ���԰���δ����������һ�������ĺ�����
	{
    //TODO add api
		kfree(np->p_kstack);
		np->p_kstack = 0;
		np->p_stat = SUNUSED;
		return -1;
	}

	//�����ϲ�����˳�����У��������̵ĸ�����Ϣ����һ�ݵ��ӽ�����
	np->p_size = proc->p_size; //���ý��̵ľ����С��
	np->p_prt = proc;          //�Ѹ���������Ϊ��ǰ�����ߡ�
	*(np->p_tf) = *(proc->p_tf);//����trapframe��

	//��ռĴ���eax��ֵ �Ա�fork�󷵻ظ��ӽ��̵�ֵΪ0
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
//TODO ʲôʱ��������̱�����ɱ����
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



//��һ���û�������
void userinit(void)
{
	struct proc *p;
	extern char _binary_initcode_start[], _binary_initcode_size[];//���Ǹ���ģ�
	p = procalloc();	//��ҳ���з���һ��proc,����ʼ������״̬ SUNUSED->SEMBRYO
						//��һ���û�����
	initproc = p;
	//Ϊ����������ڴ�ҳ�� ��ȡһ���µĶ���ҳ�����������ں�����ӳ��
	p->p_page = setupkvm();
	if (!p->p_page)
		panic("userinit: setupkvm failed!");

	//��ʼ���û����������ַ�ռ�
	inituvm(p->p_page, _binary_initcode_start, (int) _binary_initcode_size);

	//trapframe������
	memset(p->p_tf, 0, sizeof(*p->p_tf));
	p->p_tf->cs = (SEG_UCODE << 3) | DPL_USER;//%cs �Ĵ���������һ����ѡ������ ָ��� SEG_UCODE ��������Ȩ�� DPL_USER�������û�ģʽ�����ں�ģʽ��
	p->p_tf->ds = (SEG_UDATA << 3) | DPL_USER;//ds,es,ss��ѡ����ָ��� SEG_UDATA ��������Ȩ�� DPL_USER
	p->p_tf->es = p->p_tf->ds;
	p->p_tf->ss = p->p_tf->ds;
	p->p_tf->eflags = FL_IF;// FL_IF λ������Ϊ����Ӳ���ж�
	p->p_tf->esp = PGSIZE;//��Ϊ���̵������Ч�����ڴ�
	p->p_tf->eip = 0;  // beginning of initcode.Sָ���ʼ���������ڵ㣬����ַ0

	p->p_size = PGSIZE;
	safestrcpy(p->p_name, "initcode", sizeof(p->p_name));
	//��������Ŀ¼Ϊ��Ŀ¼��/��
	p->p_cdir = namei("/");
	//������״̬����ΪREADY
	p->p_stat = READY;
  p->p_ctxt->eip = (uint)forkret;//���÷��ص�ַΪforkret
}

//wait�����ȴ��ӽ���ִ�����exit,�����������ID��
int wait(void)
{
	int pid;		//�ӽ���pid

	while (true)
	{
		//���������б����ҵ���ǰ�����Ƿ����ӽ���
		search_through_ptablef(p)
		{
			if (p->p_prt == proc)
			{

				//�����ӽ�����һ����ʬ���̡����ӽ����Ѿ�����������������Ϊ̫æ��û�еȴ�����������
				//һ�������ڵ���exit��������Լ���������ʱ����ʵ����û�������ı����٣�
				//��������һ����Ϊ��ʬ���̣�Zombie�������ݽṹ��ϵͳ����exit������������ ʹ�����˳���
				//��Ҳ�������ڽ�һ�������Ľ��̱��һ����ʬ���̣������ܽ�����ȫ���٣�
				if (p->p_stat == SZOMB)
				{
					//�����ý�����Ϣ
					pid = p->p_pid;
					kfree(p->p_kstack);	//�ͷ��ں�ջ
					freevm(p->p_page);	//�ͷ������ڴ�
					p->p_pid = 0;
					p->p_prt = 0;		//��������Ϊ0
					p->p_name[0] = 0;
					p->p_killed = 0;
					p->p_stat = 0;
					p->p_stat = SUNUSED;
					return pid;
				}
				//���ҵ�һ�����ǽ�ʬ���̵��ӽ��̣��򸸽������ߣ��ȴ��ӽ��̱�Ϊ��ʬ����
				sleep(proc);
			}
		}
	}
}
