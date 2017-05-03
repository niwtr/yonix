#include "types.h"
#include "defs.h"
#include "param.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

//ÿһ��ϵͳ���ö���Ҫ����begin_op()�����ʼ,end_op()��ǽ���.
//ͨ������£�begin_op()�����Ӷ�����ϵͳ���ø�����������������־�Ѿ���������ر�ʱ
//��˯�ߵ�ǰ����ֱ�������end_op()����.

//��־��ʼ��
struct logheader {
	int n;//��ǰ��Ч���ݿ�ļ���
	int block[LOGSIZE];//�����ֵΪ��Ӧ���ݿ������Ӧ��д���ļ�ϵͳ�е���һ��
};

//��־���ݿ�
struct log {
	int start;       //��ʼλ��
	int size;		 //����־��ռ��С
	int outstanding; // ���ڱ�ִ�е��ļ�ϵͳϵͳ���ø���
	int committing;  // ״̬���Ƿ��ύ
	int dev;
	struct logheader lh;
};
struct log log;

static void recover_from_log(void);//����־�лָ�
static void commit();//�ύ��־

//��ʼ��־��
void initlog(int dev){
	if (sizeof(struct logheader) >= BSIZE)
		panic("initlog: too big logheader");

	struct superblock sb;//������
	readsb(dev, &sb);
	log.start = sb.logstart;
	log.size = sb.nlog;
	log.dev = dev;
	recover_from_log();
}

// �����ݴ���־���ǵ���Ӧ���̿�
static void install_trans(void){
	int tail;
	for (tail = 0; tail < log.lh.n; tail++) {
		struct buf *lbuf = bread(log.dev, log.start + tail + 1); // ��ȡ��־
		struct buf *dbuf = bread(log.dev, log.lh.block[tail]); // ���м���
		memmove(dbuf->data, lbuf->data, BSIZE);  // ����־�������м���
		bwrite(dbuf);  // ����д�����
		brelse(lbuf);
		brelse(dbuf);
	}
}

// �Ӵ�����־����ȡ��־��ʼ�����ڴ�
static void read_head(void)
{
	struct buf *buf = bread(log.dev, log.start);
	struct logheader *lh = (struct logheader *) (buf->data);
	int i;
	log.lh.n = lh->n;
	for (i = 0; i < log.lh.n; i++) {
		log.lh.block[i] = lh->block[i];
	}
	brelse(buf);
}

//���ڴ���־��ʼ��д�����,�����ڵ�ǰ��־�����ύʱ
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

//����־�ָ��ļ�
static void recover_from_log(void)
{
	read_head();
	install_trans(); // ȷ�Ϻ󣬼�����������־�������
	log.lh.n = 0;
	write_head(); // ��ո���־��
}

// ��ÿһ��ϵͳ���ÿ�ʼʱ���øó���
void begin_op(void)
{
	while (1) {
		if (log.committing) {
			sleep(&log);
		}
		else if (log.lh.n + (log.outstanding + 1)*MAXOPBLOCKS > LOGSIZE) {
			// �˴β������ܻ�ľ���־�ռ�; �ȴ��ύ.
			sleep(&log);
		}
		else {
			log.outstanding += 1;
			break;
		}
	}
}

// ϵͳ������ֹʱ������
// ȷ����ε����Ƿ�Ϊ���һ�β���
void end_op(void)
{
	int do_commit = 0;

	log.outstanding -= 1;
	if (log.committing)
		panic("log.committing");
	if (log.outstanding == 0) {
		do_commit = 1;
		log.committing = 1;
	}
	else {
		// begin_op() ��ǰ�ڵȴ�������־�ռ�
		wakeup(&log);
	}

	if (do_commit) {
		// �ύ��־����
		commit();
		log.committing = 0;
		wakeup(&log);
	}
}

//�ӻ������������ݿ�����־
static void write_log(void)
{
	int tail;

	for (tail = 0; tail < log.lh.n; tail++) {
		struct buf *to = bread(log.dev, log.start + tail + 1); // ��־��
		struct buf *from = bread(log.dev, log.lh.block[tail]); // ������
		memmove(to->data, from->data, BSIZE);
		bwrite(to);  //д��־
		brelse(from);
		brelse(to);
	}
}

//�ύ��־
static void commit()
{
	if (log.lh.n > 0) {
		write_log();     // ���޸ĺ�Ŀ�ӻ�����д����־
		write_head();    // ���³�ʼ��
		install_trans(); // ������ʽ��ʼ���´�������
		log.lh.n = 0;
		write_head();    //����־�в����˴θ���
	}
}

//��¼��־����
void log_write(struct buf *b)
{
	int i;

	if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1)
		panic("too big a transaction");
	if (log.outstanding < 1)
		panic("log_write outside of trans");

	for (i = 0; i < log.lh.n; i++) {
		if (log.lh.block[i] == b->blockno)   // ��¼д���������һ��
			break;
	}
	log.lh.block[i] = b->blockno;
	if (i == log.lh.n)
		log.lh.n++;
	b->flags |= B_DIRTY; // Ԥ����ʩ
}
