#include "yotypes.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "param_yonix.h"



//#include "process.h"

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


uint ticks; //interrupts



//中断描述符初始化
void trapvecinit(void){
  int i;
  //针对所有终端
  for (i=0; i< TV_ENTRIES;i++){
    /* set idt[i].trap = false to note that this is nil ax a trap.
     * set idt[i].d=ZERO to prevent user from calling that int.
     * now this iz x86 protect faculty! */
    SETGATE(idt[i], false, SEG_KCODE<<3, vectors[i], 0);
  }
  //单独处理系统调用中断
  //同时也设置系统调用门的权限为 DPL_USER，这使得用户程序可以通过 int 指令产生一个内陷。
  /* set T_SYSCALL.trap=true, set d=DPL_USER to enable user to call this trap. */
  SETGATE(idt[T_SYSCALL], true, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);
  /* TODO 这里的SEG_KCODE还不懂。*/

}

//初始化硬件中断描述符
void idtinit(void)
{
	lidt(idt, sizeof(idt));
}

//中断处理
void trap (struct trapframe * tf)
{

	//针对系统调用中断trap
	if (tf->trapno == T_SYSCALL){
		//检查它的 p->killed，如果被设置了，该进程就会调用 exit，终结自己。
		if(proc->p_killed)
		  exit();

		else{
		  proc->p_tf=tf;
		  syscall(); // entering syscall;
		  if(proc->p_killed)// TODO 为什么要再检查一遍？
			exit();
		  return;
		}
	  }

	//针对其他的中断interrupt
	switch (tf->trapno)
	{
	case T_IRQ0+IRQ_TIMER://时钟中断 处理过程不懂
		//cpunum()获取cpu自身的编号
      ticks++;//一个ticks就是一个10ms
			wakeup(&ticks);
		break;
	case T_IRQ0 + IRQ_KBD://键盘中断请求
		kbdintr();
		break;
	case T_IRQ0 + IRQ_COM1:
		uartintr();

		break;
	case T_IRQ0+IRQ_IDE0://IDE0输入请求
		ideintr();
		break;
	case T_IRQ0+IRQ_IDE1://IDE1输入请求
		// Bochs generates spurious IDE1 interrupts.
		break;
	case T_IRQ0 + IRQ_PRI://打印机中断请求
	case T_IRQ0 + IRQ_SPURIOUS:
		cprintf("cpu%d: spurious interrupt at %x:%x\n",
		  0, tf->cs, tf->eip);

		break;

	case T_PGFLT:		// 页错误处理
	if (proc == 0  || (tf->cs&3) == 0){
       panic("trap");
    }

    if (pgflt_handle(rcr2()) < 0){
        cprintf("can not handler page fault from eip %x cr2 %x",tf->eip, rcr2());
        panic("page fault");
    }
    break;

	default://不懂。。。
		//tf->cs&3得到的是系统调用门的权限
		if (proc == 0 || (tf->cs & 3) == 0) {
			// In kernel, it must be our mistake.
			cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
				tf->trapno, 0, tf->eip, rcr2());
			panic("trap");
		}
		// In user space, assume process misbehaved.
		cprintf("pid %d %s: trap %d err %d on cpu %d "
			"eip 0x%x addr 0x%x--kill proc\n",
			proc->p_pid, proc->p_name, tf->trapno, tf->err,0, tf->eip,
			rcr2());
		proc->p_killed = 1;

	}
	//针对当前cpu中有进程在运行的情况
	if (proc)
	{
		//针对用户级进程,若p_killed位被置为1，则调用exit()结束自己的进程
		if (proc->p_killed&&(tf->cs & 3 )== DPL_USER)
			exit();
		//对于内核级进程，则让它一直运行，直到它返回系统调用

		//当进程运行时间到了RR调度的时间片时，强行迫使进程放弃CPU
		if (tf->trapno == T_IRQ0 + IRQ_TIMER&&proc->p_stat == SRUN){
      //WARNING modified.
			//giveup_cpu();

      timeslice_yield();
    }

		//检查一遍进程在放弃cpu之后进程是否被killed了（为什么呢？？？？？。。。。）
		if (proc->p_killed && (tf->cs & 3) == DPL_USER)
			exit();
	}

}


