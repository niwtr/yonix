
#include "yotypes.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h" //taskstate

#include "proc.h" //proc

#include "x86.h" //trapframe
#include "syscall.h"


/*
 *     虚拟内存结构：
 *     ┌───────────┐
 *     │0xFFFFFFFF │
 *     └───────────┴───▶┌────────────────────────┐
 *                      │                        │
 *                      │                        │
 *                      │                        │
 *                      │                        │
 *                      │                        │
 *                      ├────────────────────────┤
 *                      │      Free Memory       │
 *                      │                        │
 *                      ├────────────────────────┤
 *                      │                        │
 *                      │     Text and Data      │
 *     ┌───────────┐    │                        │
 *     │0x80100000 │    │                        │
 *     └───────────┴───▶├────────────────────────┤
 *                      │                        │
 *                      │                        │
 *                      │          BIOS          │
 *     ┌───────────┐    │                        │
 *     │0x80000000 │    │                        │
 *     └───────────┴───▶├────────────────────────┤
 *                      │                        │
 *                      │                        │
 *                      │          Heap          │
 *                      │                        │
 *                      │                        │
 *                      ├────────────────────────┤
 *                      │       User Stack       │
 *                      ├────────────────────────┤
 *                      │                        │
 *                      │                        │
 *                      │   User Text and Data   │
 *      ┌───────────┐   │                        │
 *      │ 0x0000000 │   │                        │
 *      └───────────┴──▶└────────────────────────┘
 *
 *    BIOS以上的位置为内核空间。
 */
/*
rtval _sysc_getint(uint addr, int * intp)
{
  // 检查访问的地址是否非法。——用户只能访问其用户空间（小于p->p_size）的地址空间。 
  // 注意一个指针它是有宽度的。
  if(addr >= proc->p_size || addr + 4 > proc->p_size)
    return -1;


  // 用户和内核共享同一个页表，所以这里可以简单地把intp转化为一个指针。
  *intp=*(int*)addr;
  return 0;
}
rtval _sysc_getstr (uint addr, char ** strp)
{
  char * s, * bd; //boundary
  if(addr >= proc->p_size)
    return -1;

  *strp=(char*) addr;
  bd=(char *)proc->p_size;
  for(s=*strp;s<bd;s++)
    if(*s == 0)
      return s- *strp;
  return -1;
}


rtval sysc_argint(int n, int * intp)
{
  return _sysc_getint(proc->tf->esp +4 +4*n, intp);
}

// WARNING 这里我把char **改成void ** 了。
rtval sysc_argptr(int n, char ** ptrp, int size)
{
  int i;
  //先把参数当整数取一遍，看看是否合法。
  if(sysc_argint(n, &i) <0)
    return -1;
  //再检查一遍，因为argint不能检查size不等于4的情况，同时也要检查size是否合法。
  if(size < 0 || (uint) i >= proc->p_size || (uint)i+size > proc->p_size)
    return -1;
  *ptrp = (char *) i;
  return 0;
}

rtval sysc_argstr(int n, char ** strp){
  int addr;
  if(_sysc_getint(n, &addr)<0)
     return -1;
  // now the addr is morphed by _sysc_argstr.
  return _sysc_getstr(addr, strp);
}

*/








#define fetchint _sysc_getint
#define fetchstr _sysc_getstr
#define argstr sysc_argstr
#define argptr sysc_argptr
#define argint sysc_argint
// User code makes a system call with INT T_SYSCALL.
// System call number in %eax.
// Arguments on the stack, from the user call to the C
// library system call function. The saved user %esp points
// to a saved program counter, and then the first argument.

// Fetch the int at addr from the current process.
int
fetchint(uint addr, int *ip)
{
  if(addr >= proc->p_size || addr+4 > proc->p_size)
    return -1;
  *ip = *(int*)(addr);
  return 0;
}

// Fetch the nul-terminated string at addr from the current process.
// Doesn't actually copy the string - just sets *pp to point at it.
// Returns length of string, not including nul.
int
fetchstr(uint addr, char **pp)
{
  char *s, *ep;

  if(addr >= proc->p_size)
    return -1;
  *pp = (char*)addr;
  ep = (char*)proc->p_size;
  for(s = *pp; s < ep; s++)
    if(*s == 0)
      return s - *pp;
  return -1;
}

// Fetch the nth 32-bit system call argument.
int
argint(int n, int *ip)
{
  return fetchint(proc->p_tf->esp + 4 + 4*n, ip);
}

// Fetch the nth word-sized system call argument as a pointer
// to a block of memory of size bytes.  Check that the pointer
// lies within the process address space.
int
argptr(int n, char **pp, int size)
{
  int i;

  if(argint(n, &i) < 0)
    return -1;
  if(size < 0 || (uint)i >= proc->p_size || (uint)i+size > proc->p_size)
    return -1;
  *pp = (char*)i;
  return 0;
}

// Fetch the nth word-sized system call argument as a string pointer.
// Check that the pointer is valid and the string is nul-terminated.
// (There is no shared writable memory, so the string can't change
// between this check and being used by the kernel.)
int
argstr(int n, char **pp)
{
  int addr;
  if(argint(n, &addr) < 0)
    return -1;
  return fetchstr(addr, pp);
}




#define __lo_syscll__                           \
  X(fork)                                       \
  X(exit)                                       \
  X(wait)                                       \
  X(pipe)                                       \
  X(read)                                       \
  X(kill)                                       \
  X(exec)                                       \
  X(fstat)                                      \
  X(chdir)                                      \
  X(dup)                                        \
  X(getpid)                                     \
  X(sbrk)                                       \
  X(sleep)                                      \
  X(uptime)                                     \
  X(open)                                       \
  X(write)                                      \
  X(mknod)                                      \
  X(unlink)                                     \
  X(link)                                       \
  X(mkdir)                                      \
  X(close)                                      \
  X(incnice)                                    \
  X(decnice)                                    \
  X(sched)                                      \
  X(lwp_create)                                 \
  X(lwp_join)                                   \
  X(sched_name)                                 \
  X(dsstore)                                    \
  X(dsrestart)                                  \
  X(atom_add)                                   \
  X(atom_sub)                                   \
  X(atom_mul)                                   \
  X(atom_div)                                   \
  X(atom_mod)                                   \
  X(atom_set)                                   \
  X(atom_swp)                                   \
  X(sem_init)                                   \
  X(sem_p)                                      \
  X(sem_v)                                      \
  X(mut_init)                                   \
  X(mut_p)                                      \
  X(mut_v)\
  X(toggle_debug)






// externs
#define X(name) extern int sys_ ## name (void);
__lo_syscll__
#undef X

//_syscall int -> f
static int (* _syscalls[])(void) ={
#define X(name) [SYS_ ## name] sys_ ## name,
  __lo_syscll__
#undef X
};


#undef __lo_syscll__

void syscall(void)
{
  int num;

  num = proc->p_tf->eax;
  if(num<sizeof(_syscalls)){
    proc->p_tf->eax = _syscalls[num]();
  } else {
    cprintf("%d %s: unknown sys call %d\n",
            proc->p_pid, proc->p_name, num);
    proc->p_tf->eax = -1;
  }
}

