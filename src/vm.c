// 虚拟内存管理

#include "param_yonix.h"
#include "queue_netbsd.h"
#include "yotypes.h"
#include "def.h"
#include "mmu.h"
#include "memlayout.h"
#include "x86.h"
#include "proc.h"

// swap_map偏移映射到交换区section编号
#define SWAPM2SECNO(i) ((uint)(i) << 3)
#define SECNO2SWAPM(i) ((uint)(i) >> 3)

typedef Q_HEAD(page_entry_list, page_entry) q_head;
typedef Q_ENTRY(page_entry) q_entry;

// 页队列项
struct page_entry
{
	pte_t *ptr_pte;
	// we can't actually use pointer to pte
	// because in page_out, it's not the same page table
	uint pn_pid; // 页号按位与pid，作为交换区页标识符
	uint ref;	// reference bit
	q_entry link;
};

q_head pgqueue;			 // 内存页队列（物理页）
uint swap_map[SLOTSIZE]; // 交换空间映射表

extern char data[]; // defined by kernel.ld
struct segdesc gdt[NSEGS];

// 内核页表
pde_t *kpgdir = NULL;

// 内核加载过程中，最初的简单一级分页页表
// 虚拟地址[KERNBASE, KERNBASE+4MB)映射到物理地址[0, 4MB)
__attribute__((__aligned__(PGSIZE)))
pde_t initpgtb[NPDENTRIES] = {
		// 未开启PTE_W写权限
		[KERNBASE >> PDXSHIFT] = (0) | PTE_P | PTE_PS};

// 最终物理内存分布，或者说虚拟内存中内核区域分布
// 用户申请内存由内核帮助分配，在下图第三个部分
static struct kmap
{
	vaddr_t virt_start;
	paddr_t phys_start;
	uint size;
	uint perm;
} kmap[] = {
	{(vaddr_t)KERNBASE, 0, EXTMEM, PTE_W},							  // I/O space
	{(vaddr_t)KERNLINK, V2P(KERNLINK), V2P(data) - V2P(KERNLINK), 0}, // kern text+rodata
	{(vaddr_t)data, V2P(data), PHYSTOP - V2P(data), PTE_W},			  // kern data+memory
	{(vaddr_t)DEVSPACE, DEVSPACE, MEMTOP - DEVSPACE, PTE_W},		  // more devices
};

// Set up CPU's kernel segment descriptors.
// Run once on entry on each CPU.
void seginit(void)
{
	struct cpu *c;

	// Map "logical" addresses to virtual addresses using identity map.
	// Cannot share a CODE descriptor for both kernel and user
	// because it would have to have DPL_USR, but the CPU forbids
	// an interrupt from CPL=0 to DPL=3.
	c = &cpus[cpunum()];
	c->gdt[SEG_KCODE] = SEG(STA_X | STA_R, 0, 0xffffffff, 0);
	c->gdt[SEG_KDATA] = SEG(STA_W, 0, 0xffffffff, 0);
	c->gdt[SEG_UCODE] = SEG(STA_X | STA_R, 0, 0xffffffff, DPL_USER);
	c->gdt[SEG_UDATA] = SEG(STA_W, 0, 0xffffffff, DPL_USER);

	// Map cpu, and curproc
	c->gdt[SEG_KCPU] = SEG(STA_W, &c->cpu, 8, 0);

	lgdt(c->gdt, sizeof(c->gdt));
	loadgs(SEG_KCPU << 3);

	// Initialize cpu-local storage.
	cpu = c;
	proc = 0;
}

