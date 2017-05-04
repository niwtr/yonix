// Memory layout

#define EXTMEM  0x100000            // 可用内存开始
#define PHYSTOP 0x2000000           // 物理内存顶端，默认物理内存240MB
#define DEVSPACE 0xFE000000         // 预留设备地址空间，最大物理内存不超过DEVSPACE-KERNBASE=2016MB
#define MEMTOP 0xFFFFFFFF			// 32位地址空间顶端

// Key addresses for address space layout (see kmap in vm.c for layout)
#define KERNBASE 0x80000000         // 虚拟内存中内核基地址
#define KERNLINK (KERNBASE+EXTMEM)  // 虚拟内存中内核开始处

#define V2P(a) (((paddr_t) (a)) - KERNBASE)
#define P2V(a) (((vaddr_t) (a)) + KERNBASE)

#define V2P_WO(x) ((x) - KERNBASE)    // same as V2P, but without casts
#define P2V_WO(x) ((x) + KERNBASE)    // same as P2V, but without casts
