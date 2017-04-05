#include "lock.h"
#include "x86.h"
#include "mmu.h"


//��ʼ��һ����
void initlock(struct spinlock *slk, char *lname)
{
	slk->l_name = lname;
	slk->s_locked = 0;
	slk->l_cpu = 0;
}

//�˶Ը�CPU�Ƿ������
//��Ϊ�棬�򷵻�1
int ishold(struct spinlock *lock)
{
	return lock->s_locked&&lock->l_cpu == cpu;
}

//��֤�����������ԭ����
//���ƻ��ָ���е�cli������ֹ�жϷ���
void forbidtrap(void)
{
	int eflags;

	eflags = readeflags();
	cli();
	if (cpu->ncli == 0)
		cpu->intena = eflags & FL_IF;
	cpu->ncli += 1;
}

//�����жϷ���
void promisetrap(void) {
	if (readeflags()&FL_IF)
		//panic("popcli - interruptible");
	if (--cpu->ncli < 0)
		//panic("popcli");
	if (cpu->ncli == 0 && cpu->intena)
		sti();
}

//�����
//ѭ���ҵ����õ���
void acquirelock(struct spinlock *slk)
{
	forbidtrap();//��ֹ�жϷ������Ա�֤������ԭ���ԣ���������
	if (ishold(slk))
	{
		//panic("acquire");
		//����cpu�Ѿ�����������˳�
	}

	//����ѭ������cpu
	while (xchg(&slk->s_locked, 1) != 0);

	//��֪C�������ʹ������ڸô���֮��Ҫ�����κ����ݵ���CPU����֤�ٽ���δ��ʹ��
	//__sync_synchronize();

	//��¼�������cpu��Ϣ���Ա����
	slk->l_cpu = cpu;
	getcallerpcs(&slk, slk->l_pcs);
}

//�ͷ���
void releaselock(struct spinlock *slk)
{
	//�ж����Ƿ�ռ�ã��������Ѿ��ͷţ���ֱ���˳�����
	if (!ishold(slk));
		//painc("release");
	//�����ͷŸ�cpu��ռ�õ���

	slk->l_pcs[0] = 0;//Ϊʲôֻ��0���±ꣿ��
	slk->l_cpu = 0;

	__sync_synchronize();

	//�ͷ�����ʹ�û�����ԣ��Ա�֤������ԭ����
	asm volatile("movl $0, %0" : "+m" (lk->locked) : );

	//�ͷ���֮�󣬿������жϵķ���
	promisetrap();

};

//�ж���ԭ���Ե����𣿣�

//Ӳ����ز���
//��¼�µ�ǰ��������ջ
void getcallerpcs(void *v, uint pcs[])
{
	uint *ebp;
	int i;

	ebp = (uint*)v - 2;
	for (i = 0; i < 10; i++) {
		if (ebp == 0 || ebp < (uint*)KERNBASE || ebp == (uint*)0xffffffff)
			break;
		pcs[i] = ebp[1];     // saved %eip
		ebp = (uint*)ebp[0]; // saved %ebp
	}
	for (; i < 10; i++)
		pcs[i] = 0;
}
