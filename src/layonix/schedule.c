#include "param_yonix.h"
#include "yotypes.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"


//non-private.
void switch_to (struct proc * p)
{
  proc = p;        //���õ�ǰ���յĽ��̣�ȫ�ֱ�������
  switchuvm(p);    //�����û������ڴ�
  p->p_stat = SRUN;//���ý���״̬
  //�������뿪��������������ת���û����̡�


  swtch(&cpu->scheduler, p->p_ctxt);
}

void select_scheme (int schem){
  cpu->scheme = schem;
  sched_reftable[cpu->scheme].init();

}
void sched_name(char * name){
  const char * schedname =sched_reftable[cpu->scheme].scheme_method;
  safestrcpy(name, schedname, strlen(schedname)+1);
}

//#include "process.h"
//proc=0��ʾ�������̱߳�����
//������scheduler
//CPU�ڳ�ʼ�����ߵ��øú���
//userproc --sched-->schedulerproc--sched-->userproc...
void scheduler(void)
{

	while (true)
    {
      sti(); //����ʱ��Ƭ�жϣ��жϺ�trap����yeild()��������

      //RR,�ҵ�����READY״̬�Ľ���


      //sched_rr();
      if(!sched_reftable[cpu->scheme].scheme()) //sched
        {
          continue;
        }

      //ĳ��ʱ��Ƭ�жϣ�pia��һ��CPU�ֻص����������

      //switch back.
      switchkvm();//FIXME
      //���õ�ǰ��������Ϊ��������
      //���ᷢ�����������Ĵ�������д��һ�������ϡ�����
      proc = 0;//���õ�ǰ���յĽ���Ϊ��������


    }
}



//���̴��û�̬�л���cpu������xv6�е�sched
void transform(void)
{

  //ִ��after���̡�
  //1. ���¼���ʱ��Ƭ��
  //2. ���¼��㶯̬���ȼ��������еĻ���

	/*
	���󣬵��� swtch �ѵ�ǰ�����ı����� proc->context ��Ȼ���л��������������ļ� cpu->scheduler ��
	*/
	if (proc->p_stat == SRUN)
		panic("sched running");//��shed����SRUN�Ľ���



	swtch(&proc->p_ctxt, cpu->scheduler);


}




void giveup_cpu(void)
{
	proc->p_stat = READY;
  // proc->p_time_slice = SCHED_RR_TIMESLICE;
  struct slot_entry * e = (struct slot_entry *)alloc_slab();
  e->slotptr = proc;
  Q_INSERT_TAIL(&rdyqueue, e, lnk);
  sched_reftable[cpu->scheme].after();

	transform();

}


void timeslice_yield(){
  if (proc->p_time_slice == ETERNAL)
    return ;
  proc->p_time_slice -= TIMER_INTERVAL;
  proc->p_avgslp = MAX(-50, proc->p_avgslp-1);
  if(proc->p_time_slice<=0)
      giveup_cpu();
}





/* sleep a proc on specific event and swtch away. */
void sleep(void * e)
{
  //tell event.
  uint tick0 = ticks;
  proc->p_chan=e;
  proc->p_stat=SSLEEPING;
  transform(); //swtch away.
  proc->p_chan=0; // when sched back (return from wakeup), tidy up.
  proc->p_avgslp = MIN(proc->p_avgslp+(tick0-ticks), MAX_AVGSLP);
}

void wakeup(void * e)
{
  //find specific proc that is sleep on specific event e (or chan)
  search_through_ptablef(p)
    if(p->p_stat==SSLEEPING && p->p_chan==e)
      {
        p->p_stat=READY;
        struct slot_entry * ee = (struct slot_entry*) alloc_slab();
        ee->slotptr = p;
        Q_INSERT_TAIL(&rdyqueue, ee, lnk);
      }
}







