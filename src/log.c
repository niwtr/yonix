#include 'buf.h'
#include 'fs.h' // fs headfiles

#include 'types.h' 
#include 'defs.h'  
#include 'param.h' // basic headfile


struct loghd{          //log header
	int n;
	int block[logsize];
};

struct log {
	int start;   //start location
	int size;	 //block size
	int outstanding;	
	int committing;	//in commintting
	int devno;     //device's number
	struct logh bgn; 
};

struct log log;

void initlog(int n);     //initial
void readHeadToLog(void);	//get fileOp's details
void logRecord(struct buf *n); //record which block needed to  be changed
void writeLog(void);    //write log
void writeHead(void);
void fileOpCommit(void);	//commmit fileOp
void recover_from_log(void);  
void beginOp(void);   //lock
void endOp(void);    //lock
void updateFile(void);    //do fileOp

void initlog(int devno){
	if (sizeof(struct loghd) >= BSIZE) //exception handling
		panic("initlog:too big logheader");
	struct superblock sub; //TOD0,need teammates' struct 'superlock',needed to store block's inode and size
	log.start = sub.start;
	log.size = sub.size;
	log.devno = devno;
	recover_from_log();    //TOD1
}

void readHeadToLog(void){
	struct buf *buf = bread(log.devno,log.start);     //TOD2,get fileOp's device num and adress
	struct loghd *bgn = (struct loghd *)(buf->data);
	int m;
	log.bgn.n = bgn->n;
	for (m = 0;m < log.bgn.n;m++){
		log.bgn.block[m] = bgn -> block[m];
	}
}

void logRecord(struct buf *b){
	int m;
	if(log.bgn.n >= LOGSIZE || log.bgn.m >= log.size -1)
		panic("too big a transaction");
	if(log.activty < 1)
		panic("logWrite outside of trans");

	for (m = 0; m < log.bgn.n; m++) {
    	if (log.bgn.block[m] == b->blockno)   // log absorbtion
      		break;
  	}
  	log.bgn.block[m] = b->blockno;
  	if (m == log.bgn.n)
    log.bgn.m++;
  	b->flags |= B_DIRTY; // prevent eviction
}

void writeLog(void){
	int tail;

  	for (tail = 0; tail < log.bgn.n; tail++) {
    	struct buf *to = bread(log.devno, log.start+tail+1); // log block
    	struct buf *from = bread(log.devno, log.bgn.block[tail]); // cache block
    	memmove(to->data, from->data, BSIZE);  //TOD3
    	bwrite(to);  // write the log
    	brelse(from);
    	brelse(to);
	}
}

void writeHead(void){
  	struct buf *buf = bread(log.dev, log.start); //TOD4
  	struct loghd *hb = (struct loghd *) (buf->data);
  	int m;
  	hb->n = log.bgn.n;
  	for (m = 0; m < log.bgn.n; m++) {
    	hb->block[m] = log.bgn.block[m];
  	}
  	bwrite(buf);
  	brelse(buf);
}

void fileOpCommit(void){
	if (log.bgn.n > 0) {
    	writeLog();     // Write modified blocks from cache to log
    	writeHead();    // Write header to disk -- the real commit
    	fileOpCommit(); // Now install writes to home locations
    	log.bgn.n = 0;
    	writeHead();    // Erase the transaction from the log
  	}
}

void recover_from_log(void){
  	readHeadToLog();
  	fileOpCommit(); // if committed, copy from log to disk
  	log.bgn.n = 0;
  	writeHead(); // clear the log
}

void begin_op(void){
  	while(1){
    	if(log.committing){
      		sleep(&log);
    	} 
    	else if(log.bgn.n + (log.activty+1)*MAXOPBLOCKS > LOGSIZE){ // this op might exhaust log space; wait for commit.
      		sleep(&log);
    	} 
    	else {
      		log.activty += 1;
      		break;
    	}
  	}
}

void end_op(void){
  	int do_commit = 0;
	log.outstanding -= 1;

  	if(log.committing)
    	panic("log.committing");
  	if(log.outstanding == 0){
    	do_commit = 1;
    	log.committing = 1;
  	} 
  	else {	// begin_op() may be waiting for log space.
    	wakeup(&log);
  	}

    if(do_commit){ // call commit w/o holding locks, since not allowed
                   // to sleep with locks.
    	commit();
    	log.committing = 0;
    	wakeup(&log);
  	}
}

void install_trans(void){
  	int tail;

  	for (tail = 0; tail < log.bgn.n; tail++) {
    	struct buf *lbuf = bread(log.devno, log.start+tail+1); // read log block
    	struct buf *dbuf = bread(log.devno, log.bgn.block[tail]); // read dst
    	memmove(dbuf->data, lbuf->data, BSIZE);  // copy block to dst
    	bwrite(dbuf);  // write dst to disk
    	brelse(lbuf);
    	brelse(dbuf);
  }
}

