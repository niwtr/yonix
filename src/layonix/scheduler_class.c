#include "param_yonix.h"
#include "yotypes.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
int __debug = 0;

/* Define a scheduler method (scheme)
 * We hold the view that a scheduler method can be described by five distinct functions:
 * the main function: describes the algorithm itself, of how to choose the next proc to sched.
 * after function: things to be done once the proc is sched away.(giveup_cpu)
 * timeslice function : how to update(calculate) timeslice.
 * init function : the init process when the schedule method is selected.
 * enqueue function: some schemes use a ready queue for fast selection, this one is used
 * to describe the enqueue procedure. */
#define DEFSHED(X,BODY, BODY_AFTER, BODY_TIMESLICE, BODY_BEFORE, BODY_ENQUEUE) \
  void sched_ ## X ## _enqueue(struct proc *);                          \
  void sched_ ## X ## _timeslice (struct proc *);            \
  void sched_ ## X ## _after(void);                            \
  void sched_ ## X ## _before(void); \
  int sched_ ## X (void)                       \
    BODY \
    void sched_ ## X ## _after(void)            \
    BODY_AFTER\
    void sched_ ## X ## _timeslice (struct proc * P)  \
    BODY_TIMESLICE \
    void sched_ ## X ## _init (void)\
    BODY_BEFORE\
    void sched_ ## X ## _enqueue(struct proc * P)\
    BODY_ENQUEUE


/* fifo schedule method
 * a queue is in need (rdyqueue) for fast selection. */
DEFSHED(fifo, //name: fifo
        //fifo body
        {
          /* if the proc has not end itself,
           * just re-sched to it. */
          if(proc && proc->p_stat==READY)
            switch_to(proc);
          /* nothing to sched in. */
          if(Q_EMPTY(&rdyqueue))
            return 0;
          /* ok, get the first one and dequeue. */
          struct slot_entry * e = Q_FIRST(&rdyqueue);
          Q_REMOVE(&rdyqueue, e, lnk);
          if(__debug)
          cprintf("YONIX: Sched (FIFO) to PID %d\n", e->slotptr->p_pid);
          switch_to(e->slotptr);
          free_slab((char*) e);
          return 1;
        },
        //after
        {
          /* recalc the fifo-only timeslice (ETERNAL) */
          sched_fifo_timeslice(proc);
        },
        //timeslice
        {
          /* recalc the fifo-only timeslice (ETERNAL) */
          P->p_time_slice = SCHED_FIFO_TIMESLICE; //forever.
        },
        //init
        {
          /* we first check wether the queue is empty
           * that is, the fifo scheme is once used.
           * we delete the trashes and rebuild the ready queue. */
          struct slot_entry * e;
          if(!Q_EMPTY(&rdyqueue))
            Q_FOREACH(e, &rdyqueue, lnk)
              if(e->slotptr->p_stat != READY)
                {
                  Q_REMOVE(&rdyqueue, e, lnk);
                  free_slab((char *) e);
                }
          /* rebuild ready queue. */
          search_through_ptablef(p)
            if(p->p_stat==READY)
                sched_fifo_enqueue(p);
        },
        //enqueue
        {
          struct slot_entry * e = (struct slot_entry *)alloc_slab();
          e->slotptr = P;
          Q_INSERT_TAIL(&rdyqueue, e, lnk);
        }
        )


/* the simplest sched algorithm, no queue is in need.
 * each time the algo is called it sched in the next one.
 * however this one is not O(1). */
DEFSHED(rr,
        //rr body
        {
          /* use static var to track the last entry point. */
          static struct proc * p=ptable.proc;
          while(p<&ptable.proc[PROC_NUM]){
            if(p->p_stat != READY){
              p++;
              continue;
            }
            if(__debug)
            cprintf("YONIX: Sched (RR) to PID %d\n", p->p_pid);
            switch_to(p);
            p++;
            return 1;
          }
          p=ptable.proc; /* wind back. */
          return 0;
        },
        //after
        {
          /* just set the timeslice. */
          sched_rr_timeslice(proc);
        },
        //timeslice
        {
          P->p_time_slice= SCHED_RR_TIMESLICE;
        },
        //init
        {
          /* nothing to do. */
        },
        //enqueue
        {
          /* no queue is in need. */
        }
        )


DEFSHED(priority,
        //priority body
        {
          struct proc * p_primax = 0 ;
          search_through_ptablef(p){
            if(p->p_stat == READY) {
              if(p_primax == 0)
                p_primax = p;
              else
                if(p_primax->p_dpri < p->p_dpri) // 找到了一个优先级更高的。注意dpri越低，优先级越高。
                  p_primax = p;
            }
          }

          if(p_primax) //找到了进程 to swtich
          {
            if(__debug)
            cprintf("YONIX: Sched (PRI) to PID %d\n", p_primax->p_pid);
            switch_to(p_primax);
            return 1;
          } else {return 0;}
        },



        //after
        {
          sched_priority_timeslice(proc);//重新计算timeslice
          proc->p_dpri = DYNAMIC_PRI(proc->p_spri, BONUS(proc->p_avgslp));
        },
        //timeslice
        {
          P->p_spri = STATIC_PRI(P->p_nice);
          P->p_time_slice = TIME_SLICE(P->p_spri);
        },
        //init
        {

        },
        //enqueue
        {

        }
        )


struct sched_class
sched_reftable[SCHEME_NUMS]={
  [SCHEME_FIFO]= {
    SCHEME_FIFO,
    "FIFO",
    sched_fifo,
    sched_fifo_after,
    sched_fifo_timeslice,
    sched_fifo_init,
    sched_fifo_enqueue
  },
  [SCHEME_RR]  = {
    SCHEME_RR,
    "RR",
    sched_rr,
    sched_rr_after,
    sched_rr_timeslice,
    sched_rr_init,
    sched_rr_enqueue
  },
  [SCHEME_PRI] = {
    SCHEME_PRI,
    "PRIORITY",
    sched_priority,
    sched_priority_after,
    sched_priority_timeslice,
    sched_priority_init,
    sched_priority_enqueue
  }
};

