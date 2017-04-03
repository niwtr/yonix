//物理内存分配器，分页大小4KB


#include "yotypes.h"
#include "mmu.h"
#include "memlayout.h"


//空闲内存页链表，占用空间满足小于页面大小4kb
struct fpage {
	struct fpage *next;
};


//内存池
struct {
	struct fpage *freelist;
} kmem;


//注：以下过程输入均为虚拟内存地址

//释放一定范围内内存页
void freerange(vaddr_t vstart, vaddr_t vend)
{
	vaddr_t p;
	p = (vaddr_t)PGROUNDUP((uint)vstart);	//页面向上对齐，保护数据
	for(; p < (vaddr_t)vend; p += PGSIZE)
		kfree(p);
}

/*
//使用entrypgdir页表时，初始化内存,不加锁
void kinit1(void *vstart, void *vend)
{
	freerange(vstart, vend);
}


//使用完整页表时，初始化内存，加锁
void kinit2(void *vstart, void *vend)
{
	freerange(vstart, vend);
}
*/

//释放某个内存页
void kfree(vaddr_t vaddr)
{
	// 内存页基地址不是页数整数倍，或者不在有效区域
	if((uint)vaddr % PGSIZE || v < end || V2P(v) >= PHYSTOP)
		panic("kfree");

	// 多核时申请锁

	// 使用了强制类型转换，结构体地址即等于空闲页地址
	struct fpage *f;
	f = (struct fpage*)vaddr;

	f->next = kmem.freelist;
	kmem.freelist = f;

	// 多核时释放锁
}


//申请一页内存,失败返回NULL
vaddr_t kalloc()
{
	// 多核时申请锁

	struct fpage *f;
	f = kmem.freelist;
	if(f)
		kmem.freelist = f->next;

	// 多核时释放锁
	
	return f;
}