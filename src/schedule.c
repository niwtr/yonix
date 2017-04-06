#include "process.h"
#include "lock.h"
#include "queue.h"


bool queueInit()
{
	
	readyqueue = queue_create();
	if (readyqueue == nullptr)
		painc("queueInit:readyqueue failed!");
	return true;
}

//TODO proc=0中的0可能是指调度器本身。
//调度器RRscheduler RR+FCFS
//每个CPU在初始化后边调用该函数
//从调度器状态切换到用户态
void scheduler(bool preemptive)
{
	struct proc * p;
	while (true)
	{
		if(preemptive)
			sti();//允许时间片中断，中断后trap调用yeild()函数
		if (!is_queue_empty(readyqueue))
		{
			//从就绪队列中取出第一个进程,因为进程在加入就绪队列时，已经根据不同的调度算法
			//排好了顺序，所以，第一个进程即为下一个被调度的进程
			p = readyqueue->head;

			//切换其状态为RUNNING
			proc = p;
			switchvm(p);//交换虚拟内存
			p->p_stat = SRUN;
			//把进程移出就绪队列
			out_queue(readyqueue);

			swtch(&cpu->scheduler, p->p_ctxt);
			switchvm();//

			proc = 0;//
		}
	}
}

//进程从用户态切换到cpu调度器xv6中的sched
void transform(void)
{
	/*
	该函数检查了两次状态，这里的状态表明由于进程此时持有锁，所以 CPU 应该是在中断关闭的情况下运行的。
	最后，调用 swtch 把当前上下文保存在 proc->context 中然后切换到调度器上下文即 cpu->scheduler 中
	*/
	if (proc->p_stat == SRUN)
		panic("sched running");
	swtch(&proc->context, cpu->scheduler);
	//切换上下文，该上下文是在scheduler中切换时保存的
	cpu->intena=intena;
}

//放弃CUP的所有权——针对时间片到期后
void giveup_cpu(void)
{
	//在所有状态改变的操作中，都需要先获得锁，以保证不会有冲突发生
	proc->p_stat = READY;

	//加入就绪队列
	qnode * qn;
	qn = (qnode*)kmalloc(sizeof(qnode*));
	qn->p = (void*)proc;
	readyqueue = addtoqueue(qn, readyqueue,sched_al);

	transform();
}


/* sleep a proc on specific event and swtch away. */
void sleep(void * e)
{
  //TODO 为什么要检查是否为0？
  //EXPLAIN：可能0是指调度器本身。
  if(proc==0)
    panic("sleep");
  //tell event.
  proc->p_chan=e;
  proc->p_stat=SSLEEPING;

  transform(); //swtch away.
  proc->p_chan=0; // when sched back (return from wakeup), tidy up.
}

void wakeup(void * e)
{
	//find specific proc that is sleep on specific event e (or chan)
	struct proc *p;
	 search_through_ptablef(p)
	{
		if (p->p_stat == SSLEEPING && p->p_chan == e)
		{
			p->p_stat=READY;
		
			//将进程加入就绪队列
			qnode * qn;
			qn = (qnode*)kmalloc(sizeof(qnode*));
			qn->p = (void*)p;
			readyqueue = addtoqueue(qn, readyqueue, sched_al);
		}
      
	}
	  
}

//先来先服务的方法调度
queue * inqueue_FCFS(qnode * qn, queue* q)
{
	return  inqueue(qn, q);
}

//按照进程优先级的方法
queue * inqueue_PRI(qnode *qn, queue*q)
{
	//找到合适的位置插入就绪队列,插入后，队列已经有序
	qnode * tmp;
	//第一个插入队列的函数
	if (q->head = nullptr)
		inqueue(qn, q);
	else
	{
		for (tmp = q->head; tmp != nullptr; tmp = tmp->next)
		{
			//优先级的值越小，优先级越高
			if (qn->p->p_pir < tmp->p->p_pir)
			{
				//若该节点为头结点
				if (tmp == q->head)
				{
					q->head = qn;
					qn->next = tmp;
					qn->pre = nullptr;
					tmp->pre = qn;
				}
				//若该节点应放在尾部
				else if (tmp == q->tail)
				{
					qn->next = nullptr;
					qn->pre = tmp;
					tmp->next = qn;
					q->tail = qn;
				}

				else	//若该节点不在头结点的位置,也不在尾结点的位置
				{
					tmp->pre->next = qn;
					qn->next = tmp;
					qn->pre = tmp->pre;
					tmp->pre = qn;
				}
			}
		}
	}
	return q;
}


queue * addtoqueue(qnode* qn, queue *q, sched_algorithm al)
{
	if (al == FCFS)
		return inqueue_FCFS(qn, q);
	if (al == PRIORITY)
		return inqueue_PRI(qn, q);
}

void dy_sched()
