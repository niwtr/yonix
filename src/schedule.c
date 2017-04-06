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

//TODO proc=0�е�0������ָ����������
//������RRscheduler RR+FCFS
//ÿ��CPU�ڳ�ʼ����ߵ��øú���
//�ӵ�����״̬�л����û�̬
void scheduler(bool preemptive)
{
	struct proc * p;
	while (true)
	{
		if(preemptive)
			sti();//����ʱ��Ƭ�жϣ��жϺ�trap����yeild()����
		if (!is_queue_empty(readyqueue))
		{
			//�Ӿ���������ȡ����һ������,��Ϊ�����ڼ����������ʱ���Ѿ����ݲ�ͬ�ĵ����㷨
			//�ź���˳�����ԣ���һ�����̼�Ϊ��һ�������ȵĽ���
			p = readyqueue->head;

			//�л���״̬ΪRUNNING
			proc = p;
			switchvm(p);//���������ڴ�
			p->p_stat = SRUN;
			//�ѽ����Ƴ���������
			out_queue(readyqueue);

			swtch(&cpu->scheduler, p->p_ctxt);
			switchvm();//

			proc = 0;//
		}
	}
}

//���̴��û�̬�л���cpu������xv6�е�sched
void transform(void)
{
	/*
	�ú������������״̬�������״̬�������ڽ��̴�ʱ������������ CPU Ӧ�������жϹرյ���������еġ�
	��󣬵��� swtch �ѵ�ǰ�����ı����� proc->context ��Ȼ���л��������������ļ� cpu->scheduler ��
	*/
	if (proc->p_stat == SRUN)
		panic("sched running");
	swtch(&proc->context, cpu->scheduler);
	//�л������ģ�������������scheduler���л�ʱ�����
	cpu->intena=intena;
}

//����CUP������Ȩ�������ʱ��Ƭ���ں�
void giveup_cpu(void)
{
	//������״̬�ı�Ĳ����У�����Ҫ�Ȼ�������Ա�֤�����г�ͻ����
	proc->p_stat = READY;

	//�����������
	qnode * qn;
	qn = (qnode*)kmalloc(sizeof(qnode*));
	qn->p = (void*)proc;
	readyqueue = addtoqueue(qn, readyqueue,sched_al);

	transform();
}


/* sleep a proc on specific event and swtch away. */
void sleep(void * e)
{
  //TODO ΪʲôҪ����Ƿ�Ϊ0��
  //EXPLAIN������0��ָ����������
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
		
			//�����̼����������
			qnode * qn;
			qn = (qnode*)kmalloc(sizeof(qnode*));
			qn->p = (void*)p;
			readyqueue = addtoqueue(qn, readyqueue, sched_al);
		}
      
	}
	  
}

//�����ȷ���ķ�������
queue * inqueue_FCFS(qnode * qn, queue* q)
{
	return  inqueue(qn, q);
}

//���ս������ȼ��ķ���
queue * inqueue_PRI(qnode *qn, queue*q)
{
	//�ҵ����ʵ�λ�ò����������,����󣬶����Ѿ�����
	qnode * tmp;
	//��һ��������еĺ���
	if (q->head = nullptr)
		inqueue(qn, q);
	else
	{
		for (tmp = q->head; tmp != nullptr; tmp = tmp->next)
		{
			//���ȼ���ֵԽС�����ȼ�Խ��
			if (qn->p->p_pir < tmp->p->p_pir)
			{
				//���ýڵ�Ϊͷ���
				if (tmp == q->head)
				{
					q->head = qn;
					qn->next = tmp;
					qn->pre = nullptr;
					tmp->pre = qn;
				}
				//���ýڵ�Ӧ����β��
				else if (tmp == q->tail)
				{
					qn->next = nullptr;
					qn->pre = tmp;
					tmp->next = qn;
					q->tail = qn;
				}

				else	//���ýڵ㲻��ͷ����λ��,Ҳ����β����λ��
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
