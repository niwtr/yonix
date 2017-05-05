#include "param_yonix.h"
#include "yotypes.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"

/* switch to the proc specified. */
void switch_to (struct proc * p)
{
  /* set current in-cpu proc to p*/
  proc = p;
  
  switchuvm(p);
  p->p_stat = SRUN;
  /* switch the kernel-state context. */
  swtch(&cpu->scheduler, p->p_ctxt);
}

/* choose the schedule method (scheme) */
void select_scheme (int schem){
  cpu->scheme = schem;
  /* call the init process. */
  sched_reftable[cpu->scheme].init();
}

/* get the schedule method name. */
void sched_name(char * name){
  const char * schedname =sched_reftable[cpu->scheme].scheme_method;
  safestrcpy(name, schedname, strlen(schedname)+1);
}


/* the body of scheduler.
 * note there is only one scheduler in kernel
 * but it can be assigned to different schedule method (scheme)
 * the scheme is stored in a array for dynamic selection. */
void scheduler(void)
{

	while (true)
    {
      sti(); /* allow interruption. */
      if(!sched_reftable[cpu->scheme].scheme()) //sched
          continue;
      /* switch back here in scheduler. */
      switchkvm();//FIXME
      /* set the current proc to null */
      proc = 0;
    }
}

/* check the status and call switch() */
void transform(void)
{

	if (proc->p_stat == SRUN)
		panic("sched running");

	swtch(&proc->p_ctxt, cpu->scheduler);
}



/* once this function is called the current proc is
 * doomed to be sched away. we call the enqueue()
 * to move the proc to ready queue and call after()
 * to update the timeslice (if in need) */
void giveup_cpu(void)
{
	proc->p_stat = READY;
  sched_reftable[cpu->scheme].enqueue(proc);
  sched_reftable[cpu->scheme].after();
	transform(); /* scheduling away, good luck. */
}

/* in each time interrupt the function is called
 * it checks its timeslice remain and chk wether
 * it is time-out. the avgslp slot is also updated
 * which is used for dynamic priority scheduling. */
void timeslice_yield(){
  /* we never sched away unterminated proc in fifo. */
  if (proc->p_time_slice == ETERNAL)
    return ;
  /* reduce the timeslice */
  proc->p_time_slice -= TIMER_INTERVAL;
  /* update the average sleep time. */
  proc->p_avgslp = MAX(-50, proc->p_avgslp-1);
  /* check the proc is(or not) timeout. */
  if(proc->p_time_slice<=0)
      giveup_cpu();
}





/* sleep a proc on specific event and sched away.
 * the event s of which the proc is sleep on is called "chan". */
void sleep(void * e)
{
  uint tick0 = ticks;
  proc->p_chan=e;
  proc->p_stat=SSLEEPING;
  transform(); /* swtch away.*/
  proc->p_chan=0; /* when sched back (return from wakeup), tidy up. */
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







