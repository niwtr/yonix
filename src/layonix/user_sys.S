#include "syscall.h"
#include "traps.h"

#define SYSCALL(name) \
  .globl name; \
  name: \
    movl $SYS_ ## name, %eax; \
    int $T_SYSCALL; \
    ret

SYSCALL(fork)
SYSCALL(exit)
SYSCALL(wait)
SYSCALL(pipe)
SYSCALL(read)
SYSCALL(write)
SYSCALL(close)
SYSCALL(kill)
SYSCALL(exec)
SYSCALL(open)
SYSCALL(mknod)
SYSCALL(unlink)
SYSCALL(fstat)
SYSCALL(link)
SYSCALL(mkdir)
SYSCALL(chdir)
SYSCALL(dup)
SYSCALL(getpid)
SYSCALL(sbrk)
SYSCALL(sleep)
SYSCALL(uptime)
SYSCALL(incnice)
SYSCALL(decnice)
SYSCALL(sched)
SYSCALL(lwp_create)
SYSCALL(lwp_join)
SYSCALL(sched_name)
SYSCALL(dsstore)
SYSCALL(dsrestart)
SYSCALL(atom_add)
SYSCALL(atom_sub)
SYSCALL(atom_mul)
SYSCALL(atom_div)
SYSCALL(atom_mod)
SYSCALL(atom_set)
SYSCALL(atom_swp)
SYSCALL(sem_init)
SYSCALL(sem_p)
SYSCALL(sem_v)
SYSCALL(mut_init)
SYSCALL(mut_p)
SYSCALL(mut_v)

