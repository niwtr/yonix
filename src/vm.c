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
  void *virt;
  uint phys_start;
  uint phys_end;
  int perm;
} kmap[] = {
 { (void*)KERNBASE, 0,             EXTMEM,    PTE_W}, // I/O space
 { (void*)KERNLINK, V2P(KERNLINK), V2P(data), 0},     // kern text+rodata
 { (void*)data,     V2P(data),     PHYSTOP,   PTE_W}, // kern data+memory
 { (void*)DEVSPACE, DEVSPACE,      0,         PTE_W}, // more devices
};


// 获取一个新的页目录表
pde_t* setupkvm()
{

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

