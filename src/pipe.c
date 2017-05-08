#include "param_yonix.h"
#include "yotypes.h"
#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"

#define PIPESIZE 512

struct pipe {
	char data[PIPESIZE];
	uint nread;     // 从缓冲区读出的字节数
	uint nwrite;    // 从缓冲区写入的字节数
	int readopen;   // 打开读端口
	int writeopen;  // 打开写端口
};

//分配一个管道
int pipealloc(struct file **f0, struct file **f1)
{
	struct pipe *p;

	p = 0;
	*f0 = *f1 = 0;
	if ((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
		goto bad;
	if ((p = (struct pipe*)kalloc()) == 0)
		goto bad;
	p->readopen = 1;
	p->writeopen = 1;
	p->nwrite = 0;
	p->nread = 0;
	(*f0)->type = FD_PIPE;
	(*f0)->readable = 1;
	(*f0)->writable = 0;
	(*f0)->pipe = p;
	(*f1)->type = FD_PIPE;
	(*f1)->readable = 0;
	(*f1)->writable = 1;
	(*f1)->pipe = p;
	return 0;

bad:
	if (p)
		kfree((char*)p);
	if (*f0)
		fileclose(*f0);
	if (*f1)
		fileclose(*f1);
	return -1;
}
void pipeclose(struct pipe *p, int writable)
{
	if (writable) {
		p->writeopen = 0;
		wakeup(&p->nread);
	}
	else {
		p->readopen = 0;
		wakeup(&p->nwrite);
	}
	if (p->readopen == 0 && p->writeopen == 0) {
		kfree((char*)p);
	}
	else
		;
}
//写数据到pipe
int pipewrite(struct pipe *p, char *addr, int n)
{
	int i;

	for (i = 0; i < n; i++) {
		while (p->nwrite == p->nread + PIPESIZE) {  //缓冲区写满了
			if (p->readopen == 0 || proc->p_killed) {
				return -1;
			}
			wakeup(&p->nread);
			sleep(&p->nwrite);  //sleep写者
		}
		p->data[p->nwrite++ % PIPESIZE] = addr[i];
	}
	wakeup(&p->nread);  //wakeup读者
	return n;
}
//从pipe读取数据
int piperead(struct pipe *p, char *addr, int n)
{
	int i;

	while (p->nread == p->nwrite && p->writeopen) {  //缓冲区为空
		if (proc->p_killed) {
			return -1;
		}
		sleep(&p->nread); //sleep读者
	}
	for (i = 0; i < n; i++) {  //拷贝管道中的数据
		if (p->nread == p->nwrite)
			break;
		addr[i] = p->data[p->nread++ % PIPESIZE];
	}
	wakeup(&p->nwrite);  //wakeup写者
	return i;
}
