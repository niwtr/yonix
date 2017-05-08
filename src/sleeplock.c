// Sleeping locks
#include "param_yonix.h"
#include "yotypes.h"
#include "types.h"
#include "defs.h"
#include "param.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

#include "sleeplock.h"

void
initsleeplock(struct sleeplock *lk, char *name)
{

  lk->name = name;
  lk->locked = 0;
  lk->pid = 0;
}

void
acquiresleep(struct sleeplock *lk)
{
  while (lk->locked) {
    sleep(lk);
  }
  lk->locked = 1;
  lk->pid = proc->p_pid;

}

void
releasesleep(struct sleeplock *lk)
{
  lk->locked = 0;
  lk->pid = 0;
  wakeup(lk);
}

int
holdingsleep(struct sleeplock *lk)
{
  return lk->locked;
}



