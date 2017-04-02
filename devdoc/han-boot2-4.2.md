## 再谈启动过程分页细则

### `entrypgdir`分析

``` asm
__attribute__((__aligned__(PGSIZE)))
pde_t entrypgdir[NPDENTRIES] = {
  // Map VA's [0, 4MB) to PA's [0, 4MB)
  [0] = (0) | PTE_P | PTE_W | PTE_PS,
  // Map VA's [KERNBASE, KERNBASE+4MB) to PA's [0, 4MB)
  [KERNBASE>>PDXSHIFT] = (0) | PTE_P | PTE_W | PTE_PS,
};
```
这里的初始化数组方法并不常用，直接给具体某个数组元素赋初值。由于之前在`entry.s`中开启了4MB分页，MMU自动调整为一级分页，高10位为页面号，低22位为页内偏移，
具体来说，`KERNBASE>>PDXSHIFT`相当与提取了`KERNBASE`的页面号，其页表项对应物理地址为0，后面三个为flag标志。这样一来，也就实现了 [KERNBASE, KERNBASE+4MB)
到[0, 4M)的映射。
>对于虚拟地址[0, 4MB)到物理地址[0, 4MB)的映射不知道有什么作用？

### 4kb对齐
上面代码中可以看到使用了`__aligned__`属性，表示这块内存采用`PGSIZE`也就是4KB对齐。注意这里虽然使用的是4MB的页面大小，但是整个内核代码依旧采用常规
页面4KB大小来对齐。

其实原因应该跟内核结构有关，可以查看`kernel.ld`链接文件，这里的代码都是对于内核结构组织的一些定义。
``` asm
/* Adjust the address for the data segment to the next page */
	. = ALIGN(0x1000);
```

这一行表示内核代码默认4KB对齐。

>采用4kb而不是4MB对齐的原因应该是4kb页面大小刚刚合适，而4MB过于庞大，毕竟整个内核加载到内存也不会超过4MB。
