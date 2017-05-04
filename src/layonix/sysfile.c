//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//

//文件描述符层
//系统调用模块
#include "types.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "mmu.h"
#include "proc.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"

//从系统调用的参数提取出文件描述符，并根据文件描述符找到对应文件结构
static int argfd(int n, int *pfd, struct file **pf)
{
	int fd;
	struct file *f;

	if (sysc_argint(n, &fd) < 0)
		return -1;
	if (fd < 0 || fd >= NOFILE || (f = proc->p_of[fd]) == 0)
		return -1;
	if (pfd)
		*pfd = fd;
	if (pf)
		*pf = f;
	return 0;
}

//为文件分配文件描述符
static int fdalloc(struct file *f)
{
	int fd;

	for (fd = 0; fd < NOFILE; fd++) {
		if (proc->p_of[fd] == 0) {
			proc->p_of[fd] = f;
			return fd;
		}
	}
	return -1;
}

//实现文件重复引用
int sys_dup(void)
{
	struct file *f;
	int fd;

	if (argfd(0, 0, &f) < 0)
		return -1;
	if ((fd = fdalloc(f)) < 0)
		return -1;
	filedup(f);
	return fd;
}

//从文件描述符读取数据
int sys_read(void)
{
	struct file *f;
	int n;
	char *p;

	if (argfd(0, 0, &f) < 0 || sysc_argint(2, &n) < 0 || sysc_argptr(1, &p, n) < 0)
		return -1;
	return fileread(f, p, n);
}

//从文件描述符对文件结构进行写操作
int sys_write(void)
{
	struct file *f;
	int n;
	char *p;

	if (argfd(0, 0, &f) < 0 || sysc_argint(2, &n) < 0 || sysc_argptr(1, &p, n) < 0)
		return -1;
	return filewrite(f, p, n);
}

//当文件被引用数为0时，释放inode
int sys_close(void)
{
	int fd;
	struct file *f;

	if (argfd(0, &fd, &f) < 0)
		return -1;
	proc->p_of[fd] = 0;
	fileclose(f);
	return 0;
}

int sys_fstat(void)
{
	struct file *f;
	struct stat *st;

	if (argfd(0, 0, &f) < 0 || sysc_argptr(1, (void*)&st, sizeof(*st)) < 0)
		return -1;
	return filestat(f, st);
}

//建立一个硬链接，实现多个目录可指向一个inode（已存在）
int sys_link(void)
{
	char name[DIRSIZ], *new, *old;
	struct inode *dp, *ip;

	if (sysc_argstr(0, &old) < 0 || sysc_argstr(1, &new) < 0)
		return -1;

	begin_op();
	if ((ip = namei(old)) == 0) {
    end_op();
		return -1;
	}

	ilock(ip);
	if (ip->type == T_DIR) {
		iunlockput(ip);
		end_op();
		return -1;
	}

	ip->nlink++;
	iupdate(ip);
	iunlock(ip);

	if ((dp = nameiparent(new, name)) == 0)
		goto bad;
	ilock(dp);
	if (dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0) {
		iunlockput(dp);
		goto bad;
	}
	iunlockput(dp);
	iput(ip);

	end_op();
	return 0;

bad:
	ilock(ip);
	ip->nlink--;
	iupdate(ip);
	iunlockput(ip);
	end_op();
	return -1;
}


