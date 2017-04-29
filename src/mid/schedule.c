#include "param_yonix.h"
#include "yotypes.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"

//#include "process.h"
//proc=0��ʾ�������̱߳���
//������scheduler
//CPU�ڳ�ʼ����ߵ��øú���
//userproc --sched-->schedulerproc--sched-->userproc...
void scheduler(void)
{

	while (true)
	{
		sti(); //����ʱ��Ƭ�жϣ��жϺ�trap����yeild()��������

		//RR,�ҵ�����READY״̬�Ľ���
		search_through_ptablef(p)
		{
			if (p->p_stat == READY)
			{
				//�л���״̬ΪRUNNING

				switchuvm(p);    //�����û������ڴ�
        proc = p;        //���õ�ǰ���յĽ��̣�ȫ�ֱ�������
				p->p_stat = SRUN;//���ý���״̬
        //�������뿪��������������ת���û����̡�
				swtch(&cpu->scheduler, p->p_ctxt);
        //ĳ��ʱ��Ƭ�жϣ�pia��һ��CPU�ֻص����������
				switchkvm();//FIXME
        //���õ�ǰ��������Ϊ��������
        //��ᷢ����������Ĵ������д��һ�������ϡ�����
				proc = 0;//���õ�ǰ���յĽ���Ϊ��������

			}
		}

	}
}



//���̴��û�̬�л���cpu������xv6�е�sched
void transform(void)
{

	/*
	��󣬵��� swtch �ѵ�ǰ�����ı����� proc->context ��Ȼ���л��������������ļ� cpu->scheduler ��
	*/
	if (proc->p_stat == SRUN)
		panic("sched running");//��shed����SRUN�Ľ���

	swtch(&proc->p_ctxt, cpu->scheduler);


}

//����CUP������Ȩ�������ʱ��Ƭ���ں�
//����������Ҫ���¸ý��̵�ʱ��Ƭ��
void giveup_cpu(void)
{

	//������״̬�ı�Ĳ����У�����Ҫ�Ȼ�������Ա�֤�����г�ͻ����
	proc->p_stat = READY;
  proc->p_time_slice = SCHED_RR_TIMESLICE;

	transform();

}


//�������ͨ�����жϽ��롣
void timeslice_yield(){
  proc->p_time_slice -= TIMER_INTERVAL;
  cprintf("\nPID %d Rest time slice:%d\n",proc->p_pid, proc->p_time_slice);
  if(proc->p_time_slice<=0) // ʱ��Ƭ�����ˣ�����giveup��ִ���������л�����������
      giveup_cpu();
  else
    ; // ���ʱ��Ƭû�����ֱ꣬�ӷ���
}





/* sleep a proc on specific event and swtch away. */
void sleep(void * e)
{
  //tell event.
  proc->p_chan=e;
  proc->p_stat=SSLEEPING;
  transform(); //swtch away.
  proc->p_chan=0; // when sched back (return from wakeup), tidy up.
}

void wakeup(void * e)
{
  //find specific proc that is sleep on specific event e (or chan)
  search_through_ptablef(p)
    if(p->p_stat==SSLEEPING && p->p_chan==e)
      p->p_stat=READY;
}







