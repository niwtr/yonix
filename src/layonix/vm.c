// 虚拟内存管理

#include "param_yonix.h"
#include "queue_netbsd.h"
#include "yotypes.h"
#include "defs.h"
#include "mmu.h"
#include "memlayout.h"
#include "x86.h"
#include "proc.h"

// swap_map偏移映射到交换区section编号
#define SWAPM2SECNO(i) ((uint)(i) << 3)
#define SECNO2SWAPM(i) ((uint)(i) >> 3)

typedef Q_HEAD(page_entry_list, page_entry) q_head;
typedef Q_ENTRY(page_entry) q_entry;

struct cpu cpus[1];

// 页队列项
struct page_entry
{
	pte_t *ptr_pte;
	// we can't actually use pointer to pte
	// because in page_out, it's not the same page table
	uint pn_pid; // 页号按位与pid，作为交换区页标识符
	uint ref;	// reference bit, 引用位, 用于二次机会算法
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
	paddr_t phys_end;
	uint perm;
} kmap[] = {
	{(vaddr_t)KERNBASE, 0, EXTMEM, PTE_W},			  // I/O space
	{(vaddr_t)KERNLINK, V2P(KERNLINK), V2P(data), 0}, // kern text+rodata
	{(vaddr_t)data, V2P(data), PHYSTOP, PTE_W},		  // kern data+memory
	{(vaddr_t)DEVSPACE, DEVSPACE, MEMTOP, PTE_W},	 // more devices
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
	c = &cpus[0];
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
	if ((*pde & PTE_P) == 0)
	{
		if (alloc == 0 || (pgtab = (pte_t *)kalloc()) == 0) // 申请一页存储页表
			return NULL;

		// 通过判断PTE_P是否为0获知是否重复映射
		memset(pgtab, 0, PGSIZE);

		// 只是二级页表的访问权限，更加精细的页面访问权限在下一级控制
		*pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
	}
	else
	{
		pgtab = (pte_t *)P2V(PTE_ADDR(*pde));
	}

	return &pgtab[PTX(va)];
}

// 对于一段相同权限的内存，创建二级页表映射
int mappages(pde_t *pgdir, vaddr_t va, uint size, paddr_t pa, uint perm)
{
	vaddr_t bottom = (vaddr_t)PGROUNDDOWN((uint)va);
	vaddr_t top = (vaddr_t)PGROUNDDOWN((uint)va + size - 1); // 表示最后一页基地址

	pte_t *pte = NULL;

	while (1)
	{
		if ((pte = walkpgdir(pgdir, bottom, 1)) == 0)
			return -1;

		// 重复的映射
		if (*pte & PTE_P)
			panic("Duplicate mapping");

		*pte = pa | perm | PTE_P;

		if (bottom == top) // 注意：到达最大内存地址后32位会溢出，造成无限循环
			break;

		bottom += PGSIZE;
		pa += PGSIZE;
	}

	return 0;
}

// 获取一个新的二级页表，并包含内核所有映射(此处为内核页表，不需要加入到页队列中)
pde_t *setupkvm()
{
	// 物理内存过大
	if (PHYSTOP > DEVSPACE)
		panic("PHYSICAL MEMORY TOO BIG");

	pde_t *pgdir;
	if ((pgdir = (pde_t *)kalloc()) == 0)
		return 0;

	memset(pgdir, 0, PGSIZE);

	int i;
	for (i = 0; i < NELEM(kmap); i++)
		if (mappages(pgdir, kmap[i].virt_start, kmap[i].phys_end - kmap[i].phys_start,
					 kmap[i].phys_start, kmap[i].perm) < 0)
			return NULL;

	return pgdir;
}

// 为内核申请新的页表, 启动时使用
void kvmalloc()
{
	kpgdir = setupkvm();
	if (!kpgdir)
		panic("kvmalloc: setupkvm error");
	switchkvm();
}

