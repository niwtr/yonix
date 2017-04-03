// 虚拟内存管理

#include "params.h"
#include "yotypes.h"
#include "def.h"
#include "mmu.h"
#include "memlayout.h"
#include "x86.h"
#include "proc.h"


// 内核加载过程中，最初的简单一级分页页表
// 虚拟地址[KERNBASE, KERNBASE+4MB)映射到物理地址[0, 4MB)
__attribute__((__aligned__(PGSIZE)))
pde_t initpgtb[NPDENTRIES] = {

	// 未开启PTE_W写权限
	[KERNBASE>>PDXSHIFT] = (0) | PTE_P | PTE_PS
};


// 最终物理内存分布，或者说虚拟内存中内核区域分布
// 用户申请内存由内核帮助分配，在下图第三个部分
static struct kmap {
	vaddr_t virt_start;
	paddr_t phys_start;
	uint size;
	int perm;
} kmap[] = {
	{ (vaddr_t)KERNBASE, 	0,             EXTMEM,    				PTE_W}, // I/O space
	{ (vaddr_t)KERNLINK, 	V2P(KERNLINK), V2P(data)-V2P(KERNLINK), 0},     // kern text+rodata
	{ (vaddr_t)data,     	V2P(data),     PHYSTOP-V2P(data),   	PTE_W}, // kern data+memory
	{ (vaddr_t)DEVSPACE, 	DEVSPACE,      MEMTOP-DEVSPACE,    		PTE_W}, // more devices
};


// 遍历查找对应二级页表项，
// alloc表示找不到二级页表时是否创建新的页表
pte_t * walkpgdir(pde_t *pgdir, vaddr_t va, int alloc)
{
	pde_t *pde;
	pte_t *pgtab;

	pde = &pgdir[PDX(va)];
	if(*pde & PTE_P == 0) {
		if(alloc == 0 || (pgtab = (pte_t*)kalloc()) == 0)	// 申请一页存储页表
			return NULL;

		// 通过判断PTE_P是否为0获知是否重复映射
		memset(pgtab, 0, PGSIZE);

		// 只是二级页表的访问权限，更加精细的页面访问权限在下一级控制
		*pte = V2P(pgtab) | PTE_P | PTE_W | PTE_U;

	}
	else {
		pgtab = PTE_ADDR(*pde);
	}

	return &pgtab[PTX(va)];
}


// 对于一段相同权限的内存，创建二级页表映射
int mappages(pde_t *pgdir, vaddr_t va, uint size, paddr_t pa, int perm)
{
	vaddr_t bottom = PGROUNDDOWN((uint)va);
	vaddr_t top = PGROUNDDOWN((uint)va + size - 1);	// 表示最后一页基地址

	pte_t *pte = NULL;

	while(bottom <= top) {
		if((pte = walkpgdir(pgdir, bottom, 1)) == 0)
			return 0;

		// 重复的映射
		if(*pte & PTE_P)
			panic("Duplicate mapping")

		*pte = pa | perm | PTE_P;

		bottom += PGSIZE;
		pa += PGSIZE;
	}

	return 1;
}


// 获取一个新的二级页表，并包含内核所有映射
pde_t* setupkvm()
{
	// 物理内存过大
	if(V2P(PHYSTOP) > DEVSPACE)
		panic("PHYSICAL MEMORY TOO BIG");

	pde_t *pgdir = NULL;
	if((pgdir = (pde_t *)kalloc()) == 0)
		return 0;

	memset(pgdir, 0, PGSIZE);

	int i;
	for(i = 0; i < NELEM(kmem); i++)
		if(mappages(pgdir, kmem[i].virt_start, kmem.size, kmem.phys_start, kmem.perm) == 0)
			return NULL;

	return pgdir;
}


// 为内核申请新的页表
void kvmalloc()
{

}


// 切换到内核页表
void switchkvm()
{

}


// 用户进程页表切换
void switchuvm()
{

}


// 初始化用户进程虚拟地址空间
void inituvm()
{

}


// 为用户进程分配物理内存并设置页表
int allocuvm()
{

}


// 为用户进程释放部分物理内存
int inallocuvm()
{

}


// 释放用户全部地址空间
void freeuvm()
{

}


// 加载文件到内存，并设置用户页表，exec()基础
int loaduvm()
{

}


// 复制用户地址空间内容到新物理内存，fork()基础
pde_t* copyuvm()
{

}

