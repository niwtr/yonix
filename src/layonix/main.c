#include "param_yonix.h"
#include "yotypes.h"
#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"


extern pde_t *kpgdir;
extern char end[]; // first address after kernel loaded from ELF file

// Bootstrap processor starts running C code here.
// Allocate a real stack and switch to it, first
// doing some setup required for memory allocator to work.
int
main(void)
{


  kinit1(end, P2V(4*1024*1024)); // phys page allocator
  kvmalloc();      // kernel page table

  seginit();       // segment descriptors

  picinit();       // another interrupt controller

  consoleinit();   // console hardware
  uartinit();      // serial port

  prtwelcome();


  trapvecinit();        // trap vectors
  binit();         // buffer cache
  fileinit();      // file table
  ideinit();       // disk

  timerinit();   // uniprocessor timer
  kinit2(P2V(4*1024*1024), P2V(PHYSTOP));
  swapinit();     // page queue for swap initial

  esinit();       // init proc slot queue
  rdinit();
  select_scheme(SCHEME_RR);    // init scheduler method (for time slice)
  userinit();      // first user process

  idtinit();       // load idt register

  scheduler();     // start running processes
}



pde_t entrypgdir[];  // For entry.S

// The boot page table used in entry.S and entryother.S.
// Page directories (and page tables) must start on page boundaries,
// hence the __aligned__ attribute.
// PTE_PS in a page directory entry enables 4Mbyte pages.

__attribute__((__aligned__(PGSIZE)))
pde_t entrypgdir[NPDENTRIES] = {
  // Map VA's [0, 4MB) to PA's [0, 4MB)
  [0] = (0) | PTE_P | PTE_W | PTE_PS,
  // Map VA's [KERNBASE, KERNBASE+4MB) to PA's [0, 4MB)
  [KERNBASE>>PDXSHIFT] = (0) | PTE_P | PTE_W | PTE_PS,
};


