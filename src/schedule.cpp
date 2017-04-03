#include "process.h"
#include "lock.h"


//TODO proc=0中的0可能是指调度器本身。
//调度器scheduler
//每个CPU在初始化后边调用该函数
//从调度器状态切换到用户态？内核？
void scheduler(void)
{
	struct proc *p;
	while (true)
	{
		sti();//允许时间片中断，中断后trap调用yeild()函数

			  //轮转查询,找到处于READY状态的进程
		for (p = ptable.proc; p < &ptable.proc[PROC_NUM]; p++)//初始化的时候初始化了PROC_NUM个进程吗？？
		{
			if (p->p_stat == READY)
			{
				//切换其状态为RUNNING
				proc = p;
				switchvm(p);//交换虚拟内存
				p->p_stat = SRUN;
				swtch(&cpu->scheduler, p->p_ctxt);
				switchvm();//??

				proc = 0;//??
			}
		}

	}
}


//进程从用户态切换到cpu调度器xv6中的sched
void transform(void)
{

  int inter_enable;
	/*
	该函数检查了两次状态，这里的状态表明由于进程此时持有锁，所以 CPU 应该是在中断关闭的情况下运行的。
	最后，调用 swtch 把当前上下文保存在 proc->context 中然后切换到调度器上下文即 cpu->scheduler 中
	*/
	if (proc->p_stat == SRUN)
		panic("sched running");
	inter_enable = cpu->intena;
	swtch(&proc->context, cpu->scheduler);
	//切换上下文，该上下文是在scheduler中切换时保存的
  cpu->intena=intena;
}

//放弃CUP的所有权——针对时间片到期后
void giveup_cpu(void)
{
	//在所有状态改变的操作中，都需要先获得锁，以保证不会有冲突发生
	proc->p_stat = READY;
	transform();
}


/* sleep a proc on specific event and swtch away. */
void sleep(void * e)
{
  //TODO 为什么要检查是否为0？
  //EXPLAIN：可能0是指调度器本身。
  if(proc==0)
    panic("sleep");
  //tell event.
  proc->p_chan=e;
  proc->p_stat=SSLEEPING;
  transform(); //swtch away.
  proc->p_chan=0; // when sched back (return from wakeup), tidy up.
}

void wakeup(void * e)
{
  //find specific proc that is sleep on specific event e (or chan)
  struct proc *p;
  search_through_ptablef(p)
    if(p->p_stat==SSLEEPING && p->p_chan==e)
      p->p_stat=READY;
}