// 切换到内核页表, 用于进程结束后启用调度器
void switchkvm()
{
	if (!Q_EMPTY(&pgqueue))
		read_ref(); // 每次程序运行完后，更新页队列中引用位
	lcr3(V2P(kpgdir));
}

// 用户进程页表切换，用户进程调度开始
void switchuvm(struct proc *p)
{
	write_ref(); // 修改页面引用位为0

	cpu->gdt[SEG_TSS] = SEG16(STS_T32A, &cpu->ts, sizeof(cpu->ts) - 1, 0);
	cpu->gdt[SEG_TSS].s = 0;
	cpu->ts.ss0 = SEG_KDATA << 3;
	cpu->ts.esp0 = (uint)p->p_kstack + KSTACKSIZE;
	ltr(SEG_TSS << 3);
	if (p->p_page == 0)
		panic("switchuvm: no pgdir");
	lcr3(V2P(p->p_page)); // switch to new address space
}

// 将当前程序的内存页面引用位设置为0
// （注意cpu会自动设置相应页面引用位为1，但需要操作系统手动置为0）
void write_ref()
{
	struct page_entry *e;
	Q_FOREACH (e, &pgqueue, link)
	{
		// 注意限制pid最大为16^3-1
		if (PTE_FLAGS(e->pn_pid) != proc->p_pid)
			continue;

		uint pa = PTE_ADDR(*(e->ptr_pte));
		pte_t *pte = walkpgdir(proc->p_page, (char *)P2V(pa), 0); // 得到页在内核段页表项

		if (pte == 0)
			panic("write_ref: PTE should exist");

		*pte = (*pte) & (0xffffffff ^ PTE_A); // 设置accessable位
	}
}

// 读取当前程序的内存页面引用位
void read_ref()
{
	struct page_entry *e;
	Q_FOREACH (e, &pgqueue, link)
	{
		if (PTE_FLAGS(e->pn_pid) != proc->p_pid)
			continue;

		uint pa = PTE_ADDR(*(e->ptr_pte));
		pte_t *pte = walkpgdir(proc->p_page, (char *)P2V(pa), 0); // 得到页在内核段页表项
		if (pte == 0)
			panic("read_ref: PTE should exist");

		e->ref = (*pte) & PTE_A;
	}
}

// 初始化用户进程虚拟地址空间
void inituvm(pde_t *pgdir, char *init, uint sz)
{
	char *mem;

	// 初始化只分配一页
	if (sz >= PGSIZE)
		panic("inituvm: more than a page");
	mem = kalloc();
	memset(mem, 0, PGSIZE);
	mappages(pgdir, 0, PGSIZE, V2P(mem), PTE_W | PTE_U);
	memmove(mem, init, sz);
}

// 用户进程增长物理内存，从startva到endva（startva<endva）
int allocuvm(pde_t *pgdir, uint startva, uint endva)
{
	if (endva >= KERNBASE)
		return 0;
	if (endva < startva)
		return startva;

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

		if (mappages(pgdir, (vaddr_t)i, PGSIZE, V2P(mem), PTE_P | PTE_W | PTE_U) < 0)
		{
			kfree(mem);
			badflag = 1;
			break;
		}

		pte_t *p = walkpgdir(pgdir, (vaddr_t)i, 0);
		if (i > PGROUNDUP(startva)) // 跳过初始init进程空间
			if (*p & PTE_U)			//确定是用户程序
				add_page(p, (vaddr_t)i, proc->p_pid);
	}

	if (badflag)
	{
		deallocuvm(pgdir, i, startva, proc->p_pid); // 恢复原始内存大小
		return 0;
	}

	return endva;
}

