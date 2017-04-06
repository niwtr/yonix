#define T_DIR  1   // 目录
#define T_FILE 2   // 文件 
#define T_DEV  3   // 设备 

struct start{
	short type;// 文件类型 
	int dev; // 文件系统磁盘设备 
	uint ino; // i节点数目 
	short nlink; //连接文件次数 
	uint size; //文件大小 
};
