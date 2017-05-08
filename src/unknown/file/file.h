/*文件结构*/
struct file {
	enum { FD_NONE, FD_PIPE, FD_INODE } type;//文件类型pipe/inode
	int ref; //应用次数 
	char readable;//可读
	char writable;//可写
	struct pipe *pipe;
	struct inode *ip;
	uint off;
};

/*内存上i结点*/
struct inode {
	uint dev;           //设备号 
	uint inum;          //i节点号
	int ref;            //指向该i节点的指针
	struct sleeplock lock;
	int flags;          //状态（占用，空闲） 
	short type;         //文件类型
	short major;		// 主设备号 
	short minor;		// 次设备号 
	short nlink;		//指向该i节点的数目
	uint size;			//文件大小 
	uint addrs[NDIRECT + 1];//数据块地址，直接块/间接块
};
#define I_VALID 0x2

/*设备文件类型*/
struct devsw {
	int(*read)(struct inode*, char*, int);
	int(*write)(struct inode*, char*, int);
};

extern struct devsw devsw[];
#define CONSOLE 1

