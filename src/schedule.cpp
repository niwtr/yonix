#include "process.h"
#include "lock.h"


//TODO proc=0�е�0������ָ����������
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
	if (proc->p_stat == SRUN)
		panic("sched running");
	inter_enable = cpu->intena;
	swtch(&proc->context, cpu->scheduler);
	//�л������ģ�������������scheduler���л�ʱ�����
  cpu->intena=intena;
}

//����CUP������Ȩ�������ʱ��Ƭ���ں�
void giveup_cpu(void)
{
	//������״̬�ı�Ĳ����У�����Ҫ�Ȼ�������Ա�֤�����г�ͻ����
	proc->p_stat = READY;
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
    if(p->p_stat==SSLEEPING && p->p_chan==e)
      p->p_stat=READY;
}

