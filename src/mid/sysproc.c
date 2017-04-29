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


