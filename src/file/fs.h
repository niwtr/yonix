/*�ļ�ϵͳ���̽ṹ*/
//��0�飺bootl �ļ�ϵͳ��ʹ��
//��1�飺�����飬�洢һЩ��Ϣ
//��2 - n�飺i�ڵ�飬λͼ�飬���ݿ飬������

#define ROOTINO 1  
#define BSIZE 512  //���С
#define NDIRECT 12 //ֱ�Ӵ洢���ݿ���Ŀ 
#define NINDIRECT (BSIZE / sizeof(uint))//��Ӵ洢���ݿ���Ŀ
#define MAXFILE (NDIRECT + NINDIRECT)//���ݿ����洢�� 


//���������ݽṹ
struct superblock {
	uint size;         // �ļ�ϵͳ�ܿ��� 
	uint nblocks;      // ���ݿ��ܿ���
	uint ninodes;      // i�ڵ��ܿ���
	uint nlog;         // ��־����
	uint logstart;     // ��־��ĵ�һ��
	uint inodestart;   // inode��ĵ�һ��
	uint bmapstart;    // λͼ��ĵ�һ��
};

//i�ڵ��ṹ
struct dinode {
	short type;           //�ļ����ͣ��ļ�/Ŀ¼/�����ļ�
	short major;          // ���豸��(T_DEV only)
	short minor;          // ���豸��(T_DEV only)
	short nlink;		  //ָ���i�ڵ����Ŀ
	uint size;            //�ļ���С
	uint addrs[NDIRECT + 1];//���ݿ��ַ 
};

#define IPB(BSIZE / sizeof(struct dinode))//ÿ����ٸ�i�ڵ�
#define IBLOCK(i, sb)((i) / IPB + sb.inodestart)//��i��i�ڵ����ڵĿ� 
#define BPB(BSIZE*8) //ÿ��λͼ����ù���Ĵ�С 
#define BBLOCK(b, sb) (b/BPB + sb.bmapstart)//��i�������ڵ�λͼ��
#define DIRSIZ 14//Ŀ¼����Ӧ�ļ���

/*Ŀ¼��Ŀ�ṹ*/
struct dirent {
	ushort inum;  // i�ڵ�� 
	char fname[DIRSIZ]; //�ļ�����  
};
