/*�ļ�ϵͳ���̽ṹ
��0�飺bootl �ļ�ϵͳ��ʹ��
��1�飺�����飬�洢һЩ��Ϣ
��2-n�飺i�ڵ�飬���п飬���ݿ� */

#define BSIZE 512  // ���С
#define NDIRECT 12 //ֱ�Ӵ洢���ݿ���Ŀ 
#define NINDIRECT (BSIZE / sizeof(uint))//��Ӵ洢���ݿ���Ŀ һ��Ϊsizeof(uint)������ 
#define MAXFILE (NDIRECT + NINDIRECT)//���ݿ����洢�� 
  
/*���������ݽṹ*/ 
struct supblock{
	uint size;//�ļ�ϵͳ�ܿ��� 
	unit sdata;//���ݿ��ܿ��� 
	unit sinode;//i�ڵ��ܿ���
	unit slog;//��־���� 
}; 

/*�����ϵ�i�ڵ��ṹ*/
struct dinode{
	short type;//�ļ�����
	short major;// Major device number (T_DEV only)
	short minor;// Minor device number (T_DEV only)
	short nlink;//ָ���i�ڵ��Ŀ¼�� 
	uint fsize;//�ļ���С 
	uint addrs[NINDTRECT+1];//���ݿ��ַ 
}; 

#define IPB (BSIZE / sizeof(struct dinode))  //ÿ����ٸ�i�ڵ� 
#define IBLOCK(i) ((i) / IPB + 2)  //��i��i�ڵ����ڵĿ� 
#define BPB (BSIZE*8)    //ÿ��λͼ����ù���Ĵ�С  
#define BBLOCK(b, ninodes) (b/BPB + (ninodes)/IPB + 3)  //��i�������ڵ�λͼ�� 
#define DIRSIZE 14  //Ŀ¼����Ӧ�ļ���
  
/*Ŀ¼�ļ��ṹ*/ 
struct dirent {  
  ushort inum;  // i�ڵ�� 
  char fname[DIRSIZE]; //�ļ�����  
};  


/*�ں�i���*/
struct inode{
	uint dev; //�豸�� 
	uint inum; //i�ڵ��
	int ref; //ָ���i�ڵ��ָ��
	int flag ; //״̬��ռ�ã����У� 
	
	short type;//�ļ�����
	short major;// Major device number (T_DEV only)
	short minor;// Minor device number (T_DEV only)
	short nlink;//ָ���i�ڵ��Ŀ¼�� 
	uint fsize;//�ļ���С 
	uint addrs[NINDTRECT+1];//���ݿ��ַ 
}; 
/
/*�ļ��ṹ*/ 
struct file{
	enum{fd,dfpipe,fdinode}type;
	int ref;
	char rable;
	char wable;
	struct pipe *pipe;
	struct inode *ip;
}; 


/*�黺������˫��������ʽ�洢������ */
#include"spinlock.h" 
struct buf{
	int flags;//״̬ 
	uint dev;//�豸 
	uint sector;//?? 
	struct buf *pre; //�û�ҳ���� 
	struct buf *next; 
	struct buf *qnext; //�������� 
	uchar data[512]; 
};

struct bufcache{
	struct spinlock lock;
	struct buf cahce[NBUF];
	struct buf head;
};

#define b_busy  0x1 //����״̬ 
#define b_valid 0X2 //����������Ч 
#define b_dirty 0x4 //������Ҫд����� 

#define o_rdonly 0x000
#define o_wdonly 0x001
#define o_rdwd   0x002
#define o_create 0x200 

#define TDIR 1
#define TFILE 2
#define TDEV 3

struct start{
	short type;// �ļ����� 
	int dev; // �ļ�ϵͳ�����豸 
	uint ino; // i�ڵ���Ŀ 
	short nlink; //�����ļ����� 
	uint size; //�ļ���С 
};

