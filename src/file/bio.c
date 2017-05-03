/*块缓冲层*/ 
#include "types.h"
#include "param.h"
#include "defs.h"
#include "sleeplock.h"
#include "buf.h"
#include "fs.h"

struct {
	struct buf buf[NBUF];//链表缓冲块
	struct buf head;
} bcache;

//创建一个双向链表缓冲区
void binit()  
{
	struct buf *b;
   
    //创建只含头结点的双向链表 
	bcache.head.pre = &bcache.head;  
	bcache.head.next = &bcache.head;
	
    for(b = bcache.cache; b < bcache.cache+NBUF; b++)
    {
    	b->dev=-1;
    	b->next=bcache.head.next;
    	b->pre=&bcache.head;
    	 initsleeplock(&b->lock, "buffer");//建立锁并命名为buffer 
		bcache.head.next->pre=b;
		bcache.head.next=b;
    }
}

//根据块编号和块编号获取块缓冲块结构
static struct buf* bget(uint dev, uint blockno)
{
	struct buf *b;

	//缓冲块空闲则设置为忙碌并返回，忙碌则睡眠等待
	for (b = bcache.head.next; b != &bcache.head; b = b->next) {
		if (b->dev == dev && b->blockno == blockno) {
			b->refcnt++;
			acquiresleep(&b->lock);
			return b;
		}
	}

	//若此扇区无缓冲块，为扇区分配缓冲块
	for (b = bcache.head.prev; b != &bcache.head; b = b->prev) {
		if (b->refcnt == 0 && (b->flags & B_DIRTY) == 0) {
			b->dev = dev;
			b->blockno = blockno;
			b->flags = 0;
			b->refcnt = 1;
			acquiresleep(&b->lock);
			return b;
		}
	}
	panic("bget: no buffers");//缓冲区满 
}

//从磁盘取走一块读入缓冲区
struct buf* bread(uint dev, uint blockno)
{
	struct buf *b;

	b = bget(dev, blockno);
	if (!(b->flags & B_VALID)) {
		iderw(b);//若buf无效，把磁盘内容读到缓冲块
	}
	return b;
}

//将缓冲区内容写到磁盘
//在释放缓冲区之前将其写入磁盘
void bwrite(struct buf *b)
{
	if (!holdingsleep(&b->lock))
		panic("bwrite");
	b->flags |= B_DIRTY;
	iderw(b);
}

//释放缓冲块以及块置换LRU
void brelse(struct buf *b)
{
	if (!holdingsleep(&b->lock))
		panic("brelse");

	releasesleep(&b->lock);

	b->refcnt--;
	if (b->refcnt == 0) {
		//移动缓冲区 按最近被使用情况排序 
		//从队列取出p节点
		b->next->prev = b->prev;
		b->prev->next = b->next;
		//将p节点插入链表头 
		b->next = bcache.head.next;
		b->prev = &bcache.head;
		bcache.head.next->prev = b;
		bcache.head.next = b;
	}
}