// 遍历查找对应二级页表项，
// alloc表示找不到二级页表时是否创建新的页表
pte_t *walkpgdir(pde_t *pgdir, vaddr_t va, int alloc)
{
	pde_t *pde;
	pte_t *pgtab;

	pde = &pgdir[PDX(va)];
	if (*pde & PTE_P == 0)
	{
		if (alloc == 0 || (pgtab = (pte_t *)kalloc()) == 0) // 申请一页存储页表
			return NULL;

		// 通过判断PTE_P是否为0获知是否重复映射
		memset(pgtab, 0, PGSIZE);

		// 只是二级页表的访问权限，更加精细的页面访问权限在下一级控制
		*pte = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
	}
	else
	{
		pgtab = PTE_ADDR(*pde);
	}

	return &pgtab[PTX(va)];
}

// 对于一段相同权限的内存，创建二级页表映射
int mappages(pde_t *pgdir, vaddr_t va, uint size, paddr_t pa, uint perm)
{
	vaddr_t bottom = PGROUNDDOWN((uint)va);
	vaddr_t top = PGROUNDDOWN((uint)va + size - 1); // 表示最后一页基地址

	pte_t *pte = NULL;

	while (bottom <= top)
	{
		if ((pte = walkpgdir(pgdir, bottom, 1)) == 0)
			return 0;

		// 重复的映射
		if (*pte & PTE_P)
			panic("Duplicate mapping")

				*pte = pa | perm | PTE_P;

		bottom += PGSIZE;
		pa += PGSIZE;
	}

	return 1;
}

// 获取一个新的二级页表，并包含内核所有映射
pde_t *setupkvm()
{
	// 物理内存过大
	if (V2P(PHYSTOP) > DEVSPACE)
		panic("PHYSICAL MEMORY TOO BIG");

	pde_t *pgdir = NULL;
	if ((pgdir = (pde_t *)kalloc()) == 0)
		return 0;

	memset(pgdir, 0, PGSIZE);

	int i;
	for (i = 0; i < NELEM(kmem); i++)
		if (mappages(pgdir, kmem[i].virt_start, kmem.size, kmem.phys_start, kmem.perm) == 0)
			return NULL;

	return pgdir;
}

// 为内核申请新的页表, 启动时使用
void kvmalloc()
{
	kpgdir = setupkvm();
	switchkvm();
}

// 切换到内核页表
void switchkvm()
{
	lcr3(V2P(kpgdir));
}

// 用户进程页表切换
void switchuvm()
{
}

// 初始化用户进程虚拟地址空间
void inituvm()
{
}

// 用户进程增长物理内存，从startva到endva（startva<endva）
int allocuvm(pde_t *pgdir, uint startva, uint endva)
{
	if (endva > KERNBASE || endva < startva)
		return 0;

	int badflag = 0; //出错标志
	vaddr_t mem;
	uint i;
	for (i = PGROUNDUP(startva); i < endva; i += PGSIZE) // 循环申请物理内存并建立用户页表映射
	{
		mem = kalloc();
		if (mem == NULL)
		{
			badflag = 1;
			break;
		}

		memset(mem, 0, PGSIZE);

		if (mappages(pgdir, i, PGSIZE, V2P(mem), PTE_P | PTE_W | PTE_U) == 0)
		{
			kfree(mem);
			badflag = 1;
			break;
		}
	}

	if (badflag)
	{
		cprintf("OUT OF MEMORY");
		deallocuvm(pgdir, endva, startva); // 恢复原始内存大小
		return 0;
	}

	return 1;
}

// 为用户进程释放部分物理内存，从startva到endva（startva>endva）
int deallocuvm(pde_t *pgdir, uint startva, uint endva)
{
	if (endva < 0 || startva < endva)
		return 0;

	pte_t *pte = NULL;
	uint i;
	for (i = PGROUNDUP(endva); i < startva; i += PGSIZE)
	{
		pte = walkpgdir(pgdir, i, 0);
		if (!pte || ((*pte) & PTE_P) == 0) // **可优化**
			continue;

		paddr_t pa = PTE_ADDR(*pte);
		kfree(P2V(pa));

		*pte = 0; // 对应页表项清零
	}

	return 1;
}

