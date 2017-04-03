#include "process.h"
#include "lock.h"

int nextpid = 1;

//fork其余进程
extern void frk_ret(void);
//trap其余进程
extern void trp_ret(void);

//从空闲进程中找到一个进程
static struct proc* procalloc(void)
{

	//指示栈顶位置
	char * sp;

	//找到内存中处于SUNUSED状态的进程
  search_through_ptablef(p)
	{
		//如果找到了，则将其状态改为SEMBRYO
		//且增加进程的pid值
		if (p->p_stat == SUNUSED)
		{
			p->p_stat = SEMBRYO;
			p->p_pid = nextpid;
			nextpid++;


			//为该新进程分配内存栈空间
			p->p_kstack = kalloc();	//内核栈分配函数
			//若分配失败,将其状态改回SUNUSED
			if (!p->p_kstack)
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
			sp = sp - 4;
			*(uint*)sp = (uint)trapret;

			sp = sp - sizeof (*p->p_ctxt);
			p->p_ctxt = (struct context*)sp;
			memset(p->p_ctxt, 0, sizeof (*p->p_ctxt));
			p->p_ctxt->eip = (uint)forkret;

			//返回新建的进程
			return p;

/*
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
 *                         |                      |          +------------+
 *                         +----------------------+--------->|eip=forkret |
 *                         |                      |          +------------+
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
 */
      //TODO 为什么trapret会出现在这里？它在以后会有用吗？

		}

	}
	//如果找了一圈都没有找到SUNUESD的进程，则直接返回

	return 0;
}

int procgrow(int n){
  uint sz;
  sz = proc->sz;
  if(n > 0){
    //TODO require api allocuvm
    sz=allocuvm(proc->pgdir,sz,sz+n);
    if(sz==0)
      return -1;
  } else if(n<0){
    //TODO require api deallocuvm
    sz = deallocuvm(proc->pgdir, sz, sz+n);
    if(sz==0)
      return -1;
  }
  proc->sz = sz;
  switchuvm(proc);
  return 0;
}



//fork函数
int fork(void)
{
	int i, pid;
	struct proc *np;

	//为进程分配内核空间
	np = allocproc();
	if (np == 0)
	{
		return -1;
	}

	//将父进程的状态拷贝一份 vm.c
  //TODO add api
	np->p_cdir = copyuvm(, proc->sz);
	if (np->p_cdir == 0)	//虚存分配失败，则释放进程所占内存空间？？
	{
    //TODO add api
		kfree(np->kstack);
		np->p_kstack = 0;
		np->p_stat = SUNUSED;
		return -1;
	}

	//若以上操作都顺利进行，将父进程的各种信息复制一份到子进程上
	np->p_size = proc->p_size;
	np->p_prt = proc;
	*(np->p_tf) = *(proc->p_tf);

	//清空寄存器eax的值 以便fork后返回给子进程的值为0
	np->p_tf->eax = 0;
  //TODO 这是怎么做到的？还是没有看明白这是什么意思。

	//遍历父进程锁打开的所有文件,将其复制一份到子进程
	for (i = 0; i < P_NOFILE; i++)
		if (proc->p_of[i])
			np->p_of[i] = filedup(proc->p_of[i]);

	//拷贝父进程所在目录
  //TODO require api
	np->p_cdir = dirdup(proc->p_cdir);

	//拷贝父进程的进程名
  //TODO require api
	safestrcpy(np->name, proc->name, sizeof(proc->name));

	//保存子进程的pid值，以便返回给父进程
	pid = np->p_pid;

	//修改新建子进程的状态
	np->p_stat = READY;


	//将子进程的pid返回给父进程
	return pid;
}

void exit(void){


  //initproc是所有proc的根本。
  if(proc == initproc)
    panic("init exiting");

  // close all opened files.
  int fd;
  for(fd=0;fd< NOFILE;fd++){
    if(proc->p_of[fd]){
      fileclose(proc->p_of[fd]);
      proc->p_of[fd]=0;
    }
  }


  //TODO 弄懂？
  //LOG相关
  begin_op();
  //fs.c
  iput(proc->p_cdir);
  end_op();
  proc->p_cdir=0;


  wakeup(proc->p_prt);

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
      p->killed=1;
      if(p->state==SSLEEPING)
        p->state=READY;
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
    }
  int i;
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




