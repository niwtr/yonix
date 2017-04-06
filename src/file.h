/*内核i结点*/
struct inode{
	uint dev; //设备号 
	uint inum; //i节点号
	int ref; //指向该i节点的指针
	int flag ; //状态（占用，空闲） 
	
	short type;//文件类型
	short major;// Major device number (T_DEV only)
	short minor;// Minor device number (T_DEV only)
	short nlink;//指向该i节点的目录项 
	uint fsize;//文件大小 
	uint addrs[NINDTRECT+1];//数据块地址 
};  

/*文件结构*/ 
struct file{
	enum{fd,dfpipe,fdinode}type;
	int ref;//应用次数 
	char rable;
	char wable;
	struct pipe *pipe;
	struct inode *ip;
}; 

/*表映射主设备函数？？*/
struct devsw {
  int (*read)(struct inode*, char*, int);
  int (*write)(struct inode*, char*, int);
};

extern struct devsw devsw[];
