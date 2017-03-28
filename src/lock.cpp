#include "lock.h"
#include "x86.h"
#include "mmu.h"


//初始化一个锁
void initlock(struct spinlock *slk, char *lname)
{
	slk->l_name = lname;
	slk->s_locked = 0;
	slk->l_cpu = 0;
}

//核对该CPU是否持有锁
//若为真，则返回1
int ishold(struct spinlock *lock)
{
	return lock->s_locked&&lock->l_cpu == cpu;
}

//保证获得锁操作的原子性
//类似汇编指令中的cli——禁止中断发生
void forbidtrap(void)
{
	int eflags;

	eflags = readeflags();
	cli();
	if (cpu->ncli == 0)
		cpu->intena = eflags & FL_IF;
	cpu->ncli += 1;
}

//允许中断发生
void promisetrap(void) {
	if (readeflags()&FL_IF)
		//panic("popcli - interruptible");
	if (--cpu->ncli < 0)
		//panic("popcli");
	if (cpu->ncli == 0 && cpu->intena)
		sti();
}

//获得锁
//循环找到可用的锁
void acquirelock(struct spinlock *slk)
{
	forbidtrap();//禁止中断发生，以保证操作的原子性，避免死锁
	if (ishold(slk))
	{
		//panic("acquire");
		//若该cpu已经获得锁，则退出
	}

	//否则，循环遍历cpu
	while (xchg(&slk->s_locked, 1) != 0);

	//告知C编译器和处理器在该代码之后不要加载任何数据到该CPU，保证临界区未被使用
	//__sync_synchronize();

	//记录获得锁的cpu信息，以便调试
	slk->l_cpu = cpu;
	getcallerpcs(&slk, slk->l_pcs);
}

//释放锁
void releaselock(struct spinlock *slk)
{
	//判断锁是否被占用，若本身已经释放，则直接退出即可
	if (!ishold(slk));
		//painc("release");
	//否则，释放该cpu所占用的锁

	slk->l_pcs[0] = 0;//为什么只是0号下标？？
	slk->l_cpu = 0;

	__sync_synchronize();

	//释放锁，使用汇编语言，以保证操作的原子性
	asm volatile("movl $0, %0" : "+m" (lk->locked) : );

	//释放锁之后，可允许中断的发生
	promisetrap();

};

//中断与原子性的区别？？

//硬件相关操作
//记录下当前调用锁的栈
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
