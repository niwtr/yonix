//文件描述符
#include "types.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"

struct devsw devsw[NDEV];//设备文件存储数组
struct {
	struct file file[NFILE];
} ftable;

void fileinit(void)
{
}

//分配一个文件结构
struct file* filealloc(void)
{
	struct file *f;

	for (f = ftable.file; f < ftable.file + NFILE; f++) {
		if (f->ref == 0) {
			f->ref = 1;
			return f;
		}
	}
	return 0;
}

//增加文件的引用计数
struct file* filedup(struct file *f)
{
	if(f->ref < 1)
		panic("filedup");
	f->ref++;
	return f;
}

//关闭文件结构，文件引用计数位0时，回收此文件结构
void fileclose(struct file *f)
{
	struct file ff;

	if(f->ref < 1)
		panic("fileclose");
	if(--f->ref > 0){
		return;
	}
	ff = *f;
	f->ref = 0;
	f->type = FD_NONE;

	if(ff.type == FD_PIPE)
		pipeclose(ff.pipe, ff.writable);
	else if(ff.type == FD_INODE){
		begin_op();
		iput(ff.ip);
		end_op();
	}	
}

//获取文件的stat结构统计信息
int filestat(struct file *f, struct stat *st)
{
	if(f->type == FD_INODE){
		ilock(f->ip);
		stati(f->ip, st);
		iunlock(f->ip);
		return 0;
	}
	return -1;
}

//读文件
int fileread(struct file *f, char *addr, int n)
{
	int r;

	if(f->readable == 0)
		return -1;
	if(f->type == FD_PIPE)
		return piperead(f->pipe, addr, n);
	if(f->type == FD_INODE){
		ilock(f->ip);
		if((r = readi(f->ip, addr, f->off, n)) > 0)
			f->off += r;
		iunlock(f->ip);
		return r;
	}
	panic("fileread");
}

// 写文件
int filewrite(struct file *f, char *addr, int n)
{
	int r;

	if(f->writable == 0)
		return -1;
	if(f->type == FD_PIPE)
		return pipewrite(f->pipe, addr, n);
	if(f->type == FD_INODE){
		//依次写文件块，避免超过块最大SIZE
		int max = ((LOGSIZE-1-1-2) / 2) * 512;
		int i = 0;

		while(i < n){
			int n1 = n - i;
			if(n1 > max)
			n1 = max;

			begin_op();
			ilock(f->ip);

			if ((r = writei(f->ip, addr + i, f->off, n1)) > 0)
				f->off += r;

			iunlock(f->ip);
			end_op();

			if(r < 0)
				break;
			if(r != n1)
				panic("short filewrite");
			i += r;
		}
		return i == n ? n : -1;
	}
	panic("filewrite");
}

