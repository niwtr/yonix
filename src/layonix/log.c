#include "types.h"
#include "defs.h"
#include "param.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

//每一次系统调用都需要调用begin_op()标记起始,end_op()标记结束.
//通常情况下，begin_op()仅增加队列中系统调用个数，但当它发觉日志区域已经因用完而关闭时
//会睡眠当前进程直至最近的end_op()结束.

//日志初始块
struct logheader {
	int n;//当前有效数据块的计数
	int block[LOGSIZE];//数组的值为对应数据块的内容应该写入文件系统中的哪一块
};

//日志数据块
struct log {
	int start;       //起始位置
	int size;		 //该日志所占大小
	int outstanding; // 正在被执行的文件系统系统调用个数
	int committing;  // 状态：是否提交
	int dev;
	struct logheader lh;
};
struct log log;

static void recover_from_log(void);//从日志中恢复
static void commit();//提交日志

//初始化日志块
void initlog(int dev){
	if (sizeof(struct logheader) >= BSIZE)
		panic("initlog: too big logheader");

	struct superblock sb;//超级块
	readsb(dev, &sb);
	log.start = sb.logstart;
	log.size = sb.nlog;
	log.dev = dev;
	recover_from_log();//检验上次读写操作是否成功，不成功则恢复数据
}

// 正式更新磁盘数据
static void install_trans(void){
	int tail;
	for (tail = 0; tail < log.lh.n; tail++) {
		struct buf *lbuf = bread(log.dev, log.start + tail + 1); // 读取日志
		struct buf *dbuf = bread(log.dev, log.lh.block[tail]); // 读中继区
		memmove(dbuf->data, lbuf->data, BSIZE);  // 将日志覆盖至中继区
		bwrite(dbuf);  // 最终写入磁盘
		brelse(lbuf);
		brelse(dbuf);
	}
}

// 读取日志初始块至内存
static void read_head(void)
{
	struct buf *buf = bread(log.dev, log.start);
	struct logheader *lh = (struct logheader *) (buf->data);
	int i;
	log.lh.n = lh->n;//上次操作是否崩溃标志，非零则未发生崩溃
	for (i = 0; i < log.lh.n; i++) {
		log.lh.block[i] = lh->block[i];
	}
	brelse(buf);
}

//将内存日志初始块写入磁盘,发生在当前日志更新提交时
static void write_head(void)
{
	struct buf *buf = bread(log.dev, log.start);
	struct logheader *hb = (struct logheader *) (buf->data);
	int i;
	hb->n = log.lh.n;
	for (i = 0; i < log.lh.n; i++) {
		hb->block[i] = log.lh.block[i];
	}
	bwrite(buf);
	brelse(buf);
}

//从日志恢复文件
static void recover_from_log(void)
{
	read_head();
	install_trans(); // 确认后，即将数据由日志拷入磁盘
	log.lh.n = 0;
	write_head(); // 清空该日志块
}

// 在每一次系统调用开始时调用该程序
void begin_op(void)
{
	while (1) {
		if (log.committing) {
			sleep(&log);
		}
		else if (log.lh.n + (log.outstanding + 1)*MAXOPBLOCKS > LOGSIZE) {
			// 此次操作可能会耗尽日志空间; 等待提交.
			sleep(&log);
		}
		else {
			log.outstanding += 1;//增加操作次数
			break;
		}
	}
}

// 系统调用终止时被调用
// 确认这次调用是否为最后一次操作
void end_op(void)
{
	int do_commit = 0;

	log.outstanding -= 1;//减少操作次数
	if (log.committing)
		panic("log.committing");
	if (log.outstanding == 0) {
		do_commit = 1;
		log.committing = 1;
	}
	else {
		// begin_op() 此前在等待空闲日志空间
		wakeup(&log);
	}

	if (do_commit) {
		// 没有任何进程时，提交日志
		commit();
		log.committing = 0;//表明此项写操作完成，进行下一项，直至outstanding为0
		wakeup(&log);
	}
}

//从缓冲区复制数据块至日志
static void write_log(void)
{
	int tail;

	for (tail = 0; tail < log.lh.n; tail++) {
		struct buf *to = bread(log.dev, log.start + tail + 1); // 日志块
		struct buf *from = bread(log.dev, log.lh.block[tail]); // 缓冲区
		memmove(to->data, from->data, BSIZE);
		bwrite(to);  //写日志
		brelse(from);
		brelse(to);
	}
}

//提交日志
static void commit()
{
	if (log.lh.n > 0) {
		write_log();     // 将修改后的块从缓冲区写入日志
		write_head();    // 更新初始块
		install_trans(); // 现在正式开始更新磁盘数据
		log.lh.n = 0;
		write_head();    //清空日志初始块，表明此次读写操作成功
	}
}

//记录日志操作
void log_write(struct buf *b)
{
	int i;

	if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1)
		panic("too big a transaction");
	if (log.outstanding < 1)
		panic("log_write outside of trans");

	for (i = 0; i < log.lh.n; i++) {
		if (log.lh.block[i] == b->blockno)   // 记录写入磁盘中哪一块
			break;
	}
	log.lh.block[i] = b->blockno;
	if (i == log.lh.n)
		log.lh.n++;
	b->flags |= B_DIRTY; // 预防措施
}
