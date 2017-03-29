#include "process.h"
#include "lock.h"

int nextpid = 1;

//fork其余进程
extern void frk_ret(void);
//trap其余进程
extern void trp_ret(void);

//从空闲进程中找到一个进程
static struct proc* allocproc(void)
{
	//新进程
	struct proc* p;
	//指示栈顶位置
	char * stackpos;		

	//获得互斥锁
	acquirelock(&ptable.lock);

	//找到内存中处于SUNUSED状态的进程
	for (p = ptable.proc; p < &ptable.proc[PROC_NUM]; p++)
	{
		//如果找到了，则将其状态改为SEMBRYO
		//且增加进程的pid值
		if (p->p_stat == SUNUSED)
		{
			p->p_stat = SEMBRYO;
			p->p_pid = nextpid;
			nextpid++;

			//释放锁
			releaselock(&ptable.lock);
			
			//为该新进程分配内存栈空间
			p->p_kstack = kalloc();	//内核栈分配函数
			//若分配失败,将其状态改回SUNUSED
			if (!p->p_kstack)
			{
				p->p_stat = SUNUSED;
				return 0;
			}

			//修改栈顶位置
			stackpos = p->p_kstack + K_STACKSZ;
			
			//为tarpframe留出位置？？
			stackpos = stackpos - sizeof(*p->p_tf);//??
			p->p_tf = (struct trapframe*) stackpos;	//重定位trapframe

			//看不懂了=_=||
			// Set up new context to start executing at forkret,
			// which returns to trapret.
			stackpos = stackpos - 4;
			*(uint*)stackpos = (uint)trapret;

			stackpos = stackpos - sizeof (*p->p_ctxt);
			p->p_ctxt = (struct context*)stackpos;
			memset(p->p_ctxt, 0, sizeof (*p->p_ctxt));
			p->p_ctxt->eip = (uint)forkret;

			//返回新建的进程
			return p;

		}
	}
	//如果找了一圈都没有找到SUNUESD的进程，则释放锁，直接返回
	releaselock(&ptable.lock);
	return 0;

}

//fork函数
int fork(void)
{
	int i, pid;
	struct proc *np;

	//为进程分配内核空间
	np = allocproc();
	if (np == 0)
	{
		return -1;
	}

	//将父进程的状态拷贝一份 vm.c 
	//np->p_cdir = copyuvm(, proc->sz);
	if (np->p_cdir == 0)	//虚存分配失败，则释放进程所占内存空间？？
	{
		//kfree(np->kstack);
		np->p_kstack = 0;
		np->p_stat = SUNUSED;
		return -1;
	}

	//若以上操作都顺利进行，将父进程的各种信息复制一份到子进程上
	np->p_size = proc->p_size;
	np->p_prt = proc;
	*(np->p_tf) = *(proc->p_tf);//??应不应该有括号

	//清空寄存器eax的值 以便fork后返回给子进程的值为0
	np->p_tf->eax = 0;

	//遍历父进程锁打开的所有文件,将其复制一份到子进程
	for (i = 0; i < P_NOFILE; i++)
	{
		if (proc->p_of[i])
		{
			;//np->p_of[i] = filedup(proc->p_of[i]);
		}
	}

	//拷贝父进程所在目录
	//np->p_cdir = dirdup(proc->p_cdir);

	//拷贝父进程的进程名
	//safestrcpy(np->name, proc->name, sizeof(proc->name);

	//保存子进程的pid值，以便返回给父进程
	pid = np->p_pid;

	//修改新建子进程的状态，为保证互斥，使用锁
	acquirelock(&ptable.lock);
	np->p_stat = READY;
	releaselock(&ptable.lock);

	//将子进程的pid返回给父进程
	return pid;
}

//process初始化——初始化进程的互斥锁
void procinit(void)
{
	initlock(&ptable.lock, "ptable");
}
