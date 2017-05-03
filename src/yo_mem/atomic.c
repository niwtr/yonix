#include "yotypes.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"






//基本运算操作
int atomic_add(int a, int b, int * c)
{
    cli();  //关中断
    *c=a+b;
    sti();
    return 0;
}

int atomic_sub(int a, int b, int * c)
{
    cli();  //关中断
    *c=a-b;
    sti();
    return 0;
}

int atomic_multi(int a, int b, int * c)
{
    cli();  //关中断
    *c=a*b;
    sti();
    return 0;
}

int atomic_divide(int a, int b, int * c)
{
    cli();  //关中断
    *c=a/b;
    sti();
    return 0;
}

int atomic_mod(int a, int b, int * c)
{
    cli();  //关中断
    *c=a%b;
    sti();
    return 0;
}


//其他操作
int atomic_set(int * a, int b)
{
    cli();  //关中断
    (*a)=b;
    sti();
    return 0;
}

int atomic_swap(int * a, int * b)
{
    cli();  //关中断
    int temp;
    temp=(*a);
    (*a)=(*b);
    (*b)=temp;
    sti();
    return 0;
}



/*
//队列的一些操作
void atomic_inqueue(qnode *qn,queue * q)
{
    cli();  //关中断
    in_queue(qn, q);
    sti();
    return;
}

void atomic_delqnode(qnode *qn,queue * q)
{
    cli();  //关中断
    del_qnode(qn, q);
    sti();
    return;
}
*/