// 为用户进程释放部分物理内存，从startva到endva（startva>endva）
// 注意程序可能由另外的程序释放，故而应输入程序pid
int deallocuvm(pde_t *pgdir, uint startva, uint endva, uint pid)
{
	if (endva < 0)
		return 0;
	if (startva <= endva)
		return startva;

	pte_t *pte = NULL;
	uint i;
	for (i = PGROUNDUP(endva); i < startva; i += PGSIZE)
	{
		pte = walkpgdir(pgdir, (vaddr_t)i, 0);
		if (!pte)
			i += (NPTENTRIES - 1) * PGSIZE;
		else if ((*pte & PTE_P) != 0)
		{
			paddr_t pa = PTE_ADDR(*pte);

			free_page(i | pid, 1); // 从页队列中删除
			kfree(P2V(pa));

			*pte = 0; // 对应页表项清零
		}
	}

	return endva;
}

// 释放用户进程全部地址空间
void freeuvm(pde_t *pgdir, uint pid)
{
	if (pgdir == NULL)
		panic("freeuvm: already freed");

	deallocuvm(pgdir, KERNBASE, 0, pid);

	// 释放二级页表所占内存空间
	int i;
	for (i = 0; i < NPDENTRIES; i++)
	{
		pde_t *pde = &pgdir[i];
		if (*pde & PTE_P)
		{
			vaddr_t va = P2V(PTE_ADDR(*pde));
			kfree(va);
		}
	}

	kfree((vaddr_t)pgdir);
}

// 加载文件到内存，并设置用户页表，exec()基础
int loaduvm(pde_t *pgdir, vaddr_t addr, struct inode *ip, uint offset, uint sz)
{
	uint i, n;
	paddr_t pa;
	pte_t *pte;

	if ((uint)(addr) % PGSIZE != 0)
		panic("loaduvm: start addr must be page alligned");
	for (i = 0; i < sz; i += PGSIZE)
	{
		if ((pte = walkpgdir(pgdir, addr + i, 0)) == 0)
			panic("loaduvm: non-exist address");
		pa = PTE_ADDR(*pte);

		// 防止末尾文件不足一页
		if (sz - i < PGSIZE)
			n = sz - i;
		else
			n = PGSIZE;

		if (readi(ip, P2V(pa), offset + i, n) != n)
			return -1;
	}

	return 0;
}

// 复制用户地址空间内容到新物理内存，fork()基础
pde_t *copyuvm(pde_t *spgdir, uint sz, uint pid)
{
	pde_t *tpgdir = NULL;
	if ((tpgdir = setupkvm()) == NULL)
		return 0;

	// 复制用户地址空间所有内存
	int badflag = 0;
	paddr_t i;
	pte_t *pte = NULL;
	for (i = 0; i < sz; i += PGSIZE) // 用户地址空间连续分配虚拟内存时，可以优化
	{
		pte = walkpgdir(spgdir, (vaddr_t)i, 0);
		if (!(*pte & PTE_P))
		{
			page_in(i);
		}
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
			if (mappages(tpgdir, (vaddr_t)i, PGSIZE, V2P(mem), aflags) < 0)
			{
				badflag = 1;
				break;
			}

			pte_t *p = walkpgdir(tpgdir, (vaddr_t)i, 0);
			if (i > 0) // 跳过初始init进程空间
				if (*p & PTE_U)
					add_page(p, (vaddr_t)i, pid);
		}
	}

	if (badflag)
	{
		freeuvm(tpgdir, pid);
		tpgdir = NULL;
	}

	return tpgdir;
}

// Clear PTE_U on a page. Used to create an inaccessible
// page beneath the user stack.
void clearpteu(pde_t *pgdir, char *uva)
{
	pte_t *pte;

	pte = walkpgdir(pgdir, uva, 0);
	if (pte == 0)
		panic("clearpteu: error");
	*pte &= ~PTE_U;
}

// 将用户空间的虚拟地址转换到内核空间对应虚拟地址
char *
uva2ka(pde_t *pgdir, char *uva)
{
	pte_t *pte;

	pte = walkpgdir(pgdir, uva, 0);
	if ((*pte & PTE_P) == 0)
		return 0;
	if ((*pte & PTE_U) == 0)
		return 0;
	return P2V(PTE_ADDR(*pte));
}

