/*�黺������˫��������ʽ�洢������ */
struct buf{
	int flags;//״̬ 
	uint dev;//�豸�� 
	uint sector;//���� 
	struct sleeplock lock;
	struct buf *pre; //�û�ҳ���� 
	struct buf *next; 
	struct buf *qnext; //�������� 
	uchar data[BSIZE]; 
};


#define b_busy  0x1 //����״̬ 
#define b_valid 0X2 //����������Ч 
#define b_dirty 0x4 //������Ҫд����� 
#define NBUF 14//˫�������� 
