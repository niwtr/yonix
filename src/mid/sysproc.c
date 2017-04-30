#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "param_yonix.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(sysc_argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->p_pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(sysc_argint(0, &n) < 0)
    return -1;
  addr = proc->p_size;
  if(procgrow(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(sysc_argint(0, &n) < 0)
    return -1;

  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->p_killed){
      return -1;
    }
    sleep(&ticks);
  }
  //avgslp的单位为ticks，在0到100之间。
  proc->p_avgslp = MIN(proc->p_avgslp+(ticks-ticks0), MAX_AVGSLP);

  return 0;
}


// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  xticks = ticks;
  return xticks;
}



private int __incnice(int n){
  int aim= proc->p_nice+n;
  if(MIN_NICE <= aim && aim <= MAX_NICE){
    proc->p_nice = aim;
    return 0;
  }
  else return -1; //illegal nice val.
}


int sys_incnice(void){
  int n;
  if(sysc_argint(0,&n)<0)
    return -1;

  return __incnice(n);
}

int sys_decnice(void){
  int n;
  if(sysc_argint(0, &n) <0)
    return -1;
  return __incnice(-n);
}
int sys_sched(void){
  int n;
  if(sysc_argint(0,&n) < 0)
    return -1;
  if(n >= sizeof (SCHEME_NUMS)) //支持的schedule算法的数目
    return -1;
  select_scheme(n);
  recalc_timeslice();
  return 0;
}




int
sys_lwp_create(void)
{
  int fn, arg, stack, stksz;
  if(sysc_argint(0, &fn) < 0)
    return -1;
  if(sysc_argint(1, &arg) < 0)
    return -1;
  if(sysc_argint(2, &stack) < 0)
    return -1;
  if(sysc_argint(3, &stksz) < 0)
    return -1;
  if(stksz < 0) return -1;
  return lwp_create((void*)fn, (void*)arg, (void*)stack, stksz);
}

int
sys_lwp_join(void)
{
  int stack;
  if(sysc_argint(0, &stack) < 0)
    return -1;
  return lwp_join((void**)stack);
}