// Copy len bytes from p to user address va in page table pgdir.
// Most useful when pgdir is not the current page table.
// uva2ka ensures this only works for PTE_U pages.
int copyout(pde_t *pgdir, uint va, void *p, uint len)
{
	char *buf, *pa0;
	uint n, va0;

	buf = (char *)p;
	while (len > 0)
	{
		va0 = (uint)PGROUNDDOWN(va);
		pa0 = uva2ka(pgdir, (char *)va0);
		if (pa0 == 0)
		{

			return -1;
		}
		n = PGSIZE - (va - va0);
		if (n > len)
			n = len;
		memmove(pa0 + (va - va0), buf, n);
		len -= n;
		buf += n;
		va = va0 + PGSIZE;
	}
	return 0;
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

	cprintf("find_slot %d\n", pn_pid);
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

// 和上面函数作用相同，但允许输入不存在的页标识符
void free_page_slot(uint pn_pid)
{
	uint i = 0;
	for (; i < SLOTSIZE; i++)
		if (swap_map[i] == pn_pid)
		{
			swap_map[i] = 0;
			break;
		}
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
	Q_FOREACH (e, &pgqueue, link)
	{
		if (e->pn_pid == pn_pid)
		{
			if (flag_slot)
			{
				free_page_slot(e->pn_pid);
			}

			Q_REMOVE(&pgqueue, e, link);
			return;
		}
	}
}

// 从队列选择需要换出的页
struct page_entry *sel_page()
{
	struct page_entry *e;
	if (PR_ALGO == PR_SCND)
	{
		// 二次机会算法
		Q_FOREACH (e, &pgqueue, link)
		{
			if (e->ref == PTE_A)
				e->ref = 0;
			else
				return e;
		}
	}

	// FIFO算法, 或者二次机会的下一轮
	e = Q_FIRST(&pgqueue);

	return e;
}

// 页错误处理
int pgflt_handle(uint va)
{
	va = PGROUNDDOWN(va);
	cprintf("page fault %x\n", va);
	if (va < proc->p_size)
	{
		page_in(va);
		return 0;
	}

	return -1;
}

// 初始化页队列
void swapinit()
{
	Q_INIT(&pgqueue);
}

// 调页进内存
void page_in(uint va)
{
	if(!USE_SWAP)
		panic("page_in: swap is denied");	// 没有开启swap功能

	vaddr_t mem = kalloc();
	if (mem == 0)
		panic("page_in: no enough memory");

	pte_t *p = walkpgdir(proc->p_page, (vaddr_t)va, 0);
	if (p == 0)
		panic("page_in: pte should exist");

	// 从交换区读取对应slot
	uint slotn = find_slot((proc->p_pid | va));
	read_swap(slotn, mem);
	free_slot(slotn);

	// 写对应页表项
	*p = V2P(mem) | PTE_FLAGS(*p) | PTE_P;
	add_page(p, (vaddr_t)va, proc->p_pid);

	cprintf("page in %x\n", va);
}

// 置换页到交换区
void page_out()
{
	if (Q_EMPTY(&pgqueue))
		panic("page_out: no page to page out");

	// 选择适合的页，并更改对应页表项
	struct page_entry *e = sel_page();
	pte_t *p = e->ptr_pte;
	uint pa = PTE_ADDR(*p);
	*p ^= PTE_P;

	// 写入交换区,并从内存中删除
	uint slotn = alloc_slot(e->pn_pid);
	write_swap(slotn, P2V(pa));

	kfree(P2V(pa));
	free_page(e->pn_pid, 0);

	cprintf("page out %x of proc %x\n", PTE_ADDR(e->pn_pid), PTE_FLAGS(e->pn_pid));
}