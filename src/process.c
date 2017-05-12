#include "param_yonix.h"
#include "yotypes.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "queue_netbsd.h"
int nextpid = 1;

//fork return
extern void forkret(void);
//trap return
extern void trapret(void);


struct protab ptable;
struct proc * initproc;
struct proc * proc;

void esinit()
{
  Q_INIT(&esqueue);
  search_through_ptablef(p){
    struct slot_entry * e = (struct slot_entry *) alloc_slab();
    e->slotptr=p;
    Q_INSERT_TAIL(&esqueue, e, lnk);
  }
}
void rdinit()
{
  Q_INIT(&rdyqueue);
  //dynamic shed queue init
  for (int i=0;i<40;i++)
    Q_INIT(&rdy_q_dy[i]);
}




/*
 *   +------------+
 *   |  kstack +  | <------+----------------------+
 *   | kstacksize |        |                      |
 *   +------------+        |      trapframe       |
 *                         |                      |
 *                         +----------------------+
 *                         |     trapret ptr      |<- the trapret pointer is placed here
 *                         |                      |   so once the proc returns from trap() it
 *                         +----------------------+   falls through to trapret.
 *                         |       context        |
 *                         |                      |
 *                         +----------------------+
 *                         |                      |
 *                         |                      |
 *                         |                      |
 *                         |                      |
 *                         |                      |
 *                         |                      |
 *                         |                      |
 *                         |                      |
 *                         |                      |
 *                         |                      |
 *                         |                      |
 *                         |                      |
 *                         |                      |
 *   +------------+        |                      |
 *   |   kstack   |        |                      |
 *   |   bottom   | <------+----------------------+
 *   +------------+
 *
 */

/* update the timeslice of each proc. */
void recalc_timeslice (void)
{
  search_through_ptablef(p){
    if(p->p_stat != SUNUSED)
      sched_reftable[cpu->scheme].timeslice(p);
  }
}

/* allocate a new proc. once the proc
 * is ready it is in SEMBRYO state
 * which cannot be scheduled directly.
 * the fork, userinit and lwp_create is
 * used to assemble the proc to sched-ready. */
static struct proc* procalloc(void)
{


	char * sp; /* stack pointer. */

  if(Q_EMPTY(&esqueue))
    return 0;

  /* remove from the empty slot queue. */
  struct slot_entry * ee = Q_FIRST(&esqueue);
  struct proc * p = ee->slotptr;
  Q_REMOVE(&esqueue, ee, lnk);
  free_slab((char *) ee);

  p->p_stat = SEMBRYO;
  p->p_pid = nextpid++;
  p->p_nice = 0;
  p->p_spri = STATIC_PRI(p->p_nice);
  p->p_creatime = ticks;
  p->p_avgslp = 0;
  sched_reftable[cpu->scheme].timeslice(p);
  p->p_dpri = DYNAMIC_PRI(p->p_spri, BONUS(p->p_avgslp));

  /* allocate the kernel stack. */
  p->p_kstack = kalloc();


  if (p->p_kstack==0)
  {
      p->p_stat = SUNUSED;
      return 0;
  }
  sp = p->p_kstack + K_STACKSZ;
  /* set the tf position */
  sp = sp - sizeof(*p->p_tf);
  p->p_tf = (struct trapframe*) sp;

  /* set the return position, trapret. */
  sp = sp - 4;
  *(uint*)sp = (uint)trapret;
  /* set the context position */
  sp = sp - sizeof (*p->p_ctxt);
  p->p_ctxt = (struct context*)sp;
  /* clear the kernel context */
  memset(p->p_ctxt, 0, sizeof (*p->p_ctxt));
  return p;
}



int procgrow(int n){
  uint sz;
  sz = proc->p_size;
  if(n > 0){
    sz=allocuvm(proc->p_page,sz,sz+n);
    if(sz==0)
      return -1;
  } else if(n<0){
    sz = deallocuvm(proc->p_page, sz, sz+n, proc->p_pid);
    if(sz==0)
      return -1;
  }
  proc->p_size = sz;
  switchuvm(proc);
  return 0;
}




/* fork a new proc. that is, clone the virtural memory space
 * of parent proc. generate a proc that is sched-ready. */
