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

/*�����ϵĽ���ṹ*/
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
