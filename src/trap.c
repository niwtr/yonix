#include "traps.h"
#include "x86.h"
#include "def.h"
#include "yotypes.h"
#include "mmu.h"


/* below: borrowed from xv6 */
// interrupt descriptor table
struct gatedesc idt[TV_ENTRIES]; //为所有CPU共享的终端描述表 TODO：这是不是chapter3里介绍的中断描述符表？
extern uint vectors[];
struct splinlock tickslock;
uint ticks;

void tvinit(void){
  int i;
  for (i=0; i< TV_ENTRIES;i++){
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  }
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

}


void trap (struct trapframe * tf)
{
  if (tf->trapno == T_SYSCALL){
    if(proc->p_killed)
      exit();
    else{
      proc->p_tf=tf;
      syscall(); // entering syscall;
      if(proc->killed)// TODO 为什么要再检查一遍？
        exit();
      return;
    }
  }



}


