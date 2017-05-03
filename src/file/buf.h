/*块缓冲区用双向链表形式存储缓冲区 */
struct buf {
	int flags;
	uint dev;
	uint blockno;//块编号
	struct sleeplock lock;
	uint refcnt;
	struct buf *prev; //置换页链表 
	struct buf *next;
	struct buf *qnext; //磁盘链表
	uchar data[BSIZE]; //数据
};
#define B_VALID 0x2  //缓冲区拥有磁盘数据有效 
#define B_DIRTY 0x4  //缓冲区数据需要写会磁盘 
