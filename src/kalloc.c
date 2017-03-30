//物理内存分配器，分页大小4KB


#include "yotypes.h"
#include "mmu.h"
#include "memlayout.h"


//空闲内存页链表
struct fpage {
	struct fpage *next;
};


//内存池
struct {
	struct fpage *freelist;
} kmem;


//注：以下过程输入均为虚拟内存地址

//释放一定范围内内存页
void freerange(void *vstart, void *vend)
{

}


//使用entrypgdir页表时，初始化内存
void kinit1(void *vstart, void *vend)
{

}


//使用完整页表时，初始化内存
void kinit2(void *vstart, void *vend)
{

}


//释放某个内存页
void kfree(char *vaddr)
{

}


//申请一页内存
char* kalloc()
{

}