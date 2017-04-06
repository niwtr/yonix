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

/*�ļ��ṹ*/ 
struct file{
	enum{fd,dfpipe,fdinode}type;
	int ref;//Ӧ�ô��� 
	char rable;
	char wable;
	struct pipe *pipe;
	struct inode *ip;
}; 

/*��ӳ�����豸��������*/
struct devsw {
  int (*read)(struct inode*, char*, int);
  int (*write)(struct inode*, char*, int);
};

extern struct devsw devsw[];
