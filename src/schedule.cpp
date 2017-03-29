#include "process.h"
#include "lock.h"

//semaphore_wait
//slk为调用者所持有的锁，以保证其他进程无法调用signal
//为什么会有两个锁？？
void sem_wait(void *w_queue, struct spinlock *slk)
{
	if (proc == 0)
		painc("sleep:process is not exit!");
	if (slk == 0)
		painc("sleep:the caller don't have lock!");

	//进程存在，且调用它的函数持有锁slk
	//若slk!=&ptable.lock,则当进程持有ptable.lock后，便可安全释放slk
	//因为signal函数需在持有锁的情况下才能执行
	if (slk != &ptable.lock)
	{
		acquirelock(&ptable.lock);
		releaselock(slk);
	}
	//若slk==&ptable.lock
	//此时若要求持有 ptable.lock 然后又把它作为 slk 释放的时候会出现死锁。
	//所以不执行上述if条件里的语句

	//使进程进入休眠状态
	proc->p_chan = w_queue;
	proc->p_stat = SSLEEPING;
	//调用transform(),从用户态切换到cpu调度器状态
	transform();

	//清零？？
	proc->p_chan = 0;

	//针对两个锁不想等的情况：换回之前的锁
	if (slk != &ptable.lock)
	{
		releaselock(&ptable.lock);
		acquirelock(slk);
	}
}


//semaphore_broadcast
//唤醒所有处于睡眠状态的进程
void sem_broadcast(void *w_queue)
{
	acquirelock(&ptable.lock);
	wakeup(w_queue);
	releaselock(&ptable.lock);
}

//内部函数
static void wakeup(void *w_queue)
{
	struct proc *pro;

	//遍历进程表，唤醒所有进程
	for (pro = ptable.proc; pro < &ptable.proc[PROC_NUM]; pro++)//可不可以是pro<PROC_NUM??
	{
		if (pro->p_stat == SSLEEPING && pro->p_chan == w_queue)
			pro->p_stat = READY;
	}
}

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
			  //获得锁，以防止多进程同时执行此处
		acquirelock(&ptable.lock);
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
		releaselock(&ptable.lock);//释放锁

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
	if (!ishold(&ptable.lock))				//判断是否有锁
		panic("sched ptable.lock");
	if (cpu->ncli != 1)						//Depth of pushcli nesting =1意味着什么嘞？
		panic("sched locks");
	if (proc->p_stat == SRUN)
		panic("sched running");
	if (readeflags()&FL_IF)					//读取中断标识
		panic("sched interruptible");
	inter_enable = cpu->intena;
	swtch(&proc->context, cpu->scheduler);
	//切换上下文，该上下文是在scheduler中切换时保存的
	cpu->intena = inter_enable;
}

//放弃CUP的所有权——针对时间片到期后
void giveup_cpu(void)
{
	//在所有状态改变的操作中，都需要先获得锁，以保证不会有冲突发生
	acquirelock(&ptable.lock);
	proc->p_stat = READY;
	transform();
	releaselock(&ptable.lock);
}
