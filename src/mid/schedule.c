#include "param_yonix.h"
#include "yotypes.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"



private void switch_to (struct proc * p)
{
  switchuvm(p);    //交换用户虚拟内存
  proc = p;        //设置当前关照的进程（全局变量）。
  p->p_stat = SRUN;//设置进程状态
  //从这里离开调度器的上下文转入用户进程。
  swtch(&cpu->scheduler, p->p_ctxt);
}


void sched_fifo (void)
{

  if(proc && proc->p_stat==READY)
    switch_to(proc);

  //find proc with smallest creatime.
  struct proc * min = 0;
  search_through_ptablef(p){
    if(p->p_stat == READY){
      if(min==0)
        min=p;
      else
        if(min->p_creatime > p->p_creatime)
          min = p;
    }
  }
  if(min)
    switch_to(min);
  /*
  static struct proc * p = ptable.proc ;
  while(p<&ptable.proc[PROC_NUM]){
    if(p->p_stat != READY){
      p++;
      continue;
    }
    switch_to(p);
    p++;
    return ;
  }
  p=ptable.proc; // wind back.
  */
}
void sched_fifo_after(void)
{
  ; //do nothing.
}
void sched_fifo_timeslice(struct proc * p)
{
  p->p_time_slice = SCHED_FIFO_TIMESLICE; //forever.
}
/*
 * 优先级调度。找到优先级最大的进程进行调度。
 * 静态优先级使用nice来计算：nice+120
 * 每个进程都被赋予不同的时间片。
 * 时间片的计算用静态优先级。
 */
void sched_priority(void)
{
  struct proc * p_primax = 0 ;
  search_through_ptablef(p){
    if(p->p_stat == READY) {
      if(p_primax == 0)
        p_primax = p;
      else
        if(p_primax->p_dpri > p->p_dpri) // 找到了一个优先级更高的。注意dpri越低，优先级越高。
          p_primax = p;
    }
  }
  if(p_primax) //找到了进程 to swtich
    switch_to(p_primax);
}
void sched_priority_timeslice(struct proc *p)
{

  p->p_spri = STATIC_PRI(p->p_nice);
  p->p_time_slice = TIME_SLICE(p->p_spri);
}

void sched_priority_after(void)
{
  sched_priority_timeslice(proc);
  proc->p_dpri = DYNAMIC_PRI(proc->p_spri, BONUS(proc->p_avgslp));
  cprintf("pid %d dpri %d\n", proc->p_pid, proc->p_dpri);
}

//RR调度。
void sched_rr (void)
{
  static struct proc * p=ptable.proc;
  while(p<&ptable.proc[PROC_NUM]){
    if(p->p_stat != READY){
      p++;
      continue;
    }
    switch_to(p);
    p++;
    return ;
  }
  p=ptable.proc; // wind back.
}

void sched_rr_timeslice(struct proc * p){
  p->p_time_slice= SCHED_RR_TIMESLICE;
}

void sched_rr_after(void){
  sched_rr_timeslice(proc);
}

struct sched_refstruct sched_reftable[SCHEME_NUMS]={
  [SCHEME_FIFO]= {sched_fifo, sched_fifo_after, sched_fifo_timeslice},
  [SCHEME_RR]  = {sched_rr, sched_rr_after, sched_rr_timeslice},
  [SCHEME_PRI] = {sched_priority, sched_priority_after, sched_priority_timeslice}

};


void select_scheme (int schem){
  cpu->scheme = schem;
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
      sched_reftable[cpu->scheme].scheme(); //sched

      //某个时间片中断！pia的一下CPU又回到了这里！！！

      //switch back.
      switchkvm();//FIXME
      //设置当前工作进程为调度器。
      //你会发现这个函数的代码最好写在一个柱面上。：）
      proc = 0;//设置当前关照的进程为调度器。


    }
}



//被废弃不用的scheduler，这里留作展览。
void scheduler1(void)
{

	while (true)
	{
		sti(); //允许时间片中断，中断后trap调用yeild()函数返回

		//RR,找到处于READY状态的进程
		search_through_ptablef(p)
		{
			if (p->p_stat == READY)
			{
				//切换其状态为RUNNING

				switchuvm(p);    //交换用户虚拟内存
        proc = p;        //设置当前关照的进程（全局变量）。
				p->p_stat = SRUN;//设置进程状态
        //从这里离开调度器的上下文转入用户进程。
				swtch(&cpu->scheduler, p->p_ctxt);
        //某个时间片中断！pia的一下CPU又回到了这里！！！
				switchkvm();//FIXME
        //设置当前工作进程为调度器。
        //你会发现这个函数的代码最好写在一个柱面上。：）
				proc = 0;//设置当前关照的进程为调度器。

			}
		}

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