// 释放用户进程全部地址空间
void freeuvm(pde_t *pgdir)
{
	if (pgdir == NULL)
		panic("ALREADY FREEED");

	deallocuvm(pgdir, KERNBASE, 0);

	// 释放二级页表所占内存空间
	int i;
	for (i = 0; i < NPDENTRIES; i++)
	{
		pde_t *pde = &pgdir[i];
		if (*pde & PDE_P)
		{
			va = P2V(PTE_ADDR(*pde));
			kfree(va);
		}
	}

	kfree((vaddr_t)pgdir);
}

// 加载文件到内存，并设置用户页表，exec()基础
int loaduvm()
{
}

// 复制用户地址空间内容到新物理内存，fork()基础
pde_t *copyuvm(pde_t *spgdir)
{
	pde_t *tpgdir = NULL;
	if ((tpgdir = setupkvm()) == NULL)
		return 0;

	// 复制用户地址空间所有内存
	int badflag = 0;
	paddr_t i;
	pte_t *pte = NULL;
	for (i = 0; i < KERNBASE; i += PGSIZE) // 用户地址空间连续分配虚拟内存时，可以优化
	{
		if ((pte = walkpgdir(spgdir, i, 0)) && (*pte & PTE_P))
		{
			paddr_t pa = PTE_ADDR(*pte);
			uint aflags = PTE_FLAGS(*pte);

			vaddr_t mem = kalloc();
			if (!mem)
			{
				badflag = 1;
				break;
			}
			else
			{
				memmove(mem, P2V(pa), PGSIZE);
				if (mappages(tpgdir, i, PGSIZE, V2P(mem), aflags) == 0)
				{
					badflag = 1;
					break;
				}
			}
		}
	}

	if (badflag)
	{
		freeuvm(tpgdir);
		tpgdir = NULL;
	}

	return tpgdir;
}

// 以下涉及交换区页面置换操作

// 在交换区申请一个slot
uint alloc_slot(uint pn_pid)
{
	uint i = 0;
	for (; i < SLOTSIZE; i++)
		if (swap_map[i] == 0) // 找到一个空闲slot
			break;
	if (i == SLOTSIZE)
		panic("alloc_swap: no free slot in swap space");

	swap_map[i] = pn_pid;

	return SWAPM2SECNO(i);
}

// 在交换区查找一个slot
uint find_slot(uint pn_pid)
{
	uint i = 0;
	for (; i < SLOTSIZE; i++) // 需要查找全部，空闲部分不一定连续
		if (swap_map[i] == pn_pid)
			return SWAPM2SECNO(i);

	panic("find_slot: slot not found");
}

// 从交换区删除一个slot（实际上只删除索引）, 输入交换区section编号
void free_slot(uint slotn)
{
	uint i = SECNO2SWAPM(slotn);
	if (i >= SLOTSIZE)
		panic("free_slot: free a non-exist slot");
	swap_map[i] = 0;
}

// 添加页到队列中，用于后期页面置换（只用于添加用户页，内核页不允许置换）
void add_page(pte_t *p, vaddr_t va, uint pid)
{
	// 动态申请一个32-byte的slab
	struct page_entry *e = (struct page_entry *)alloc_slab();
	e->ptr_pte = p;
	e->pn_pid = (uint)va | pid;
	e->ref = *p & PTE_A;

	// 队列尾部添加
	Q_INSERT_TAIL(&pgqueue, e, link);
}

// 从队列中释放页
void free_page(uint pn_pid, uint flag_slot)
{
	struct page_entry *e;
	Q_FOREACH (e, &queue.qhead, link)
	{
		if (e->pn_pid == pn_pid)
		{
			if (flag_slot)
			{
				uint slotn = find_slot(e->pn_pid);
				free_slot(slotn);
			}

			Q_REMOVE(&queue.qhead, e, link);
			return;
		}
	}

	panic("free_page: free a non-exist page");
}