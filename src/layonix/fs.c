/*���̿�ķ����ͷŵȲ���*/
#include "param_yonix.h"//**+
#include "yotypes.h"//**+
#include "types.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "mmu.h"
#include "proc.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"
#include "file.h"

#define min(a, b) ((a) < (b) ? (a) : (b))
static void itrunc(struct inode*);//�ͷ�inode�е����ݿ�
struct superblock sb; 

//��������
void readsb(int dev, struct superblock *sb)
{
	struct buf *bp;

	bp = bread(dev, 1);
	memmove(sb, bp->data, sizeof(*sb));
	brelse(bp);
}
//���һ����
static void bzero(int dev, int bno)
{
	struct buf *bp;

	bp = bread(dev, bno);
	memset(bp->data, 0, BSIZE);
	log_write(bp);
	brelse(bp);
}
//����һ�����̿�
static uint balloc(uint dev)
{
	int b, bi, m;
	struct buf *bp;

	bp = 0;
	for(b = 0; b < sb.size; b += BPB){
		bp = bread(dev, BBLOCK(b, sb));
		for(bi = 0; bi < BPB && b + bi < sb.size; bi++){
		m = 1 << (bi % 8);
		if((bp->data[bi/8] & m) == 0){  // ���Ƿ����
			bp->data[bi/8] |= m; 
			log_write(bp);
			brelse(bp);
			bzero(dev, b + bi);
			return b + bi;
		}
		}
		brelse(bp);
	}
	panic("balloc: out of blocks");
}
// �ͷ�һ�����̿�
static void bfree(int dev, uint b)
{
	struct buf *bp;
	int bi, m;

	readsb(dev, &sb);
	bp = bread(dev, BBLOCK(b, sb));
	bi = b % BPB;
	m = 1 << (bi % 8);
	if((bp->data[bi/8] & m) == 0)
		panic("freeing free block");
	bp->data[bi/8] &= ~m;
	log_write(bp);
	brelse(bp);
}

// inodes
struct {
	struct inode inode[NINODE];
}icache;
//��ӡ�����豸�Ͽ���Ϣ
void iinit(int dev)
{
	int i = 0;
  
	for(i = 0; i < NINODE; i++) {
		initsleeplock(&icache.inode[i].lock, "inode");
	}
  
	readsb(dev, &sb);
	cprintf("sb: size %d nblocks %d ninodes %d nlog %d logstart %d inodestart %d bmap start %d\n", sb.size, sb.nblocks,sb.ninodes, sb.nlog, sb.logstart, sb.inodestart,sb.bmapstart);
}
//�����豸�ţ�inode�ţ�����inodeָ��
static struct inode* iget(uint dev, uint inum)
{
	struct inode *ip, *empty;

	empty = 0;
	for (ip = &icache.inode[0]; ip < &icache.inode[NINODE]; ip++) {
		if (ip->ref > 0 && ip->dev == dev && ip->inum == inum) {
			ip->ref++;
			return ip;
		}
		if (empty == 0 && ip->ref == 0)
			empty = ip;
	}

	if (empty == 0)
		panic("iget: no inodes");

	ip = empty;
	ip->dev = dev;
	ip->inum = inum;
	ip->ref = 1;
	ip->flags = 0;

	return ip;
}
//�ڴ���������һ��inode
struct inode* ialloc(uint dev, short type)
{
	int inum;
	struct buf *bp;
	struct dinode *dip;

