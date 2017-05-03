// Format of an ELF executable file

#define ELF_MAGIC 0x464C457FU  // "\x7FELF" in little endian

// File header--ELF头
struct elfhdr {
  uint magic;       // must equal ELF_MAGIC 4个字节
  uchar elf[12];
  ushort type;      //目标文件类型
  ushort machine;   //运行改程序的体系结构类型
  uint version;     //目标文件版本
  uint entry;       //Entry Point 程序入口的虚拟地址
  uint phoff;       //程序头部表在文件中的偏移量
  uint shoff;       //节区头部表在文件中的偏移量
  uint flags;       //对IA32而言,此项为0
  ushort ehsize;    //ELF头部表大小
  ushort phentsize; //程序头部表一条表项/条目大小
  ushort phnum;     //程序头部表的数目
  ushort shentsize; //节区头部表一条表项/条目大小
  ushort shnum;     //节区头部表数目
  ushort shstrndx;  //包含节名称的字符串是第几字节
};

// Program section header--程序头表
struct proghdr {
  uint type;        //程序头表描述的段的类型
  uint off;         //段的第一个字节在文件中的偏移
  uint vaddr;       //段的第一个字节在文件中的虚拟地址
  uint paddr;       //为物理内存地址保留——针对物理内存相关的系统
  uint filesz;      //段在文件中的长度
  uint memsz;       //段在内存中的长度
  uint flags;       //与段相关的标志-处理器特定标志
  uint align;       //关于段在内存,文件中是如何对齐
};

// Values for Proghdr type --程序头表类型
#define ELF_PROG_LOAD           1   //指定段为可载入的段

// Flag bits for Proghdr flags--程序头表标志
#define ELF_PROG_FLAG_EXEC      1   //可执行程序标志
#define ELF_PROG_FLAG_WRITE     2   //可写文件标志
#define ELF_PROG_FLAG_READ      4   //可读文件标志

//PAGEBREAK!
// Blank page.
