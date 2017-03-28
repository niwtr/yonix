  #include "syscall.h"
  #include "traps.h"
  #define _SYSCALL(name)\
  .globl name                   ;\
name: \
  movl $SYS_ ## name, %eax      ;\
  int $T_SYSCALL                ;\
  ret
  
  #define __VAR_LST__\
  X(fork)\
  X(exit)\
  X(wait)\
  X(pipe)\
  X(read)\
  X(write)\
  X(close)\
  X(kill)\
  X(exec)\
  X(open)\
  X(mknod)\
  X(unlink)\
  X(fstat)\
  X(link)\
  X(mkdir)\
  X(chdir)\
  X(dup)\
  X(getpid)\
  X(sbrk)\
  X(sleep)\
  X(uptime)

  #define X(name) _SYSCALL(name)
  __VAR_LST__
  #undef X
  #undef __VAR_LST__
  #undef _SYSCALL
