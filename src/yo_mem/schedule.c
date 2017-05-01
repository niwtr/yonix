#include "param_yonix.h"
#include "yotypes.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"


//non-private.
void switch_to (struct proc * p)
{
  proc = p;        //设置当前关照的进程（全局变量）。
  switchuvm(p);    //交换用户虚拟内存
  p->p_stat = SRUN;//设置进程状态
  //从这里离开调度器的上下文转入用户进程。
  swtch(&cpu->scheduler, p->p_ctxt);
}

void select_scheme (int schem){
  cpu->scheme = schem;
}
void sched_name(char * name){
  const char * schedname =sched_reftable[cpu->scheme].scheme_method;
  safestrcpy(name, schedname, strlen(schedname)+1);
}

//#include "process.h"
//proc=0表示调度器线程本身。
//调度器scheduler
//CPU在初始化后边调用该函数
//userproc --sched-->schedulerproc--sched-->userproc...
void scheduler(void)
{

	while (true)
    {
      sti(); //允许时间片中断，中断后trap调用yeild()函数返回

      //RR,找到处于READY状态的进程

      //sched_rr();
      if(!sched_reftable[cpu->scheme].scheme()) //sched
        continue;

      //某个时间片中断！pia的一下CPU又回到了这里！！！

      //switch back.
      switchkvm();//FIXME
      //设置当前工作进程为调度器。
      //你会发现这个函数的代码最好写在一个柱面上。：）
      proc = 0;//设置当前关照的进程为调度器。


    }
}



//进程从用户态切换到cpu调度器xv6中的sched
void transform(void)
{

  //执行after过程。
  //1. 重新计算时间片。
  //2. 重新计算动态优先级（如果有的话）

	/*
	最后，调用 swtch 把当前上下文保存在 proc->context 中然后切换到调度器上下文即 cpu->scheduler 中
	*/
	if (proc->p_stat == SRUN)
		panic("sched running");//不shed正在SRUN的进程

	swtch(&proc->p_ctxt, cpu->scheduler);


}



//放弃CUP的所有权——针对时间片到期后
//于是我们需要更新该进程的时间片。
void giveup_cpu(void)
{

	//在所有状态改变的操作中，都需要先获得锁，以保证不会有冲突发生
	proc->p_stat = READY;
  // proc->p_time_slice = SCHED_RR_TIMESLICE;
  sched_reftable[cpu->scheme].after();

	transform();

}


//这个过程通常由中断进入。
void timeslice_yield(){
  if (proc->p_time_slice == ETERNAL)
    return ; // 该进程的时间片是无穷，直接返回。
  proc->p_time_slice -= TIMER_INTERVAL;
  proc->p_avgslp -= 1; //让avgslp递减1个tick

  if(proc->p_time_slice<=0) // 时间片用完了，于是giveup，执行上下文切换到调度器。
    {
      cprintf("\nPID %d Used up its time slice.\n",proc->p_pid);
      giveup_cpu();
    }
  else
    ; // 如果时间片没有用完，直接返回
}





/* sleep a proc on specific event and swtch away. */
void sleep(void * e)
{
  //tell event.
  proc->p_chan=e;
  proc->p_stat=SSLEEPING;
  transform(); //swtch away.
  proc->p_chan=0; // when sched back (return from wakeup), tidy up.
}

void wakeup(void * e)
{
  //find specific proc that is sleep on specific event e (or chan)
  search_through_ptablef(p)
    if(p->p_stat==SSLEEPING && p->p_chan==e)
      p->p_stat=READY;
}







