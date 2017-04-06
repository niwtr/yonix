/*�黺���*/ 
#include "types.h"
#include "param.h"
#include "defs.h"
#include "spinlock.h"
#include "buf.h" 

struct bufcache{
	struct spinlock lock; //������ 
	struct buf cache[NBUF]; 
	struct buf head; 
}bcache;

/*����һ��˫����������*/
void binit()  
{
	struct buf *p; 
  
 	initlock(&bcache.lock, "bufcache");//������  
   
    //����ֻ��ͷ����˫������ 
	bcache.head.prev = &bcache.head;  
	bcache.head.next = &bcache.head;
	
    for(p = bcache.cache; p < bcache.cache+NBUF; p++)
    {
    	p->dev=-1;
    	p->next=bcache.head.next;
    	p->pre=&bcache.head;
		bcache.head.next->pre=p;
		bcache.head.next=p;
    }
}

/*���ݿ��ź��豸�Ż�ȡ�黺���ṹ*/
static struct buf * bufget(uint dev,uint sector)
{
	struct buf *p;
	
	acquire(&bcache.lock);
	
	/*��������������Ϊæµ�����أ�æµ��˯�ߵȴ�*/
	loop:
	for(p = bcache.cache.next; p!= &bcache.head; p = p->next)
	{
		if(p.dev == dev && p.sector == sector)
		{
			if(!(p->flags & b_busy))
			{
				p->flags = (p->flags | b_busy);
				release(&bcache.lock);
				return p;
			}
			sleep(p,&bcache.lock);
			goto loop;//���⾺������ 
		}
	}
	
	/*Ϊ�������仺���*/ 
	for(p = bcache.head.pre; p!= &bcache.head; p = p.pre) 
	{
		if((p->flags & b_busy)==0 && (p->flags & b_dirty)==0)
		{
			p->dev=dev;
			p->sector=sector;
			p->flags=b_busy;
			release(&bcache.lock);
			return p;
		}
	}
	
	panic("bufget:no buffers");//�������� 
} 

/*�Ӵ���ȡ��һ����뻺����*/ 
struct buf * bufread(uint dev,uint sector)
{
	struct buf *p;
	
	p=bufget(dev,sector);
	if(!(p->flags & b_valid))
	{
		iderw(p); //��Ч�������ݶ����ں� 
	}
	return p;
}

/*������������д������*/
void bufwrite(struct buf *p)
{
	if(!(p->flags & b_busy))
	{
		panic("bwrite");
	} 
	p->flags = (p->flags | b_busy);//״̬����Ϊb_busy 
	idrew(p);	
} 

/*�ͷŻ�����Լ����û�LRU*/
void bufrelse(struct buf *p)
{
	if(!(p->flags & b_busy))
	{
		panic("brelse");
	} 
	
	acquire(&bcache.lock);
	
	b->next->prev = b->prev;
	b->prev->next = b->next;
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;

    p->flags = (p->flags & ~b_busy);
	wakeup(p);
	
	release(&bcache.lock);
}
}
 
