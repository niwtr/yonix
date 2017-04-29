#include "param_yonix.h"
#include "yotypes.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"



private void switch_to (struct proc * p)
{
  switchuvm(p);    //�����û������ڴ�
  proc = p;        //���õ�ǰ���յĽ��̣�ȫ�ֱ�������
  p->p_stat = SRUN;//���ý���״̬
  //�������뿪��������������ת���û����̡�
  swtch(&cpu->scheduler, p->p_ctxt);
}


void sched_fifo (void)
{

  if(proc && proc->p_stat==READY)
    switch_to(proc);

  //find proc with smallest creatime.
  struct proc * min = 0;
  search_through_ptablef(p){
    if(p->p_stat == READY){
      if(min==0)
        min=p;
      else
        if(min->p_creatime > p->p_creatime)
          min = p;
    }
  }
  if(min)
    switch_to(min);
  /*
  static struct proc * p = ptable.proc ;
  while(p<&ptable.proc[PROC_NUM]){
    if(p->p_stat != READY){
      p++;
      continue;
    }
    switch_to(p);
    p++;
    return ;
  }
  p=ptable.proc; // wind back.
  */
}
void sched_fifo_after(void)
{
  ; //do nothing.
}
void sched_fifo_timeslice(struct proc * p)
{
  p->p_time_slice = SCHED_FIFO_TIMESLICE; //forever.
}
/*
 * ���ȼ����ȡ��ҵ����ȼ����Ľ��̽��е��ȡ�
 * ��̬���ȼ�ʹ��nice�����㣺nice+120
 * ÿ�����̶������費ͬ��ʱ��Ƭ��
 * ʱ��Ƭ�ļ����þ�̬���ȼ���
 */
void sched_priority(void)
{
  struct proc * p_primax = 0 ;
  search_through_ptablef(p){
    if(p->p_stat == READY) {
      if(p_primax == 0)
        p_primax = p;
      else
        if(p_primax->p_dpri > p->p_dpri) // �ҵ���һ�����ȼ����ߵġ�ע��dpriԽ�ͣ����ȼ�Խ�ߡ�
          p_primax = p;
    }
  }
  if(p_primax) //�ҵ��˽��� to swtich
    switch_to(p_primax);
}
void sched_priority_timeslice(struct proc *p)
{

  p->p_spri = STATIC_PRI(p->p_nice);
  p->p_time_slice = TIME_SLICE(p->p_spri);
}

void sched_priority_after(void)
{
  sched_priority_timeslice(proc);
  proc->p_dpri = DYNAMIC_PRI(proc->p_spri, BONUS(proc->p_avgslp));
  cprintf("pid %d dpri %d\n", proc->p_pid, proc->p_dpri);
}

//RR���ȡ�
void sched_rr (void)
{
  static struct proc * p=ptable.proc;
  while(p<&ptable.proc[PROC_NUM]){
    if(p->p_stat != READY){
      p++;
      continue;
    }
    switch_to(p);
    p++;
    return ;
  }
  p=ptable.proc; // wind back.
}

void sched_rr_timeslice(struct proc * p){
  p->p_time_slice= SCHED_RR_TIMESLICE;
}

void sched_rr_after(void){
  sched_rr_timeslice(proc);
}

struct sched_refstruct sched_reftable[SCHEME_NUMS]={
  [SCHEME_FIFO]= {sched_fifo, sched_fifo_after, sched_fifo_timeslice},
  [SCHEME_RR]  = {sched_rr, sched_rr_after, sched_rr_timeslice},
  [SCHEME_PRI] = {sched_priority, sched_priority_after, sched_priority_timeslice}

};


void select_scheme (int schem){
  cpu->scheme = schem;
}

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

      //sched_rr();
      sched_reftable[cpu->scheme].scheme(); //sched

      //ĳ��ʱ��Ƭ�жϣ�pia��һ��CPU�ֻص����������

      //switch back.
      switchkvm();//FIXME
      //���õ�ǰ��������Ϊ��������
      //��ᷢ����������Ĵ������д��һ�������ϡ�����
      proc = 0;//���õ�ǰ���յĽ���Ϊ��������


    }
}



//���������õ�scheduler����������չ����
void scheduler1(void)
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

  //ִ��after���̡�
  //1. ���¼���ʱ��Ƭ��
  //2. ���¼��㶯̬���ȼ�������еĻ���

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
  // proc->p_time_slice = SCHED_RR_TIMESLICE;
  sched_reftable[cpu->scheme].after();

	transform();

}


//�������ͨ�����жϽ��롣
void timeslice_yield(){
  if (proc->p_time_slice == ETERNAL)
    return ; // �ý��̵�ʱ��Ƭ�����ֱ�ӷ��ء�
  proc->p_time_slice -= TIMER_INTERVAL;
  proc->p_avgslp -= 1; //��avgslp�ݼ�1��tick

  if(proc->p_time_slice<=0) // ʱ��Ƭ�����ˣ�����giveup��ִ���������л�����������
    {
      cprintf("\nPID %d Used up its time slice.\n",proc->p_pid);
      giveup_cpu();
    }
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







