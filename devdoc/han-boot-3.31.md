## 分页机制
### 从启动说起

操作系统从`entry.s`的`entry`区段开始引导内核。最开始无法直接访问物理内存，便需要启动临时分页机制，通过设置控制寄存器CR0-CR4完成。
1. 设置分页大小为4MB，通过设置CR4寄存器标志位完成。

``` asm
# Turn on page size extension for 4Mbyte pages
  movl    %cr4, %eax
  orl     $(CR4_PSE), %eax
  movl    %eax, %cr4
```

2. 设置页表目录物理基地址，系统设置CR3寄存器高20位存储页表物理地址。之所以是20位，因为物理内存按页分配，只需要找到页表所在物理页的基地址即可。

``` asm
# Set page directory
  movl    $(V2P_WO(entrypgdir)), %eax
  movl    %eax, %cr3
```

3. 设置CR0寄存器开启分页机制和内存保护。

``` asm
# Turn on paging.
  movl    %cr0, %eax
  orl     $(CR0_PG|CR0_WP), %eax
  movl    %eax, %cr0
```

### 二级页表

页目录10位，页表10位，页内偏移12位。

``` c
// A virtual address 'la' has a three-part structure as follows:
//
// +--------10------+-------10-------+---------12----------+
// | Page Directory |   Page Table   | Offset within Page  |
// |      Index     |      Index     |                     |
// +----------------+----------------+---------------------+
//  \--- PDX(va) --/ \--- PTX(va) --/
```

页表项分为高20位和低12位，分别存储物理页号和flag标志。通过如下宏定义转化。
``` c
// Address in page table or page directory entry
#define PTE_ADDR(pte)   ((uint)(pte) & ~0xFFF)
#define PTE_FLAGS(pte)  ((uint)(pte) &  0xFFF)
```
