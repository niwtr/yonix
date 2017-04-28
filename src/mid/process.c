

#include "param_yonix.h"
#include "yotypes.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"

int nextpid = 1;

//fork return
extern void forkret(void);
//trap return
extern void trapret(void);


struct protab ptable;
struct proc * initproc;

/* 找到一个空闲的槽，建立新的进程PCB并为其分配内核空间。
 * 新的进程的状态为SEMBRYO。
 * 新的进程将会有内核栈，这个内核栈的形态为：
 *   +------------+
 *   |  kstack +  | <------+----------------------+
 *   | kstacksize |        |                      |
 *   +------------+        |      trapframe       |
 *                         |                      |
 *                         +----------------------+
 *                         |     trapret ptr      |
 *                         |                      |
 *                         +----------------------+
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
 * 产生的进程称为芽孢进程。这个进程的上下文全部为初始值0.
 */


static struct proc* procalloc(void)
{

	//指示栈顶位置
	char * sp;

	//找到内存中处于SUNUSED状态的进程
    search_through_ptablef(p)
	{
    //cprintf("searching for unused.:%d,%d\n", p->p_stat, ittt);
		//如果找到了，则将其状态改为SEMBRYO
		//且增加进程的pid值
		if (p->p_stat == SUNUSED)
		{
			p->p_stat = SEMBRYO;
			p->p_pid = nextpid++;
			//nextpid++;


			//为该新进程分配内核栈空间
			p->p_kstack = kalloc();	//内核栈分配函数

			//若分配失败,将其状态改回SUNUSED
			if (p->p_kstack==0)
			{
				p->p_stat = SUNUSED;
				return 0;
			}

			//修改栈顶位置
			sp = p->p_kstack + K_STACKSZ;

			//为tarpframe留出位置？？
			sp = sp - sizeof(*p->p_tf);
			p->p_tf = (struct trapframe*) sp;


			// Set up new context to start executing at forkret,
			// which returns to trapret.
      //TODO 这是怎么做到的？
			sp = sp - 4;
			*(uint*)sp = (uint)trapret;

			sp = sp - sizeof (*p->p_ctxt);
			p->p_ctxt = (struct context*)sp;
      //TODO require API
			memset(p->p_ctxt, 0, sizeof (*p->p_ctxt));
      //返回新建的进程
			return p;

		}

	}
	//如果找了一圈都没有找到SUNUESD的进程，则直接返回
	return 0;
}


/* 调整当前进程分配的虚拟内存空间。 */
int procgrow(int n){
  uint sz;
  sz = proc->p_size;
  if(n > 0){ //扩大虚拟内存
    //TODO require api allocuvm
    sz=allocuvm(proc->p_page,sz,sz+n);
    if(sz==0)
      return -1;
  } else if(n<0){ //减小虚拟内存
    //TODO require api deallocuvm
    sz = deallocuvm(proc->p_page, sz, sz+n);
    if(sz==0)
      return -1;
  }

  proc->p_size = sz;
  switchuvm(proc); //FIXME 这是什么？
  return 0;
}



/* fork函数，创建子进程。 */
int fork(void)
{
  cprintf("   forking...\n");
	int i, pid;
	struct proc *np;

	//为进程分配内核空间
	np = procalloc();

	if (np == 0)
	{
		return -1;
	}


	//拷贝父进程的用户地址空间（用户虚拟内存）到新的物理内存处。
    //TODO add api
  //FIXME xv6
	np->p_page = copyuvm(proc->p_page, proc->p_size);
	if (np->p_page == 0)	//虚存分配失败，则释放进程所占内存空间
    //TODO 是否可以把这段代码提出来成一个单独的函数？
	{
    //TODO add api
		kfree(np->p_kstack);
		np->p_kstack = 0;
		np->p_stat = SUNUSED;
		return -1;
	}
	//若以上操作都顺利进行，将父进程的各种信息复制一份到子进程上
	np->p_size = proc->p_size; //设置进程的镜像大小。
	np->p_prt = proc;          //把父进程设置为当前调用者。
	*(np->p_tf) = *(proc->p_tf);//设置trapframe。

	//清空寄存器eax的值 以便fork后返回给子进程的值为0
	np->p_tf->eax = 0;
  np->p_ctxt->eip = (uint)forkret;
  //TODO 这是怎么做到的？还是没有看明白这是什么意思。

	//遍历父进程打开的所有文件,将其复制一份到子进程
	for (i = 0; i < P_NOFILE; i++)
		if (proc->p_of[i])
			np->p_of[i] = filedup(proc->p_of[i]);

	//拷贝父进程所在目录
  //TODO require api
  //FIXME xv6
	np->p_cdir = idup(proc->p_cdir);

	//拷贝父进程的进程名
  //TODO require api
  //FIXME xv6
	safestrcpy(np->p_name, proc->p_name, sizeof(proc->p_name));

	//保存子进程的pid值，以便返回给父进程
	pid = np->p_pid;

	//修改新建子进程的状态
	np->p_stat = READY;


	//将子进程的pid返回给父进程
  cprintf("   forked\n");

	return pid;
}