static int isdirempty(struct inode *dp)
{
	int off;
	struct dirent de;

	for (off = 2 * sizeof(de); off<dp->size; off += sizeof(de)) {
		if (readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
			panic("isdirempty: readi");
		if (de.inum != 0)
			return 0;
	}
	return 1;
}

//取消一个硬链接
int sys_unlink(void)
{
	struct inode *ip, *dp;
	struct dirent de;
	char name[DIRSIZ], *path;
	uint off;

	if (sysc_argstr(0, &path) < 0)
		return -1;

	begin_op();
	if ((dp = nameiparent(path, name)) == 0) {
		end_op();
		return -1;
	}

	ilock(dp);

	
	if (namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
		goto bad;

	if ((ip = dirlookup(dp, name, &off)) == 0)
		goto bad;
	ilock(ip);

	if (ip->nlink < 1)
		panic("unlink: nlink < 1");
	if (ip->type == T_DIR && !isdirempty(ip)) {
		iunlockput(ip);
		goto bad;
	}

	memset(&de, 0, sizeof(de));
	if (writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
		panic("unlink: writei");
	if (ip->type == T_DIR) {
		dp->nlink--;
		iupdate(dp);
	}
	iunlockput(dp);

	ip->nlink--;
	iupdate(ip);
	iunlockput(ip);

	end_op();

	return 0;

bad:
	iunlockput(dp);
	end_op();
	return -1;
}

//创建一个新的inode并初始化
static struct inode* create(char *path, short type, short major, short minor)
{
	uint off;
	struct inode *ip, *dp;
	char name[DIRSIZ];

	if ((dp = nameiparent(path, name)) == 0)
		return 0;
	ilock(dp);

	if ((ip = dirlookup(dp, name, &off)) != 0) {
		iunlockput(dp);
		ilock(ip);
		if (type == T_FILE && ip->type == T_FILE)
			return ip;
		iunlockput(ip);
		return 0;
	}

	if ((ip = ialloc(dp->dev, type)) == 0)
		panic("create: ialloc");

	ilock(ip);
	ip->major = major;
	ip->minor = minor;
	ip->nlink = 1;
	iupdate(ip);

	if (type == T_DIR) {  
		dp->nlink++;  
		iupdate(dp);
		
		if (dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
			panic("create dots");
	}

	if (dirlink(dp, name, ip->inum) < 0)
		panic("create: dirlink");

	iunlockput(dp);

	return ip;
}

//为inode分配文件和文件描述符，并填充文件
int sys_open(void)
{

	char *path;
	int fd, omode;
	struct file *f;
	struct inode *ip;

	if (sysc_argstr(0, &path) < 0 || sysc_argint(1, &omode) < 0)
		return -1;

	begin_op();

	if (omode & O_CREATE) {
		ip = create(path, T_FILE, 0, 0);
		if (ip == 0) {
			end_op();
			return -1;

		}
	}
	else {
		if ((ip = namei(path)) == 0) {
			end_op();
			return -1;
		}
		ilock(ip);
		if (ip->type == T_DIR && omode != O_RDONLY) {
			iunlockput(ip);
			end_op();
			return -1;
		}
	}

	if ((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0) {
		if (f)
			fileclose(f);
		iunlockput(ip);
		end_op();
		return -1;
	}
	iunlock(ip);
	end_op();

	f->type = FD_INODE;
	f->ip = ip;
	f->off = 0;
	f->readable = !(omode & O_WRONLY);
	f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
	return fd;
}

int sys_mkdir(void)
{
	char *path;
	struct inode *ip;

	begin_op();
	if (sysc_argstr(0, &path) < 0 || (ip = create(path, T_DIR, 0, 0)) == 0) {
		end_op();
		return -1;
	}
	iunlockput(ip);
	end_op();
	return 0;
}

int sys_mknod(void)
{
	struct inode *ip;
	char *path;
	int major, minor;

	begin_op();
	if ((sysc_argstr(0, &path)) < 0 ||
		sysc_argint(1, &major) < 0 ||
		sysc_argint(2, &minor) < 0 ||
		(ip = create(path, T_DEV, major, minor)) == 0) {
		end_op();
		return -1;
	}
	iunlockput(ip);
	end_op();
	return 0;
}

int sys_chdir(void)
{
	char *path;
	struct inode *ip;

	begin_op();
	if (sysc_argstr(0, &path) < 0 || (ip = namei(path)) == 0) {
		end_op();
		return -1;
	}
	ilock(ip);
	if (ip->type != T_DIR) {
		iunlockput(ip);
		end_op();
		return -1;
	}
	iunlock(ip);
	iput(proc->p_cdir);
	end_op();
	proc->p_cdir = ip;
	return 0;
}

int sys_exec(void)
{
	char *path, *argv[MAXARG];
	int i;
	uint uargv, uarg;

	if (sysc_argstr(0, &path) < 0 || sysc_argint(1, (int*)&uargv) < 0) {
		return -1;
	}
	memset(argv, 0, sizeof(argv));
	for (i = 0;; i++) {
		if (i >= NELEM(argv))
			return -1;
		if (_sysc_getint(uargv + 4 * i, (int*)&uarg) < 0)
			return -1;
		if (uarg == 0) {
			argv[i] = 0;
			break;
		}
		if (_sysc_getstr(uarg, &argv[i]) < 0)
			return -1;
	}
	return exec(path, argv);
}

int sys_pipe(void)
{
	int *fd;
	struct file *rf, *wf;
	int fd0, fd1;

	if (sysc_argptr(0, (void*)&fd, 2 * sizeof(fd[0])) < 0)
		return -1;
	if (pipealloc(&rf, &wf) < 0)
		return -1;
	fd0 = -1;
	if ((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0) {
		if (fd0 >= 0)
			proc->p_of[fd0] = 0;
		fileclose(rf);
		fileclose(wf);
		return -1;
	}
	fd[0] = fd0;
	fd[1] = fd1;
	return 0;
}
