#include "process.h"
#include "lock.h"

int nextpid = 1;

//fork�������
extern void frk_ret(void);
//trap�������
extern void trp_ret(void);

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
	np = allocproc();
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
  for(fd=0;fd< NOFILE;fd++){
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




