#include "process.h"
#include "lock.h"

int nextpid = 1;

//fork return
extern void forkret(void);
//trap return
extern void trapret(void);

//�ӿ��н������ҵ�һ������
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


			//Ϊ���½��̷����ڴ�ջ�ռ�
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
			sp = sp - 4;
			*(uint*)sp = (uint)trapret;

			sp = sp - sizeof (*p->p_ctxt);
			p->p_ctxt = (struct context*)sp;
			memset(p->p_ctxt, 0, sizeof (*p->p_ctxt));
			p->p_ctxt->eip = (uint)forkret;

			//�����½��Ľ���
			return p;

/*
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
 *                         |                      |          +------------+
 *                         +----------------------+--------->|eip=forkret |
 *                         |                      |          +------------+
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
 */
      //TODO Ϊʲôtrapret���������������Ժ��������

		}

	}
	//�������һȦ��û���ҵ�SUNUESD�Ľ��̣���ֱ�ӷ���

	return 0;
}

int procgrow(int n){
  uint sz;
  sz = proc->sz;
  if(n > 0){
    //TODO require api allocuvm
    sz=allocuvm(proc->pgdir,sz,sz+n);
    if(sz==0)
      return -1;
  } else if(n<0){
    //TODO require api deallocuvm
    sz = deallocuvm(proc->pgdir, sz, sz+n);
    if(sz==0)
      return -1;
  }
  proc->sz = sz;
  switchuvm(proc);
  return 0;
}



//fork����
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

	//�������̵�״̬����һ�� vm.c
    //TODO add api
	np->p_cdir = copyuvm(, proc->sz);
	if (np->p_cdir == 0)	//������ʧ�ܣ����ͷŽ�����ռ�ڴ�ռ䣿��
	{
    //TODO add api
		kfree(np->kstack);
		np->p_kstack = 0;
		np->p_stat = SUNUSED;
		return -1;
	}

	//�����ϲ�����˳�����У��������̵ĸ�����Ϣ����һ�ݵ��ӽ�����
	np->p_size = proc->p_size;
	np->p_prt = proc;
	*(np->p_tf) = *(proc->p_tf);

	//��ռĴ���eax��ֵ �Ա�fork�󷵻ظ��ӽ��̵�ֵΪ0
	np->p_tf->eax = 0;
  //TODO ������ô�����ģ�����û�п���������ʲô��˼��

	//�������������򿪵������ļ�,���临��һ�ݵ��ӽ���
	for (i = 0; i < P_NOFILE; i++)
		if (proc->p_of[i])
			np->p_of[i] = filedup(proc->p_of[i]);

	//��������������Ŀ¼
  //TODO require api
	np->p_cdir = dirdup(proc->p_cdir);

	//���������̵Ľ�����
  //TODO require api
	safestrcpy(np->name, proc->name, sizeof(proc->name));

	//�����ӽ��̵�pidֵ���Ա㷵�ظ�������
	pid = np->p_pid;

	//�޸��½��ӽ��̵�״̬
	np->p_stat = READY;


	//���ӽ��̵�pid���ظ�������
	return pid;
}

void exit(void){


  //initproc������proc�ĸ�����
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


  //TODO Ū����
  //LOG���
  begin_op();
  //fs.c
  iput(proc->p_cdir);
  end_op();
  proc->p_cdir=0;


  wakeup(proc->p_prt);

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
      p->killed=1;
      if(p->state==SSLEEPING)
        p->state=READY;
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
    }
  int i;
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
	extern char biInitcodeStart[], biInitcodeSz[];//���Ǹ���ģ�
	p = procalloc();	//��ҳ���з���һ��proc,����ʼ������״̬ SUNUSED->SEMBRYO
						//��һ���û�����
	initcode = p;
	//Ϊ����������ڴ�ҳ�� ��ȡһ���µĶ���ҳ���������ں�����ӳ��
	p->p_page = setupkvm();
	if (!p->p_page)
		panic("userinit: setupkvm failed!");

	//��ʼ���û����������ַ�ռ�
	inituvm(p->p_cdir, biInitcodeStart, (int)biInitcodeSz);

	//trapframe������
	memset(p->tf, 0, sizeof(*p->tf));
	p->tf->cs = (SEG_UCODE << 3) | DPL_USER;//%cs �Ĵ���������һ����ѡ������ ָ��� SEG_UCODE ��������Ȩ�� DPL_USER�������û�ģʽ�����ں�ģʽ��
	p->tf->ds = (SEG_UDATA << 3) | DPL_USER;//ds,es,ss��ѡ����ָ��� SEG_UDATA ��������Ȩ�� DPL_USER
	p->tf->es = p->tf->ds;
	p->tf->ss = p->tf->ds;
	p->tf->eflags = FL_IF;// FL_IF λ������Ϊ����Ӳ���ж�
	p->tf->esp = PGSIZE;//��Ϊ���̵������Ч�����ڴ�
	p->tf->eip = 0;  // beginning of initcode.Sָ���ʼ���������ڵ㣬����ַ0

	p->p_size = PGSIZE;
	safestrcpy(p->name, "initcode", sizeof(p->name));
	//��������Ŀ¼Ϊ��Ŀ¼��/��
	p->p_cdir = namei("/");
	//������״̬����ΪREADY
	p->p_stat = READY;
}

//wait�����ȴ��ӽ���ִ�����exit,�����������ID��
int wait(void)
{
	int pid;		//�ӽ���pid
	bool isparent = false;	//�жϸý����Ƿ��Ǹ�����

	while (true)
	{
		//���������б��ҵ���ǰ�����Ƿ����ӽ���
		search_through_ptablef(p)
		{
			if (p->parent == proc)
			{
				isparent = true;
				//�����ӽ�����һ����ʬ���̡����ӽ����Ѿ�����������������Ϊ̫æ��û�еȴ�����������
				//һ�������ڵ���exit��������Լ���������ʱ����ʵ����û�������ı����٣�
				//��������һ����Ϊ��ʬ���̣�Zombie�������ݽṹ��ϵͳ����exit������������ ʹ�����˳���
				//��Ҳ�������ڽ�һ�������Ľ��̱��һ����ʬ���̣������ܽ�����ȫ���٣�
				if (p->p_stat == SZOMB)
				{
					//����ý�����Ϣ
					pid = p->p_pid;
					kfree(p->p_kstack);	//�ͷ��ں�ջ
					freevm(p->p_page);	//�ͷ������ڴ�
					p->p_pid = 0;
					p->p_prt = 0;		//��������Ϊ0
					p->name[0] = 0;
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

