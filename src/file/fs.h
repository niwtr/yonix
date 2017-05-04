/*文件系统磁盘结构*/
//第0块：bootl 文件系统不使用
//第1块：超级块，存储一些信息
//第2 - n块：i节点块，位图块，数据块，其他块

#define ROOTINO 1  
#define BSIZE 512  //块大小
#define NDIRECT 12 //直接存储数据块数目 
#define NINDIRECT (BSIZE / sizeof(uint))//间接存储数据块数目
#define MAXFILE (NDIRECT + NINDIRECT)//数据块最大存储量 


//超级块数据结构
struct superblock {
	uint size;         // 文件系统总块数 
	uint nblocks;      // 数据块总块数
	uint ninodes;      // i节点总块数
	uint nlog;         // 日志块数
	uint logstart;     // 日志块的第一块
	uint inodestart;   // inode块的第一块
	uint bmapstart;    // 位图块的第一块
};

//i节点块结构
struct dinode {
	short type;           //文件类型，文件/目录/特殊文件
	short major;          // 主设备号(T_DEV only)
	short minor;          // 次设备号(T_DEV only)
	short nlink;		  //指向该i节点的数目
	uint size;            //文件大小
	uint addrs[NDIRECT + 1];//数据块地址 
};

#define IPB(BSIZE / sizeof(struct dinode))//每块多少个i节点
#define IBLOCK(i, sb)((i) / IPB + sb.inodestart)//第i个i节点所在的块 
#define BPB(BSIZE*8) //每个位图块可用管理的大小 
#define BBLOCK(b, sb) (b/BPB + sb.bmapstart)//第i个块所在的位图块
#define DIRSIZ 14//目录结点对应文件数

/*目录条目结构*/
struct dirent {
	ushort inum;  // i节点号 
	char fname[DIRSIZ]; //文件名字  
};
