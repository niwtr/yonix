/*文件系统磁盘结构
第0块：bootl 文件系统不使用
第1块：超级块，存储一些信息
第2-n块：i节点块，空闲块，数据块 */

#define BSIZE 512  // 块大小
#define NDIRECT 12 //直接存储数据块数目 
#define NINDIRECT (BSIZE / sizeof(uint))//间接存储数据块数目 一般为sizeof(uint)？？？ 
#define MAXFILE (NDIRECT + NINDIRECT)//数据块最大存储量 
  
/*超级块数据结构*/ 
struct supblock{
	uint size;//文件系统总块数 
	unit sdata;//数据块总块数 
	unit sinode;//i节点总块数
	unit slog;//日志块数 
}; 

/*磁盘上的结点块结构*/
struct dinode{
	short type;//文件类型
	short major;// Major device number (T_DEV only)
	short minor;// Minor device number (T_DEV only)
	short nlink;//指向该i节点的目录项 
	uint fsize;//文件大小 
	uint addrs[NINDTRECT+1];//数据块地址 
}; 

#define IPB (BSIZE / sizeof(struct dinode))  //每块多少个i节点 
#define IBLOCK(i) ((i) / IPB + 2)  //第i个i节点所在的块 
#define BPB (BSIZE*8)    //每个位图块可用管理的大小  
#define BBLOCK(b, ninodes) (b/BPB + (ninodes)/IPB + 3)  //第i个块所在的位图块 
#define DIRSIZE 14  //目录结点对应文件数
  
/*目录文件结构*/ 
struct dirent {  
  ushort inum;  // i节点号 
  char fname[DIRSIZE]; //文件名字  
};  
