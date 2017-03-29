#include "process.h"
#include "lock.h"

//semaphore_wait
//slkΪ�����������е������Ա�֤���������޷�����signal
//Ϊʲô��������������
void sem_wait(void *w_queue, struct spinlock *slk)
{
	if (proc == 0)
		painc("sleep:process is not exit!");
	if (slk == 0)
		painc("sleep:the caller don't have lock!");

	//���̴��ڣ��ҵ������ĺ���������slk
	//��slk!=&ptable.lock,�򵱽��̳���ptable.lock�󣬱�ɰ�ȫ�ͷ�slk
	//��Ϊsignal�������ڳ�����������²���ִ��
	if (slk != &ptable.lock)
	{
		acquirelock(&ptable.lock);
		releaselock(slk);
	}
	//��slk==&ptable.lock
	//��ʱ��Ҫ����� ptable.lock Ȼ���ְ�����Ϊ slk �ͷŵ�ʱ������������
	//���Բ�ִ������if����������

	//ʹ���̽�������״̬
	proc->p_chan = w_queue;
	proc->p_stat = SSLEEPING;
	//����transform(),���û�̬�л���cpu������״̬
	transform();

	//���㣿��
	proc->p_chan = 0;

	//�������������ȵ����������֮ǰ����
	if (slk != &ptable.lock)
	{
		releaselock(&ptable.lock);
		acquirelock(slk);
	}
}


//semaphore_broadcast
//�������д���˯��״̬�Ľ���
void sem_broadcast(void *w_queue)
{
	acquirelock(&ptable.lock);
	wakeup(w_queue);
	releaselock(&ptable.lock);
}

//�ڲ�����
static void wakeup(void *w_queue)
{
	struct proc *pro;

	//�������̱��������н���
	for (pro = ptable.proc; pro < &ptable.proc[PROC_NUM]; pro++)//�ɲ�������pro<PROC_NUM??
	{
		if (pro->p_stat == SSLEEPING && pro->p_chan == w_queue)
			pro->p_stat = READY;
	}
}

//������scheduler
//ÿ��CPU�ڳ�ʼ����ߵ��øú���
//�ӵ�����״̬�л����û�̬���ںˣ�
void scheduler(void)
{
	struct proc *p;
	while (true)
	{
		sti();//����ʱ��Ƭ�жϣ��жϺ�trap����yeild()����

			  //��ת��ѯ,�ҵ�����READY״̬�Ľ���
			  //��������Է�ֹ�����ͬʱִ�д˴�
		acquirelock(&ptable.lock);
		for (p = ptable.proc; p < &ptable.proc[PROC_NUM]; p++)//��ʼ����ʱ���ʼ����PROC_NUM�������𣿣�
		{
			if (p->p_stat == READY)
			{
				//�л���״̬ΪRUNNING
				proc = p;
				switchvm(p);//���������ڴ�
				p->p_stat = SRUN;
				swtch(&cpu->scheduler, p->p_ctxt);
				switchvm();//??

				proc = 0;//??
			}
		}
		releaselock(&ptable.lock);//�ͷ���

	}
}


//���̴��û�̬�л���cpu������xv6�е�sched
void transform(void)
{
	int inter_enable;
	/*
	�ú������������״̬�������״̬�������ڽ��̴�ʱ������������ CPU Ӧ�������жϹرյ���������еġ�
	��󣬵��� swtch �ѵ�ǰ�����ı����� proc->context ��Ȼ���л��������������ļ� cpu->scheduler ��
	*/
	if (!ishold(&ptable.lock))				//�ж��Ƿ�����
		panic("sched ptable.lock");
	if (cpu->ncli != 1)						//Depth of pushcli nesting =1��ζ��ʲô�ϣ�
		panic("sched locks");
	if (proc->p_stat == SRUN)
		panic("sched running");
	if (readeflags()&FL_IF)					//��ȡ�жϱ�ʶ
		panic("sched interruptible");
	inter_enable = cpu->intena;
	swtch(&proc->context, cpu->scheduler);
	//�л������ģ�������������scheduler���л�ʱ�����
	cpu->intena = inter_enable;
}

//����CUP������Ȩ�������ʱ��Ƭ���ں�
void giveup_cpu(void)
{
	//������״̬�ı�Ĳ����У�����Ҫ�Ȼ�������Ա�֤�����г�ͻ����
	acquirelock(&ptable.lock);
	proc->p_stat = READY;
	transform();
	releaselock(&ptable.lock);
}
