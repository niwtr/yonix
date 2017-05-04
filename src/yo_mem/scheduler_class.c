#include "param_yonix.h"
#include "yotypes.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
//extern struct proc * proc;


#define DEFSHED(X,BODY, BODY_AFTER, CAPTURE_NAME, BODY_TIMESLICE, BODY_BEFORE)  \
  void sched_ ## X ## _timeslice (struct proc *);            \
  void sched_ ## X ## _after(void);                            \
  void sched_ ## X ## _before(void); \
  int sched_ ## X (void)                       \
    BODY \
    void sched_ ## X ## _after(void)            \
    BODY_AFTER\
    void sched_ ## X ## _timeslice (struct proc * CAPTURE_NAME) \
    BODY_TIMESLICE \
    void sched_ ## X ## _init (void)\
    BODY_BEFORE


/*
DEFSHED(fifo, //name: fifo
        //fifo body
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
          if(min){
            switch_to(min);
            return 1;
          }
          else return 0;
        },
        //after
        {
          sched_fifo_timeslice(proc);
        },
        //timeslice
        p,//captured name
        {
          p->p_time_slice = SCHED_FIFO_TIMESLICE; //forever.
        })
*/
DEFSHED(fifo, //name: fifo
        //fifo body
        {

          if(proc && proc->p_stat==READY)
            switch_to(proc);

          if(Q_EMPTY(&rdyqueue)){
            return 0;
          }
          struct slot_entry * e = Q_FIRST(&rdyqueue);
          Q_REMOVE(&rdyqueue, e, lnk);
          switch_to(e->slotptr);
          free_slab((char*) e);
          return 1;
        },
        //after
        {
          sched_fifo_timeslice(proc);
        },
        //timeslice
        p,//captured name
        {
          p->p_time_slice = SCHED_FIFO_TIMESLICE; //forever.
        },
        {
          struct slot_entry * e;
          if(!Q_EMPTY(&rdyqueue))
            Q_FOREACH(e, &rdyqueue, lnk)
              if(e->slotptr->p_stat != READY)
              {
                Q_REMOVE(&rdyqueue, e, lnk);
                free_slab((char *) e);
              }
          search_through_ptablef(p)
            if(p->p_stat==READY)
            {
              struct slot_entry * e = (struct slot_entry *)alloc_slab();
              e->slotptr = p;
              Q_INSERT_TAIL(&rdyqueue, e, lnk);
            }
        }
        )



DEFSHED(rr,
        {
          static struct proc * p=ptable.proc;
          while(p<&ptable.proc[PROC_NUM]){
            if(p->p_stat != READY){
              p++;
              continue;
            }
            switch_to(p);
            p++;
            return 1;
          }
          p=ptable.proc; // wind back.
          return 0;
        },
        {
          sched_rr_timeslice(proc);
        },
        p,
        {
          p->p_time_slice= SCHED_RR_TIMESLICE;
        },
        {

        }
        )


DEFSHED(priority,
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
          {
            switch_to(p_primax);
            return 1;
          } else {return 0;}
        },
        {
          sched_priority_timeslice(proc);//重新计算timeslice
          proc->p_dpri = DYNAMIC_PRI(proc->p_spri, BONUS(proc->p_avgslp));
          //cprintf("pid %d ts %d bns %d dpri %d\n", proc->p_pid,proc->p_time_slice,BONUS(proc->p_avgslp), proc->p_dpri);
        },
        p,
        {
          p->p_spri = STATIC_PRI(p->p_nice);
          p->p_time_slice = TIME_SLICE(p->p_spri);
        }
        ,
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
    sched_fifo_init
  },
  [SCHEME_RR]  = {
    SCHEME_RR,
    "RR",
    sched_rr,
    sched_rr_after,
    sched_rr_timeslice,
    sched_rr_init
  },
  [SCHEME_PRI] = {
    SCHEME_PRI,
    "PRIORITY",
    sched_priority,
    sched_priority_after,
    sched_priority_timeslice,
    sched_priority_init
  }
};

