


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
rtval _sysc_getint(uint addr, int * intp)
{
  /* 检查访问的地址是否非法。——用户只能访问其用户空间（小于p->p_size）的地址空间。 */
  /* 注意一个指针它是有宽度的。 */
  if(addr >= proc->p_size || addr + 4 > proc->p_size)
    return -1;

  /* FIXME good luck. */
  /* 用户和内核共享同一个页表，所以这里可以简单地把intp转化为一个指针。*/
  *intp=*(int*)addr;
  return 0;
}
rtval _sysc_getstr (uint addr, char ** strp)
{
  char * s, * bd; //boundary
  if(addr >= proc->p_size)
    return -1;

  bd=(char *) proc->p_size;
  *strp=(char* addr);
  for(s=*strp;s<ep;s++)
    if(*s == 0)
      return s- (*strp);
  return -1;
}


rtval sysc_argint(int n, int * intp)
{
  return _sysc_getint(proc->tf->esp +4 +4*n, intp);
}

// WARNING 这里我把char **改成void ** 了。
rtval sysc_argptr(int n, void ** ptrp, int size)
{
  int i;
  /*先把参数当整数取一遍，看看是否合法。*/
  if(argint(n, &i) <0)
    return -1;
  /*再检查一遍，因为argint不能检查size不等于4的情况，同时也要检查size是否合法。*/
  if(size < 0 || (uint) i >= proc->p_size || (uint)i+size > proc_sz)
    return -1;
  **ptrp = (void *) i;
  return 0;
}

rtval sysc_argstr(int n, char ** strp){
  int addr;
  if(_sysc_getint(n, &addr<0))
     return -1;
  // now the addr is morphed by _sysc_argstr.
  return _sysc_getstr(addr, strp);
}