	for(inum = 1; inum < sb.ninodes; inum++){
		bp = bread(dev, IBLOCK(inum, sb));
		dip = (struct dinode*)bp->data + inum%IPB;
		if(dip->type == 0){  // ����inode typeֵΪ0
		memset(dip, 0, sizeof(*dip));
		dip->type = type;
		log_write(bp);   // ��¼���ڴ����ϱ�����
		brelse(bp);
		return iget(dev, inum);
		}
		brelse(bp);
	}
	panic("ialloc: no inodes");
}
//���ڴ���inode���Ƶ�������inode
void iupdate(struct inode *ip)
{
	struct buf *bp;
	struct dinode *dip;

	bp = bread(ip->dev, IBLOCK(ip->inum, sb));
	dip = (struct dinode*)bp->data + ip->inum%IPB;
	dip->type = ip->type;
	dip->major = ip->major;
	dip->minor = ip->minor;
	dip->nlink = ip->nlink;
	dip->size = ip->size;
	memmove(dip->addrs, ip->addrs, sizeof(ip->addrs));
	log_write(bp);
	brelse(bp);
}
//����inode�����ü���
struct inode* idup(struct inode *ip)
{
	ip->ref++;
	return ip;
}
//��סinode�����Ӵ��̶���inode��Ϣ
void ilock(struct inode *ip)
{
	struct buf *bp;
	struct dinode *dip;

	if(ip == 0 || ip->ref < 1)
		panic("ilock");

	acquiresleep(&ip->lock);

	if(!(ip->flags & I_VALID)){
		bp = bread(ip->dev, IBLOCK(ip->inum, sb));
		dip = (struct dinode*)bp->data + ip->inum%IPB;
		ip->type = dip->type;
		ip->major = dip->major;
		ip->minor = dip->minor;
		ip->nlink = dip->nlink;
		ip->size = dip->size;
		memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
		brelse(bp);
		ip->flags |= I_VALID;
		if(ip->type == 0)
			panic("ilock: no type");
	}
}
//�ͷ�inode�ϵ���
void iunlock(struct inode *ip)
{
  if(ip == 0 || !holdingsleep(&ip->lock) || ip->ref < 1)
    panic("iunlock");

  releasesleep(&ip->lock);
}
//�ͷ�inodeָ��
void iput(struct inode *ip)
{
	if(ip->ref == 1 && (ip->flags & I_VALID) && ip->nlink == 0){
		//�����ü���Ϊ1ʱ���մ�inode
		itrunc(ip);
		ip->type = 0;
		iupdate(ip);
		ip->flags = 0;
	}
	ip->ref--;
}
//�򿪱�����inode��Ȼ���ͷ�inode
void iunlockput(struct inode *ip)
{
	iunlock(ip);
	iput(ip);
}
//����inode��ip�е�bn�����ݿ飬���û��������ݿ飬�����һ��
static uint bmap(struct inode *ip, uint bn)
{
	uint addr, *a;
	struct buf *bp;

	if(bn < NDIRECT){//ֱ�����ݿ�
		if((addr = ip->addrs[bn]) == 0)
		ip->addrs[bn] = addr = balloc(ip->dev);
		return addr;
	}
	bn -= NDIRECT;

	if(bn < NINDIRECT){//���ؼ�����ݿ�
		if((addr = ip->addrs[NDIRECT]) == 0)
		ip->addrs[NDIRECT] = addr = balloc(ip->dev);
		bp = bread(ip->dev, addr);
		a = (uint*)bp->data;
		if((addr = a[bn]) == 0){
		a[bn] = addr = balloc(ip->dev);
		log_write(bp);
		}
		brelse(bp);
		return addr;
	}

	panic("bmap: out of range");
}
//�ͷ�inode�е����ݿ�
static void itrunc(struct inode *ip)
{
	int i, j;
	struct buf *bp;
	uint *a;

	for(i = 0; i < NDIRECT; i++){
		if(ip->addrs[i]){
		bfree(ip->dev, ip->addrs[i]);
		ip->addrs[i] = 0;
		}
	}

	if(ip->addrs[NDIRECT]){
		bp = bread(ip->dev, ip->addrs[NDIRECT]);
		a = (uint*)bp->data;
		for(j = 0; j < NINDIRECT; j++){
			if(a[j])
				bfree(ip->dev, a[j]);
		}
		brelse(bp);
		bfree(ip->dev, ip->addrs[NDIRECT]);
		ip->addrs[NDIRECT] = 0;
	}

	ip->size = 0;
	iupdate(ip);
}
//��inode��Ϣ������stat�ṹ����
void stati(struct inode *ip, struct stat *st)
{
	st->dev = ip->dev;
	st->ino = ip->inum;
	st->type = ip->type;
	st->nlink = ip->nlink;
	st->size = ip->size;
}
//��inode�ж�ȡ����
int readi(struct inode *ip, char *dst, uint off, uint n)
{
	uint tot, m;
	struct buf *bp;

	if(ip->type == T_DEV){//�ļ�����Ϊ�豸
		if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].read)
			return -1;
		return devsw[ip->major].read(ip, dst, n);
	}

	if(off > ip->size || off + n < off)
		return -1;
	if(off + n > ip->size)
		n = ip->size - off;

	for(tot=0; tot<n; tot+=m, off+=m, dst+=m){
		bp = bread(ip->dev, bmap(ip, off/BSIZE));
		m = min(n - tot, BSIZE - off%BSIZE);
    /*
    cprintf("data off %d:\n", off);
    for (int j = 0; j < min(m, 10); j++) {
      cprintf("%x ", bp->data[off%BSIZE+j]);
    }
    cprintf("\n");
    */
		memmove(dst, bp->data + off%BSIZE, m);
		brelse(bp);
	}
	return n;
}
//д���ݵ�inode
int writei(struct inode *ip, char *src, uint off, uint n)
{
	uint tot, m;
	struct buf *bp;
	
	if(ip->type == T_DEV){//�ļ�����Ϊ�豸
		if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].write)
			return -1;
		return devsw[ip->major].write(ip, src, n);
	}

	if(off > ip->size || off + n < off)
		return -1;
	if(off + n > MAXFILE*BSIZE)
		return -1;

	for(tot=0; tot<n; tot+=m, off+=m, src+=m){
		bp = bread(ip->dev, bmap(ip, off/BSIZE));
		m = min(n - tot, BSIZE - off%BSIZE);
		memmove(bp->data + off%BSIZE, src, m);
		log_write(bp);
		brelse(bp);
	}

	if(n > 0 && off > ip->size){
		ip->size = off;
		iupdate(ip);
	}
	return n;
}
//Ŀ¼
int namecmp(const char *s, const char *t)
{
  return strncmp(s, t, DIRSIZ);//�Ƚ��ַ�����DIRSIZ���ַ�
}
//����Ŀ¼��ָ�����ֵ���Ŀ������ҵ�����������һ��ָ�����i�ڵ��ָ��
struct inode* dirlookup(struct inode *dp, char *name, uint *poff)
{
	uint off, inum;
	struct dirent de;

