/*�ļ��ṹ*/
struct file {
	enum { FD_NONE, FD_PIPE, FD_INODE } type;//�ļ�����pipe/inode
	int ref; //Ӧ�ô��� 
	char readable;//�ɶ�
	char writable;//��д
	struct pipe *pipe;
	struct inode *ip;
	uint off;
};

/*�ڴ���i���*/
struct inode {
	uint dev;           //�豸�� 
	uint inum;          //i�ڵ��
	int ref;            //ָ���i�ڵ��ָ��
	struct sleeplock lock;
	int flags;          //״̬��ռ�ã����У� 
	short type;         //�ļ�����
	short major;		// ���豸�� 
	short minor;		// ���豸�� 
	short nlink;		//ָ���i�ڵ����Ŀ
	uint size;			//�ļ���С 
	uint addrs[NDIRECT + 1];//���ݿ��ַ��ֱ�ӿ�/��ӿ�
};
#define I_VALID 0x2

/*�豸�ļ�����*/
struct devsw {
	int(*read)(struct inode*, char*, int);
	int(*write)(struct inode*, char*, int);
};

extern struct devsw devsw[];
#define CONSOLE 1

