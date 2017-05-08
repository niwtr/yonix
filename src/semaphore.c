#include "yotypes.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "semaphore.h"


//计数信号量

//初始化指定大小,指定申请单位的信号量
int sem_init(int num,sem * semaphore)
{
    if(num<0)
        panic("semaphre init failed!");
    *semaphore=num;
    return 0;
}

//等待信号量
int sem_P(int step,sem * semaphore)
{
    //关中断
    cli();
    if(step<0)
        panic("semaphre P error!");
    while (*semaphore <=(step-1))
    {
      cprintf("sleep,%p\n",semaphore);
        sleep(semaphore);
    }
    (*semaphore)-=step;
    //开中断
    sti();
    return 0;
}

int sem_V(int step,sem *semaphore)
{
    cli();
    if(step<0)
        panic("semaphre V error!");
    (*semaphore)+=step;
    // if there is any available lock, call wakeup

    if(*semaphore > 0)
        wakeup(semaphore);
    sti();
    return 0;
}

//二元信号量－mutex

//初始化指定大小,指定申请单位的信号量
int mutex_init(sem * semaphore)
{
    *semaphore=1;

    return 0;
}


//等待互斥信号量
int mutex_P(sem * semaphore)
{
    //关中断
    cli();
    while (*semaphore <=0)
    {
        //cprintf("sleep,%d\n",*semaphore);
        sleep(semaphore);
    }
    (*semaphore)--;
    //开中断
    sti();
    return 0;
}

int mutex_V(sem *semaphore)
{
    cli();
    (*semaphore)++;
    // if there is any available lock, call wakeup
    if(*semaphore > 0)
        wakeup(semaphore);
    sti();
    return 0;
}

