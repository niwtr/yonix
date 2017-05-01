//物理内存分配器，分页大小4KB

#include "yotypes.h"
#include "param_yonix.h"
#include "mmu.h"
#include "memlayout.h"
#include "defs.h"

extern char end[]; // first address after kernel loaded from ELF file

//空闲链表，结构体占用空间满足小于size
struct fpage
{
	uint size;		// 页大小或者slab大小
	struct fpage *next;
};

// 4kb页内存池
struct
{
	uint nfreeblock;
	struct fpage *freelist;
} kmem;

// 32byte内存池
struct
{
	uint nfreeblock;
	struct fpage *freelist;
} slab;

//注：以下过程输入均为虚拟内存地址

//释放一定范围内内存页
void freerange(vaddr_t vstart, vaddr_t vend)
{
	vaddr_t p;
	p = (vaddr_t)PGROUNDUP((uint)vstart); //页面向上对齐，保护数据
	for (; p < (vaddr_t)vend; p += PGSIZE)
		kfree(p);
}


//使用entrypgdir页表时，初始化内存
void kinit1(void *vstart, void *vend)
{
	freerange(vstart, vend);
}


//使用完整页表时，初始化内存
void kinit2(void *vstart, void *vend)
{
	// freerange(vstart, vend);
	slabinit();

	cprintf("free memory: %dMB\n", kmem.nfreeblock*4/1024);
}


//释放某个内存页
void kfree(vaddr_t v)
{
	// 内存页基地址不是页数整数倍，或者不在有效区域
	if ((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
		panic("kfree");

	// 使用了强制类型转换，结构体地址即等于空闲页地址
	struct fpage *f;
	f = (struct fpage *)v;

	f->size = PGSIZE;
	f->next = kmem.freelist;

	kmem.freelist = f;
	kmem.nfreeblock++;
}

//申请一页内存,失败返回NULL
vaddr_t kalloc()
{
	if (kmem.nfreeblock == 0)
		page_out();

	struct fpage *f;
	f = kmem.freelist;
	if (f)
		kmem.freelist = f->next;

	kmem.nfreeblock--;

	if(!f)
		cprintf("kalloc: out of memory\n");

	return (vaddr_t)f;
}

// 小内存池初始化，用于动态申请32-byte slab
void slabinit()
{
	vaddr_t p = kalloc();
	vaddr_t start = p;
	while (p + SLABSIZE < start + PGSIZE)
	{
		free_slab(p);
		p += SLABSIZE;
	}
}

// 释放一个slab
void free_slab(vaddr_t v)
{
	struct fpage *p = (struct fpage *)v;
	p->size = SLABSIZE;
	p->next = slab.freelist;

	slab.freelist = p;
	slab.nfreeblock++;
}

// 申请一个slab
vaddr_t alloc_slab()
{
	if(slab.nfreeblock == 0)
		slabinit();
	
	struct fpage *f = slab.freelist;
	if(f)
		slab.freelist = f->next;
	slab.nfreeblock--;

	return (vaddr_t)f;
}