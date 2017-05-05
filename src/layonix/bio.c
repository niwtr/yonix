/*�黺���*/
#include "types.h"
#include "defs.h"
#include "param.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"


struct {
	struct buf buf[NBUF];//���������
	struct buf head;
} bcache;

//����һ��˫������������
void binit()
{
	struct buf *b;
    //����ֻ��ͷ����˫������
	bcache.head.prev = &bcache.head;
	bcache.head.next = &bcache.head;

    for(b = bcache.buf; b < bcache.buf+NBUF; b++)
    {
    	b->dev=-1;
    	b->next=bcache.head.next;
    	b->prev=&bcache.head;
    	 initsleeplock(&b->lock, "buffer");//������������Ϊbuffer 
		bcache.head.next->prev=b;
		bcache.head.next=b;
    }
}

//���ݿ��źͿ��Ż�ȡ�黺���ṹ
static struct buf* bget(uint dev, uint blockno)
{
	struct buf *b;

	//��������������Ϊæµ�����أ�æµ��˯�ߵȴ�
	for (b = bcache.head.next; b != &bcache.head; b = b->next) {
		if (b->dev == dev && b->blockno == blockno) {
			b->refcnt++;
			acquiresleep(&b->lock);
			return b;
		}
	}

	//���������޻���飬Ϊ�������仺���
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
	panic("bget: no buffers");//�������� 
}

//�Ӵ���ȡ��һ����뻺����
struct buf* bread(uint dev, uint blockno)
{
	struct buf *b;

	b = bget(dev, blockno);
	if (!(b->flags & B_VALID)) {
		iderw(b);//��buf��Ч���Ѵ������ݶ��������
	}
	return b;
}

//������������д������
//���ͷŻ�����֮ǰ����д�����
void bwrite(struct buf *b)
{
	if (!holdingsleep(&b->lock))
		panic("bwrite");
	b->flags |= B_DIRTY;
	iderw(b);
}

//�ͷŻ�����Լ����û�LRU
void brelse(struct buf *b)
{
	if (!holdingsleep(&b->lock))
		panic("brelse");

	releasesleep(&b->lock);

	b->refcnt--;
	if (b->refcnt == 0) {
		//�ƶ������� �������ʹ���������
		//�Ӷ���ȡ��p�ڵ�
		b->next->prev = b->prev;
		b->prev->next = b->next;
		//��b�ڵ��������ͷ
		b->next = bcache.head.next;
		b->prev = &bcache.head;
		bcache.head.next->prev = b;
		bcache.head.next = b;
	}
}


