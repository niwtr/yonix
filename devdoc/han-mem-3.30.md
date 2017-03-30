## 系统初始内存分配

### 第一个页表-entrypgdir

xv6启动过程应该从`entry.S`开始，后续加载`main.c`开始正式执行我们的代码。在`main.c`中刚开始，由于还没有执行我们的分页程序，内核使用`entrypgdir`简
化页表访问物理内存，且物理内存只有低4MB可以使用，也就是[0, 4MB)。

具体映射关系是 逻辑地址 -> 物理地址：`[2G，2G+4M)-> [0, 4MB)`，也就是说内核逻辑基地址是从2G开始的。我们的用户程序逻辑地址就只能为0-2G了。
>这块没有直接加载内核逻辑地址为物理地址，官方解释是之前考虑到使用小型机，可能机器没有这样这样大的物理内存。因此内核优先从物理内存低地址开始分配。

### 正统页表
进入`main.c`后首要任务就是加载我们的页表机制，但是这里出现了一种‘死锁’，我们需要构建页表，但构建页表的过程也需要使用页表转换机制。因此可以看到在`main.c`
中初始化页表分成了两个部分，`kinit1`和`kinit2`。
- `kinit1`利用之前的`entrypgdir`机制在内核后面申请了4MB内存，用来存储可分配物理内存页的空闲链表。
- `kinit2`将最开始物理内存4MB以后的所有地址重新分配，建立一个新的内存地址空间。也就是说物理内存从4MB开始到顶端都作为空闲页供系统调用。

>`kinit2`之所以从4MB以后开始初始化，官方解释是，重新分配后的物理内存中，0-4MB用于I/O空间，不知道后续具体在哪里应用到这块地址？另外官方默认最大物理
内存为240MB，解释说是X86机器自动检测物理内存大小比较复杂。



|虚拟地址|物理地址|
|---|---|
|0..KERNBASE|ser memory (text+data+stack+heap), mapped to phys memory allocated by the kernel#用户程序内存|
|KERNBASE..KERNBASE+EXTMEM| mapped to 0..EXTMEM (for I/O space) #I/O内存空间4MB|
|KERNBASE+EXTMEM..data| mapped to EXTMEM..V2P(data) for the kernel's instructions and r/o data  #内核内存空间|
|data..KERNBASE+PHYSTOP| mapped to V2P(data)..PHYSTOP,rw data + free physical memory #内核内存+空闲内存|
|0xfe000000..0|mapped direct (devices such as ioapic) #其他设备地址|