void exit(void){


  //initproc是所有proc的根本。该进程不可能退出！
  if(proc == initproc)
    panic("init exiting");

  // close all opened files.
  int fd;
  for(fd=0;fd< P_NOFILE;fd++){
    if(proc->p_of[fd]){
      fileclose(proc->p_of[fd]);
      proc->p_of[fd]=0;
    }
  }


  //释放当前进程所在目录
  //TODO 文件系统
  //TODO require api
  begin_op();
  iput(proc->p_cdir);
  end_op();
  proc->p_cdir=0;


  wakeup(proc->p_prt); //唤醒父进程。

  //TODO 这些还不太懂。
  search_through_ptablef(p){
    if(p->p_prt==proc){
      p->p_prt=initproc;
      if(p->p_stat==SZOMB)
        wakeup(initproc);
    }
  }

  proc->p_stat=SZOMB;
  transform();
  panic("zombie exit.");

}

//mark the proc to kill.
//TODO 什么时候这个进程被彻底杀掉？
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
    iinit(ROOTDEV);//FIXME
    initlog(ROOTDEV);//FIXME
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



//第一个用户级进程
void userinit(void)
{
	struct proc *p;
	extern char _binary_initcode_start[], _binary_initcode_size[];//这是干嘛的？
	p = procalloc();	//在页表中分配一个proc,并初始化进程状态 SUNUSED->SEMBRYO
						//第一个用户进程
	initproc = p;
	//为其分配虚拟内存页表 获取一个新的二级页表，并包含内核所有映射
	p->p_page = setupkvm();
	if (!p->p_page)
		panic("userinit: setupkvm failed!");

	//初始化用户进程虚拟地址空间
	inituvm(p->p_page, _binary_initcode_start, (int) _binary_initcode_size);

	//trapframe的设置
	memset(p->p_tf, 0, sizeof(*p->p_tf));
	p->p_tf->cs = (SEG_UCODE << 3) | DPL_USER;//%cs 寄存器保存着一个段选择器， 指向段 SEG_UCODE 并处于特权级 DPL_USER（即在用户模式而非内核模式）
	p->p_tf->ds = (SEG_UDATA << 3) | DPL_USER;//ds,es,ss段选择器指向段 SEG_UDATA 并处于特权级 DPL_USER
	p->p_tf->es = p->p_tf->ds;
	p->p_tf->ss = p->p_tf->ds;
	p->p_tf->eflags = FL_IF;// FL_IF 位被设置为允许硬件中断
	p->p_tf->esp = PGSIZE;//设为进程的最大有效虚拟内存
	p->p_tf->eip = 0;  // beginning of initcode.S指向初始化代码的入口点，即地址0

	p->p_size = PGSIZE;
	safestrcpy(p->p_name, "initcode", sizeof(p->p_name));
	//进程所在目录为根目录“/”
	p->p_cdir = namei("/");
	//将进程状态设置为READY
	p->p_stat = READY;
  p->p_ctxt->eip = (uint)forkret;//设置返回地址为forkret
}


int
wait1(void)
{
 
  int havekids, pid;

  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    //for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    search_through_ptablef(p){
      if(p->p_prt != proc)
        continue;
      havekids = 1;
      if(p->p_stat == SZOMB){
        // Found one.
        pid = p->p_pid;
        kfree(p->p_kstack);
        p->p_kstack = 0;
        freevm(p->p_page);
        p->p_pid = 0;
        p->p_prt = 0;
        p->p_name[0] = 0;
        p->p_killed = 0;
        p->p_stat = SUNUSED;
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->p_killed){
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc);  //DOC: wait-sleep
  }
}

//wait——等待子进程执行完毕exit,并返回其进程ID号
int wait(void)
{
	int pid;		//子进程pid
  int have_kid;
	while (true)
	{
    have_kid=0;
		//遍历进程列表，找到当前进程是否有子进程
		search_through_ptablef(p)
		{

			if (p->p_prt == proc)
			{
        have_kid=1;
				//若该子进程是一个僵尸进程——子进程已经结束，但父进程因为太忙而没有等待它。。。。
				//一个进程在调用exit命令结束自己的生命的时候，其实它并没有真正的被销毁，
				//而是留下一个称为僵尸进程（Zombie）的数据结构（系统调用exit，它的作用是 使进程退出，
				//但也仅仅限于将一个正常的进程变成一个僵尸进程，并不能将其完全销毁）
				if (p->p_stat == SZOMB)
				{
					//处理该进程信息
					pid = p->p_pid;
					kfree(p->p_kstack);	//释放内核栈
          p->p_kstack=0;
					freevm(p->p_page);	//释放虚拟内存
					p->p_pid = 0;
					p->p_prt = 0;		//父进程设为0
					p->p_name[0] = 0;
					p->p_killed = 0;
					p->p_stat = SUNUSED;
					return pid;
				}
      }
    }
    if(!have_kid || proc->p_killed){
      return -1;
    }
    //若找到一个不是僵尸进程的子进程，则父进程休眠，等待子进程变为僵尸进程
    sleep(proc);
  }
}


