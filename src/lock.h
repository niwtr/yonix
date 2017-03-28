#pragma once
#include "yotypes.h"

//定义锁大的数据结构
struct spinlock {
	uint s_locked;

	//调试信息定义
	char *l_name;	//锁的名字
	struct cpu *l_cpu;//持有锁大的CPU
	uint l_pcs[10];	//调用锁的栈
};

//初始化一个锁
void initlock(struct spinlock *slk, char *lname);

//核对该CPU是否持有锁
//若为真，则返回1
int ishold(struct spinlock *lock);

//保证获得锁操作的原子性
//类似汇编指令中的cli——禁止中断发生
void forbidtrap(void);

//允许中断发生
void promisetrap(void);

//获得锁
//循环找到可用的锁
void acquirelock(struct spinlock *slk);

//释放锁
void releaselock(struct spinlock *slk);

//硬件相关操作
//记录下当前调用锁的栈
void getcallerpcs(void *v, uint pcs[]);