int fork(void)
{

	int i, pid;
	struct proc *np;
	np = procalloc();
	if (np == 0)
	{
		return -1; /* proc allocate fail. */
	}
  /* copy the parent vm */
	np->p_page = copyuvm(proc->p_page, proc->p_size, np->p_pid);
  /* copy failed. reset the state and collect rubbish. */
	if (np->p_page == 0)
	{
		kfree(np->p_kstack);
		np->p_kstack = 0;
		np->p_stat = SUNUSED;
    struct slot_entry * ee = (struct slot_entry*) alloc_slab();
    ee->slotptr = np;
    Q_INSERT_TAIL(&esqueue, ee, lnk);
		return -1;
	}

	np->p_size = proc->p_size;
	np->p_prt = proc;
  /* copu trapframe (user context) */
	*(np->p_tf) = *(proc->p_tf);
  /* clear the eax so that fork() returns zero in child */
	np->p_tf->eax = 0;
  /* set the entry where trap returns to. */
  np->p_ctxt->eip = (uint)forkret;
  /* duplicate the open file. */
	for (i = 0; i < P_NOFILE; i++)
		if (proc->p_of[i])
			np->p_of[i] = filedup(proc->p_of[i]);

  /* duplicate current direction */
	np->p_cdir = idup(proc->p_cdir);
  /* inherent the parent name.  */
	safestrcpy(np->p_name, proc->p_name, sizeof(proc->p_name));
	pid = np->p_pid;
  np->p_procp = 1; /* this is, indeed, a proc. */
	np->p_stat = READY;
  /* enter ready queue specified on each scheme. */
  sched_reftable[cpu->scheme].enqueue(np);
	return pid;
}

/* exit a proc. if there exists a parent waiting,
 * the parent is waken to kill this proc. or else the
 * proc become a zombie and is set to be cleaned by init. */
void exit(void){

  if(proc == initproc)
      panic("init exiting");

  /* close all opened files. */
  int fd;
  for(fd=0;fd< P_NOFILE;fd++){
    if(proc->p_of[fd]){
      fileclose(proc->p_of[fd]);
      proc->p_of[fd]=0;
    }
  }

  begin_op();
  /* release the inode pointing
   * to the current dir*/
  iput(proc->p_cdir);
  end_op();
  proc->p_cdir=0;

  /* wake up the parent! */
  wakeup(proc->p_prt);

  /* find its children and transfer
   * their parent to init which,
   * will peace them eventually. */
  search_through_ptablef(p){
    if(p->p_prt==proc){
      p->p_prt=initproc;
      if(p->p_stat==SZOMB)
        wakeup(initproc);
    }
  }
  /* become a zombie */
  proc->p_stat=SZOMB;
  /* sched away, good luck */
  transform();

  /* a zombie proc never escape form here. */
  panic("zombie exit.");

}

/* kill some proc. */
int kill(int pid)
{

  search_through_ptablef(p){
    if(p->p_pid==pid){
      p->p_killed=1;
      if(p->p_stat==SSLEEPING)
        p->p_stat=READY;
      return 0;
    }
  }return -1; //not found
}


void forkret(void){
  static int firstproc=1;
  if(firstproc){
    firstproc=0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }
}



