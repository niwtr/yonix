#include "process.h"
#include "lock.h"

//���̱�ṹ
struct protab {
	struct spinlock lock;		//������
	struct proc proc[PROC_NUM]; //���̶���
};

int nextpid = 1;
protab ptable;

//fork�������
extern void frk_ret(void);
//trap�������
extern void trp_ret(void);

//�ӿ��н������ҵ�һ������
static struct proc* allocproc(void)
{
	//�½���
	struct proc* p;
	//ָʾջ��λ��
	char * stackpos;		

	//��û�����
	acquirelock(&ptable.lock);

	//�ҵ��ڴ��д���SUNUSED״̬�Ľ���
	for (p = ptable.proc; p < &ptable.proc[PROC_NUM]; p++)
	{
		//����ҵ��ˣ�����״̬��ΪSEMBRYO
		//�����ӽ��̵�pidֵ
		if (p->p_stat == SUNUSED)
		{
			p->p_stat = SEMBRYO;
			p->p_pid = nextpid;
			nextpid++;

			//�ͷ���
			releaselock(&ptable.lock);
			
			//Ϊ���½��̷����ڴ�ջ�ռ�
			p->p_kstack = kalloc();	//�ں�ջ���亯��
			//������ʧ��,����״̬�Ļ�SUNUSED
			if (!p->p_kstack)
			{
				p->p_stat = SUNUSED;
				return 0;
			}

			//�޸�ջ��λ��
			stackpos = p->p_kstack + K_STACKSZ;
			
			//Ϊtarpframe����λ�ã���
			stackpos = stackpos - sizeof(*p->p_tf);//??
			p->p_tf = (struct trapframe*) stackpos;	//�ض�λtrapframe

			//��������=_=||
			// Set up new context to start executing at forkret,
			// which returns to trapret.
			stackpos = stackpos - 4;
			*(uint*)stackpos = (uint)trapret;

			stackpos = stackpos - sizeof (*p->p_ctxt);
			p->p_ctxt = (struct context*)stackpos;
			memset(p->p_ctxt, 0, sizeof (*p->p_ctxt));
			p->p_ctxt->eip = (uint)forkret;

			//�����½��Ľ���
			return p;

		}
	}
	//�������һȦ��û���ҵ�SUNUESD�Ľ��̣����ͷ�����ֱ�ӷ���
	releaselock(&ptable.lock);
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
	//np->p_cdir = copyuvm(, proc->sz);
	if (np->p_cdir == 0)	//������ʧ�ܣ����ͷŽ�����ռ�ڴ�ռ䣿��
	{
		//kfree(np->kstack);
		np->p_kstack = 0;
		np->p_stat = SUNUSED;
		return -1;
	}

	//�����ϲ�����˳�����У��������̵ĸ�����Ϣ����һ�ݵ��ӽ�����
	np->p_size = proc->p_size;
	np->p_prt = proc;
	*(np->p_tf) = *(proc->p_tf);//??Ӧ��Ӧ��������

	//��ռĴ���eax��ֵ �Ա�fork�󷵻ظ��ӽ��̵�ֵΪ0
	np->p_tf->eax = 0;

	//�������������򿪵������ļ�,���临��һ�ݵ��ӽ���
	for (i = 0; i < P_NOFILE; i++)
	{
		if (proc->p_of[i])
		{
			;//np->p_of[i] = filedup(proc->p_of[i]);
		}
	}

	//��������������Ŀ¼
	//np->p_cdir = dirdup(proc->p_cdir);

	//���������̵Ľ�����
	//safestrcpy(np->name, proc->name, sizeof(proc->name);

	//�����ӽ��̵�pidֵ���Ա㷵�ظ�������
	pid = np->p_pid;

	//�޸��½��ӽ��̵�״̬��Ϊ��֤���⣬ʹ����
	acquirelock(&ptable.lock);
	np->p_stat = SRUN;
	releaselock(&ptable.lock);

	//���ӽ��̵�pid���ظ�������
	return pid;
}

//process��ʼ��������ʼ�����̵Ļ�����
void procinit(void)
{
	initlock(&ptable.lock, "ptable");
}

//semaphore_wait
//slkΪ�����������е������Ա�֤���������޷�����signal
//Ϊʲô��������������
void sem_wait(void *w_queue, struct spinlock *slk)
{
	if (proc == 0)
		painc("sleep:process is not exit!");
	if (slk == 0)
		painc("sleep:the caller don't have lock!");

	//���̴��ڣ��ҵ������ĺ���������slk
	//��slk!=&ptable.lock,�򵱽��̳���ptable.lock�󣬱�ɰ�ȫ�ͷ�slk
	//��Ϊsignal�������ڳ�����������²���ִ��
	if (slk != &ptable.lock)
	{	
		acquirelock(&ptable.lock);
		releaselock(slk);
	}
	//��slk==&ptable.lock
	//��ʱ��Ҫ����� ptable.lock Ȼ���ְ�����Ϊ slk �ͷŵ�ʱ������������
	//���Բ�ִ������if����������

	//ʹ���̽�������״̬
	proc->p_chan = w_queue;
	proc->p_stat = SSLEEPING;
	//����shed(),���û�̬�л���cpu������״̬
	sched();

	//���㣿��
	proc->p_chan = 0;

	//�������������ȵ����������֮ǰ����
	if (slk != &ptable.lock)
	{
		releaselock(&ptable.lock);
		acquirelock(slk);
	}
}


//semaphore_broadcast
//�������д���˯��״̬�Ľ���
void sem_broadcast(void *w_queue)
{
	acquirelock(&ptable.lock);
	wakeup(w_queue);
	releaselock(&ptable.lock);
}

//�ڲ�����
static void wakeup(void *w_queue)
{
	struct proc *pro;

	//�������̱��������н���
	for (pro = ptable.proc; pro < &ptable.proc[PROC_NUM]; pro++)//�ɲ�������pro<PROC_NUM??
	{
		if (pro->p_stat == SSLEEPING && pro->p_chan == w_queue)
			pro->p_stat = SRUN;
	}
}
