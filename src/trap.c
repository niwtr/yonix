#include "traps.h"
#include "x86.h"
#include "def.h"
#include "yotypes.h"
#include "mmu.h"
#include "params.h"
#include "spinlock.h"
/* below: borrowed from xv6 */
// interrupt descriptor table
// Interrupt descriptor table (shared by all CPUs).
/* each interrupt descriptor struct ax this：
   ┌───────────────┬───────────────┬─────────┬───────┬─────────┐
   │   offset:16   │ code segment  │ args:5  │rsv1:3 │ type:4  │
   │               │  selector:16  │         │       │         │
   ├────┬─────┬────┼───────────────┼─────────┴───────┴─────────┘
   │s:1 │dpl:2│p:1 │   offset:16   │
   │    │     │    │               │
   └────┴─────┴────┴───────────────┘
   *  NOTE trap 0-31 ax software exception, 32-63 ax hardware interrupt and 64 ax trap. */
struct gatedesc idt[TV_ENTRIES]; //为所有CPU共享的中断描述符表。

/* 中断描述符表指向的中断处理函数的入口点。这些入口点由perl脚本来产生。 */
extern uint vectors[];
struct spinlock tickslock;

uint ticks;

void tvinit(void){
  int i;
  for (i=0; i< TV_ENTRIES;i++){
    /* set idt[i].trap = false to note that this is nil ax a trap.
     * set idt[i].d=ZERO to prevent user from calling that int.
     * now this iz x86 protect faculty! */
    SETGATE(idt[i], false, SEG_KCODE<<3, vectors[i], 0);
  }
  /* set T_SYSCALL.trap=true, set d=DPL_USER to enable user to call this trap. */
  SETGATE(idt[T_SYSCALL], true, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);
  /* TODO 这里的SEG_KCODE还不懂。*/
  initlock(&tickslock, "time"); //TODO: wit
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