void dbg_procdump(void)
{
  static char * status [] =
    {
#define X(name) [name] #name,
      __STAT_LOV__
#undef X
    };

  char * state;
  uint pc[10];
  search_through_ptablef(p){
    if(p->p_stat==SUNUSED)
      continue;
    if(p->p_stat>=0 && p->p_stat<sizeof(status))
      state=status[p->p_stat];
    else
      state="UNKNOWN";
    cprintf("%d %s %s", p->p_pid, state, p->p_name);
    if(p->p_stat==SSLEEPING){
      // track back the sleeping proc.
      getcallerpcs((uint*)p->p_ctxt->ebp+2, pc);
      int i;
      for (i=0;i<10 && pc[i]!=0;i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }

}

void dbg_lstprocs(void){
  static char * status [] =
    {
#define X(name) [name] #name,
      __STAT_LOV__
#undef X
    };
  char * state;
  cprintf("\n");
  search_through_ptablef(p){
    if(p->p_stat == SUNUSED)
      continue;
    if(p->p_stat >= 0 && p->p_stat <sizeof(status))
      state=status[p->p_stat];
    else
      state="UNKNOWN";

    cprintf("%d %s %s %s ts:%d avgslp:%d dpri:%d nice:%d chan:%p psz:%d msz: %d vsz: %d\n", p->p_pid, p->p_name, state, p->p_procp?"proc":"thread", p->p_time_slice, p->p_avgslp, p->p_dpri, p->p_nice, p->p_chan,p->p_size, (int)procmemsz(p), (int)procvmsz(p));
  }
}
void dbg_lstslp(void){
  search_through_ptablef(p){
    if(p->p_stat != SSLEEPING)
      continue;
    cprintf("Sleeping %s PID: %d, chan: %p\n", p->p_procp?"proc":"thread", p->p_pid, p->p_chan);
  }
}


void dbg_lstrdy(void){
  int num=0;
  search_through_ptablef(p){
    if(p->p_stat != READY)
      continue;
    cprintf("%d %s %s %s ts:%d avgslp:%d dpri:%d chan:%p\n", p->p_pid, p->p_name, "READY", p->p_procp?"proc":"thread", p->p_time_slice, p->p_avgslp, p->p_dpri, p->p_chan);
    num++;
  }
  if(!num)
    cprintf("No process is currently in READY status.\n");

}



void userinit(void)
{
	struct proc *p;
	extern char _binary_initcode_start[], _binary_initcode_size[];
	p = procalloc();

	initproc = p;

	p->p_page = setupkvm();
	if (!p->p_page)
		panic("userinit: setupkvm failed!");


	inituvm(p->p_page, _binary_initcode_start, (int) _binary_initcode_size);
	memset(p->p_tf, 0, sizeof(*p->p_tf));
	p->p_tf->cs = (SEG_UCODE << 3) | DPL_USER;
	p->p_tf->ds = (SEG_UDATA << 3) | DPL_USER;
	p->p_tf->es = p->p_tf->ds;
	p->p_tf->ss = p->p_tf->ds;
	p->p_tf->eflags = FL_IF;
	p->p_tf->esp = PGSIZE;
	p->p_tf->eip = 0;

	p->p_size = PGSIZE;
	safestrcpy(p->p_name, "initcode", sizeof(p->p_name));

	p->p_cdir = namei("/");
  p->p_procp = 1;//this is indeed a proc.

	p->p_stat = READY;

  sched_reftable[cpu->scheme].enqueue(p);

  p->p_ctxt->eip = (uint)forkret;
}



int wait(void)
{
	int pid;		//�ӽ���pid
  int have_kid;
	while (true)
	{
    have_kid=0;

		search_through_ptablef(p)
		{

			if (p->p_prt == proc)
			{
        have_kid=1;
				if (p->p_stat == SZOMB)
				{

					pid = p->p_pid;
					kfree(p->p_kstack);
          p->p_kstack=0;
					freeuvm(p->p_page, pid);
					p->p_pid = 0;
					p->p_prt = 0;
					p->p_name[0] = 0;
					p->p_killed = 0;
					p->p_stat = SUNUSED;
          struct slot_entry * ee = (struct slot_entry*) alloc_slab();
          ee->slotptr = p;
          Q_INSERT_TAIL(&esqueue, ee, lnk);

					return pid;
				}
      }
    }
    if(!have_kid || proc->p_killed){
      return -1;
    }

    sleep(proc);
  }
}




/* create a light-weight proc. */
int lwp_create (void * task, void * arg, void * stack, int stksz){
  int i, pid;
  struct proc * lwp;
  lwp = procalloc();
  if(lwp == 0) return -1;

  lwp->p_page = proc->p_page;
  lwp->p_size = proc->p_size;

  if(lwp->p_page == 0) return -1;

  if (proc->p_procp){
    lwp->p_prt = proc;
  } else {
    lwp->p_prt = proc->p_prt;
  }
  *(lwp->p_tf) = *(proc->p_tf);

	lwp->p_tf->eax = 0;
  lwp->p_ctxt->eip=(uint)forkret;
  lwp->p_tf->eip = (int)task;
  lwp->p_stack = (int)stack;
  lwp->p_tf->esp = lwp->p_stack + stksz - 4 ;
  *((int *)(lwp->p_tf->esp)) = (int)arg;
  *((int *)(lwp->p_tf->esp-4))=0xFFFFFFFF;
  lwp->p_tf->esp -=4;

	for (i = 0; i < P_NOFILE; i++)
		if (proc->p_of[i])
			lwp->p_of[i] = filedup(proc->p_of[i]);

  lwp->p_cdir = idup(proc->p_cdir);

  safestrcpy(lwp->p_name, proc->p_name, sizeof(proc->p_name));

	pid = lwp->p_pid;
  lwp->p_procp = 0; // this is,not a proc.

	lwp->p_stat = READY;


  sched_reftable[cpu->scheme].enqueue(lwp);

	return pid;
}

int
lwp_join(void **stack)
{


  int have_kid, pid;

  for(;;){
    // Scan through table looking for zombie children.
    have_kid = 0;
    search_through_ptablef(p){
      // wait for the child thread, but not the child process
      if(p->p_prt != proc || p->p_procp != 0)
        continue;
      have_kid = 1;
      if(p->p_stat == SZOMB){
        // Found one.
        pid = p->p_pid;
        kfree(p->p_kstack);
        p->p_kstack = 0;
        p->p_stat = SUNUSED;
        p->p_pid = 0;
        p->p_prt = 0;
        p->p_name[0] = 0;
        p->p_killed = 0;
        *(int*)stack = p->p_stack; //mark the stack.
        struct slot_entry * ee = (struct slot_entry*) alloc_slab();
        ee->slotptr = p;
        Q_INSERT_TAIL(&esqueue, ee, lnk);
        return pid;
      }
    }

    // No point waiting if we don't have any children thread.
    if(!have_kid || proc->p_killed)
      return -1;


    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc);  //DOC: wait-sleep
  }
}


/* 提供了动态调转功能 */
void dynamic_sstore(void * stack, struct trapframe * tf, int stksz){
  memmove((char *)stack,(char *) proc->p_stack, stksz);
  memmove((char *)tf, (char *)proc->p_tf, sizeof(*tf));
}

void dynamic_restart(void * stack, struct trapframe * tf, int stksz){
  memmove((char *)proc->p_stack, (char *)stack, stksz);
  memmove((char *)proc->p_tf, (char *)tf, sizeof(*tf));
}
