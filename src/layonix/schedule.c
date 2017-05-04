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
  proc = p;
  switchuvm(p);
  p->p_stat = SRUN;
  swtch(&cpu->scheduler, p->p_ctxt);
}

void select_scheme (int schem){
  cpu->scheme = schem;
  sched_reftable[cpu->scheme].init();
}
void sched_name(char * name){
  const char * schedname =sched_reftable[cpu->scheme].scheme_method;
  safestrcpy(name, schedname, strlen(schedname)+1);
}

void scheduler(void)
{

	while (true)
    {
      sti();
      if(!sched_reftable[cpu->scheme].scheme()) //sched
          continue;
      //switch back.
      switchkvm();//FIXME
      proc = 0;
    }
}


void transform(void)
{

	if (proc->p_stat == SRUN)
		panic("sched running");

	swtch(&proc->p_ctxt, cpu->scheduler);
}




void giveup_cpu(void)
{
	proc->p_stat = READY;

  sched_reftable[cpu->scheme].enqueue(proc);
  sched_reftable[cpu->scheme].after();

	transform();

}


void timeslice_yield(){
  if (proc->p_time_slice == ETERNAL)
    return ;
  proc->p_time_slice -= TIMER_INTERVAL;
  proc->p_avgslp = MAX(-50, proc->p_avgslp-1);
  if(proc->p_time_slice<=0)
      giveup_cpu();
}





/* sleep a proc on specific event and swtch away. */
void sleep(void * e)
{
  //tell event.
  uint tick0 = ticks;
  proc->p_chan=e;
  proc->p_stat=SSLEEPING;
  transform(); //swtch away.
  proc->p_chan=0; // when sched back (return from wakeup), tidy up.
  proc->p_avgslp = MIN(proc->p_avgslp+(tick0-ticks), MAX_AVGSLP);
}

void wakeup(void * e)
{
  //find specific proc that is sleep on specific event e (or chan)
  search_through_ptablef(p)
    if(p->p_stat==SSLEEPING && p->p_chan==e)
      {
        p->p_stat=READY;
        sched_reftable[cpu->scheme].enqueue(p);
      }
}







