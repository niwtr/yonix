/*�黺������˫��������ʽ�洢������ */
struct buf {
	int flags;
	uint dev;
	uint blockno;//����
	struct sleeplock lock;
	uint refcnt;
	struct buf *prev; //�û�ҳ���� 
	struct buf *next;
	struct buf *qnext; //��������
	uchar data[BSIZE]; //����
};
#define B_VALID 0x2  //������ӵ�д���������Ч 
#define B_DIRTY 0x4  //������������Ҫд����� 
