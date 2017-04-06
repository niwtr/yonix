/*块缓冲区用双向链表形式存储缓冲区 */
struct buf{
	int flags;//状态 
	uint dev;//设备号 
	uint sector;//块编号 
	struct sleeplock lock;
	struct buf *pre; //置换页链表 
	struct buf *next; 
	struct buf *qnext; //磁盘链表 
	uchar data[BSIZE]; 
};


#define b_busy  0x1 //工作状态 
#define b_valid 0X2 //磁盘数据有效 
#define b_dirty 0x4 //数据需要写会磁盘 
#define NBUF 14//双向链表数 