	if(dp->type != T_DIR)
		panic("dirlookup not DIR");

	for (off = 0; off < dp->size; off += sizeof(de)) {
		if (readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
			panic("dirlink read");
		if (de.inum == 0)
			continue;
		if (namecmp(name, de.fname) == 0) {
			// entry matches path element
			if (poff)
				*poff = off;
			inum = de.inum;
			return iget(dp->dev, inum);
		}
	}
	return 0;
}
//дһ���µ�Ŀ¼��Ŀ��ָ��Ŀ¼inode��
int dirlink(struct inode *dp, char *name, uint inum)
{
	int off;
	struct dirent de;
	struct inode *ip;

	// ���������Ƿ��Ѵ���Ŀ¼�У����ڷ���-1
	if((ip = dirlookup(dp, name, 0)) != 0){
		iput(ip);
	return -1;
	}

  // Ѱ��һ���յ�Ŀ¼��
	for(off = 0; off < dp->size; off += sizeof(de)){
		if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
			panic("dirlink read");
		if(de.inum == 0)
			break;
	}

	strncpy(de.fname, name, DIRSIZ);
	de.inum = inum;
	if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
	    panic("dirlink");

	return 0;
}

//����path��ÿһ���֣�nameΪ��һ���֡�/�������֣�pathΪʣ��·��
//���磺skipelem("///a//bb", name) = "bb", setting name = "a"��skipelem("a/bb/c", name) = "bb/c", setting name = "a"
static char* skipelem(char *path, char *name)
{
	char *s;
	int len;

	while(*path == '/')
		path++;
	if(*path == 0)
		return 0;
	s = path;
	while(*path != '/' && *path != 0)
		path++;
	len = path - s;
	if(len >= DIRSIZ)
		memmove(name, s, DIRSIZ);
	else {
		memmove(name, s, len);
		name[len] = 0;
	}
	while(*path == '/')
		path++;
	return path;
}
//����·������ȡ��Ӧname��inode
static struct inode* namex(char *path, int nameiparent, char *name)
{
	struct inode *ip, *next;

	//���·���Է�б�ܿ�ʼ�������ͻ�Ӹ�Ŀ¼��ʼ��������������ӵ�ǰĿ¼��ʼ��
	if(*path == '/')
		ip = iget(ROOTDEV, ROOTINO);
	else
		ip = idup(proc->p_cdir);

	while((path = skipelem(path, name)) != 0){
		ilock(ip);
		if(ip->type != T_DIR){
		iunlockput(ip);
		return 0;
		}
		if(nameiparent && *path == '\0'){
		iunlock(ip);
		return ip;
		}
		if((next = dirlookup(ip, name, 0)) == 0){
		iunlockput(ip);
		return 0;
		}
		iunlockput(ip);
		ip = next;
	}
	if(nameiparent){
		iput(ip);
		return 0;
	}
	return ip;
}
//����path�����ض��õ�inode
struct inode* namei(char *path)
{
	char name[DIRSIZ];
	return namex(path, 0, name);
}
//����path�������һ��Ԫ��֮ǰֹͣ
struct inode* nameiparent(char *path, char *name)
{
	return namex(path, 1, name);
}
