typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef uint pde_t; // BORROWED from xv6
typedef uint pte_t;
typedef uint paget;

// 硬件默认代码中所有地址均为虚拟地址,
// 故而将物理地址存为无符号整型
typedef char* vaddr_t;
typedef uint paddr_t;

typedef int rtval;