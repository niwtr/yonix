#define T_DIR  1   // Ŀ¼
#define T_FILE 2   // �ļ� 
#define T_DEV  3   // �豸 

struct start{
	short type;// �ļ����� 
	int dev; // �ļ�ϵͳ�����豸 
	uint ino; // i�ڵ���Ŀ 
	short nlink; //�����ļ����� 
	uint size; //�ļ���С 
};
