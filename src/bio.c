/*�黺���*/ 
#include "types.h"
#include "param.h"
#include "defs.h"
#include "sleeplock.h"
#include "buf.h" 

struct bufcache{
	struct buf cache[NBUF]; 
	struct buf head; //�����ʹ�õ� 
}bcache;

/*����һ��˫����������*/
void binit()  
{
	struct buf *p; 
   
    //����ֻ��ͷ����˫������ 
	bcache.head.prev = &bcache.head;  
	bcache.head.next = &bcache.head;
	
    for(p = bcache.cache; p < bcache.cache+NBUF; p++)
    {
    	p->dev=-1;
    	p->next=bcache.head.next;
    	p->pre=&bcache.head;
    	 initsleeplock(&p->lock, "buffer");
		bcache.head.next->pre=p;
		bcache.head.next=p;
    }
}

/*���ݿ��ź��豸�Ż�ȡ�黺���ṹ*/
static struct buf * bufget(uint dev,uint sector)
{
	struct buf *p;
	

	/*��������������Ϊæµ�����أ�æµ��˯�ߵȴ�*/
	loop:
	for(p = bcache.cache.next; p!= &bcache.head; p = p->next)
	{
		if(p.dev == dev && p.sector == sector)
		{
			if(!(p->flags & b_busy))
			{
				p->flags = (p->flags | b_busy);
				return p;
			}
			acquiresleep(&p->lock);
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
			acquiresleep(&p->lock);
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
	
	releasesleep(&p->lock);
	
	//�ƶ������� �������ʹ��������� 
	p->next->pre = p->pre;
	p->pre->next = p->next;
    p->next = bcache.head.next;
    p->pre = &bcache.head;
    bcache.head.next->pre = p;
    bcache.head.next = p; 

    p->flags = (p->flags & ~b_busy);	
}
}
 
