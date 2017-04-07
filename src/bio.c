/*块缓冲层*/ 
#include "types.h"
#include "param.h"
#include "defs.h"
#include "sleeplock.h"
#include "buf.h" 

struct bufcache{
	struct buf cache[NBUF]; 
	struct buf head; 
}bcache;

/*创建一个双向链表缓冲区*/
void binit()  
{
	struct buf *p; 
   
    //创建只含头结点的双向链表 
	bcache.head->pre = &bcache.head;  
	bcache.head->next = &bcache.head;
	
    for(p = bcache.cache; p < bcache.cache+NBUF; p++)
    {
    	p->dev=-1;
    	p->next=bcache.head->next;
    	p->pre=&bcache.head;
    	 initsleeplock(&p->lock, "buffer");
	bcache.head->next->pre=p;
	bcache.head->next=p;
    }
}

/*根据块编号和设备号获取块缓冲块结构*/
static struct buf * bufget(uint dev,uint sector)
{
	struct buf *p;
	

	/*缓冲块空闲则设置为忙碌并返回，忙碌则睡眠等待*/
	loop:
	for(p = bcache.cache->next; p!= &bcache.head; p = p->next)
	{
		if(p.dev == dev && p.sector == sector)
		{
			if(!(p->flags & b_busy))
			{
				p->flags = (p->flags | b_busy);
				return p;
			}
			acquiresleep(&p->lock);
			goto loop;//避免竞争现象 
		}
	}
	
	/*为扇区分配缓冲块*/ 
	for(p = bcache.head->pre; p!= &bcache.head; p = p->pre) 
	{
		if((p->flags & b_busy)==0 && (p->flags & b_dirty)==0)
		{
			p->dev=dev;
			p->sector=sector;
			p->flags=b_busy;
			acquiresleep(&p->lock);
			return p;
		}
	}
	
	panic("bufget:no buffers");//缓冲区满 
} 

/*从磁盘取走一块读入缓冲区*/ 
struct buf * bufread(uint dev,uint sector)
{
	struct buf *p;
	
	p=bufget(dev,sector);
	if(!(p->flags & b_valid))
	{
		iderw(p); //无效区域将数据读入内核 
	}
	return p;
}

/*将缓冲区内容写到磁盘*/
void bufwrite(struct buf *p)
{
	if(!(p->flags & b_busy))
	{
		panic("bwrite error");
	} 
	p->flags = (p->flags | b_busy);//状态设置为b_busy 
	idrew(p);	
} 

/*释放缓冲块以及块置换LRU*/
void bufrelse(struct buf *p)
{
	if(!(p->flags & b_busy))
	{
		panic("brelse error");
	} 
		
	//移动缓冲区 按最近被使用情况排序 
	//从队列取出p节点
	p->next->pre = p->pre;
	p->pre->next = p->next;
	//将p节点插入链表头 
     	p->next = bcache.head->next;
    	p->pre = &bcache.head;
    	bcache.head->next->pre = p;
    	bcache.head->next = p; 

    	p->flags = (p->flags & ~b_busy);	
    
    	releasesleep(&p->lock);
}
}
 
