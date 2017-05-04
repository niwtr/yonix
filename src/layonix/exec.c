#include "types.h"
#include "yotypes.h"
#include "param_yonix.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "defs.h"
#include "x86.h"
#include "elf.h"

int
exec(char *path, char **argv)
{
  char *s, *last;
  int i, off;
  uint argc, sz, sp, ustack[3+MAXARG+1];
  struct elfhdr elf;      //elf头
  struct inode *ip;       //i节点
  struct proghdr ph;      //prog头
  pde_t *p_page, *oldp_page;
  // A system call should call begin_op()/end_op() to mark
  // its start and end. Usually begin_op() just increments
  // the count of in-progress FS system calls and returns.
  // But if it thinks the log is close to running out, it
  // sleeps until the last outstanding end_op() commits.
  begin_op();   //--start

  //根据路径名找到其对应的i节点号
  if((ip = namei(path)) == 0){
    end_op();   //--end
    return -1;
  }
  ilock(ip);    //锁定i节点
  p_page = 0;
  
  // Check ELF header
  if(readi(ip, (char*)&elf, 0, sizeof(elf)) != sizeof(elf)) //读取elf
    goto bad;
  if(elf.magic != ELF_MAGIC)    //检查文件是否包含ELF的二进制代码
    goto bad;

  if((p_page = setupkvm()) == 0)//获取一个新的二级页表，并包含内核所有映射(此处为内核页表，不需要加入到页队列中)
    goto bad;

  // Load program into memory.
  sz = 0;
  //xv6中程序只有一个程序段的头
  //               段偏移量      段的数目
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
    if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))//将问价内容从i节点读出
      goto bad;
    if(ph.type != ELF_PROG_LOAD)
      continue;
    if(ph.memsz < ph.filesz)
      goto bad;
    if(ph.vaddr + ph.memsz < ph.vaddr)
      goto bad;
    if((sz = allocuvm(p_page, sz, ph.vaddr + ph.memsz)) == 0)//为每个ELF段分配内存
      goto bad;
    if(ph.vaddr % PGSIZE != 0)
      goto bad;
    if(loaduvm(p_page, (char*)ph.vaddr, ip, ph.off, ph.filesz) < 0)//把段的内容载入内存中
      goto bad;
  }
  iunlockput(ip);//i节点使用完毕 被释放
  end_op();     //--end
  ip = 0;

  // Allocate two pages at the next page boundary.
  // Make the first inaccessible.  Use the second as the user stack.
  sz = PGROUNDUP(sz);
  //                     oldsize   newsize
  if((sz = allocuvm(p_page, sz, sz + 2*PGSIZE)) == 0)
    goto bad;
  // Used to create an inaccessible page beneath the user stack.
  clearpteu(p_page, (char*)(sz - 2*PGSIZE));
  sp = sz;

  // Push argument strings, prepare rest of stack in ustack.
  //拷贝所有参数到栈顶
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
    if(copyout(p_page, sp, argv[argc], strlen(argv[argc]) + 1) < 0)//拷贝参数到栈顶
      goto bad;
    ustack[3+argc] = sp;//指针保存在ustack中
  }
  ustack[3+argc] = 0;

  ustack[0] = 0xffffffff;  // fake return PC
  ustack[1] = argc;
  ustack[2] = sp - (argc+1)*4;  // argv pointer

  sp -= (3+argc+1) * 4;
  if(copyout(p_page, sp, ustack, (3+argc+1)*4) < 0)
    goto bad;

  // Save program p_name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(proc->p_name, last, sizeof(proc->p_name));

  // Commit to the user image.
  oldp_page = proc->p_page;
  proc->p_page = p_page;
  proc->p_size = sz;
  proc->p_tf->eip = elf.entry;  // main
  proc->p_tf->esp = sp;
  switchuvm(proc);
  freeuvm(oldp_page, proc->p_pid);
  return 0;

 bad:
  if(p_page)
    freeuvm(p_page, proc->p_pid);
  if(ip){
    iunlockput(ip);
    end_op();
  }
  return -1;
}
