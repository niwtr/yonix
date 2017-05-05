struct buf;
struct context;
struct file;
struct inode;
struct pipe;
struct proc;
struct rtcdate;
struct sleeplock;
struct stat;
struct superblock;
struct page_entry;
struct trapframe;
typedef int sem;
// bio.c
void            binit(void);
struct buf*     bread(uint, uint);
void            brelse(struct buf*);
void            bwrite(struct buf*);

// console.c
void            consoleinit(void);
void            cprintf(char*, ...);
void            consoleintr(int(*)(void));
void            panic(char*) __attribute__((noreturn));
void            prtwelcome();
// exec.c
int             exec(char*, char**);

// file.c
struct file*    filealloc(void);
void            fileclose(struct file*);
struct file*    filedup(struct file*);
void            fileinit(void);
int             fileread(struct file*, char*, int n);
int             filestat(struct file*, struct stat*);
int             filewrite(struct file*, char*, int n);

// fs.c
void            readsb(int dev, struct superblock *sb);
int             dirlink(struct inode*, char*, uint);
struct inode*   dirlookup(struct inode*, char*, uint*);
struct inode*   ialloc(uint, short);
struct inode*   idup(struct inode*);
void            iinit(int dev);
void            ilock(struct inode*);
void            iput(struct inode*);
void            iunlock(struct inode*);
void            iunlockput(struct inode*);
void            iupdate(struct inode*);
int             namecmp(const char*, const char*);
struct inode*   namei(char*);
struct inode*   nameiparent(char*, char*);
int             readi(struct inode*, char*, uint, uint);
void            stati(struct inode*, struct stat*);
int             writei(struct inode*, char*, uint, uint);

// ide.c
void            ideinit(void);
void            ideintr(void);
void            iderw(struct buf*);
int             read_swap(uint, void *);
int             write_swap(uint, const void *);


// kalloc.c
char*           kalloc(void);
void            kfree(char*);
void            kinit1(void*, void*);
void            kinit2(void*, void*);
void            slabinit();
void            free_slab(char*);
char*           alloc_slab(void);

// kbd.c
void            kbdintr(void);



// log.c
void            initlog(int dev);
void            log_write(struct buf*);
void            begin_op();
void            end_op();



// picirq.c
void            picenable(int);
void            picinit(void);

// pipe.c
int             pipealloc(struct file**, struct file**);
void            pipeclose(struct pipe*, int);
int             piperead(struct pipe*, char*, int);
int             pipewrite(struct pipe*, char*, int);

//PAGEBREAK: 16
// proc.c

void            esinit(void);
void            rdinit(void);
void            exit(void);
int             fork(void);
int             lwp_create(void *, void *, void *, int);
int             lwp_join(void **);
void            dynamic_sstore(void * , struct trapframe *, int);
void            dynamic_restart(void * , struct trapframe *, int);

extern int __debug;
int             procgrow(int);

int             kill(int);

void            dbg_procdump(void);
void            dbg_lstprocs(void);
void            dbg_lstrdy(void);
void            dbg_lstslp(void);
void            switch_to(struct proc *);
void            select_scheme(int);
void            sched_name(char *);
void            recalc_timeslice(void);
void            scheduler(void) __attribute__((noreturn));






void            sleep(void*);
void            userinit(void);
int             wait(void);
void            wakeup(void*);
void            giveup_cpu(void);
void            timeslice_yield(void);
void            transform(void);



// swtch.S
void            swtch(struct context**, struct context*);

// procutils.c
void            getcallerpcs(void*, uint*);


// sleeplock.c
void            acquiresleep(struct sleeplock*);
void            releasesleep(struct sleeplock*);
int             holdingsleep(struct sleeplock*);
void            initsleeplock(struct sleeplock*, char*);

// string.c
int             memcmp(const void*, const void*, uint);
void*           memmove(void*, const void*, uint);
void*           memset(void*, int, uint);
char*           safestrcpy(char*, const char*, int);
int             strlen(const char*);
int             strncmp(const char*, const char*, uint);
char*           strncpy(char*, const char*, int);

// syscall.c
int             sysc_argint(int, int*);
int             sysc_argptr(int, char**, int);
int             sysc_argstr(int, char**);
int             _sysc_getint(uint, int*);
int             _sysc_getstr(uint, char**);
void            syscall(void);

// timer.c
void            timerinit(void);


// trap.c
void            idtinit(void);
extern uint     ticks;

void            trapvecinit(void);

// uart.c
void            uartinit(void);
void            uartintr(void);
void            uartputc(int);

// vm.c
void            seginit(void);
void            kvmalloc(void);
pde_t*          setupkvm(void);
char*           uva2ka(pde_t*, char*);
int             allocuvm(pde_t*, uint, uint);
int             deallocuvm(pde_t*, uint, uint, uint);
void            freeuvm(pde_t*, uint);
void            inituvm(pde_t*, char*, uint);
int             loaduvm(pde_t*, char*, struct inode*, uint, uint);
pde_t*          copyuvm(pde_t*, uint, uint);
void            switchuvm(struct proc*);
void            switchkvm(void);
int             copyout(pde_t*, uint, void*, uint);
void            clearpteu(pde_t *pgdir, char *uva);

void            read_ref();
void            write_ref();
uint            find_slot(uint);
uint            alloc_slot(uint);
void            free_slot(uint);
void            free_page(uint, uint);
void            free_page_slot(uint);
void            add_page(uint *, char*, uint);
struct page_entry *sel_page();
int             pgflt_handle(uint);
void            swapinit(void);
void            page_out(void);
void            page_in(uint);


//atomic.c
int             atomic_add(int a, int b, int * c);
int             atomic_sub(int a, int b, int * c);
int             atomic_multi(int a, int b, int * c);
int             atomic_divide(int a, int b, int * c);
int             atomic_mod(int a, int b, int * c);
int             atomic_set(int * a,int b);
int             atomic_swap(int * a,int *b);

//semaphore.c
int             sem_init(int num,sem * semaphore);
int             sem_P(int step,sem * semaphore);
int             sem_V(int step,sem * semaphore);
int             mutex_init(sem * semaphore);
int             mutex_P(sem * semaphore);
int             mutex_V(sem * semaphore);

// number of elements in fixed-size array
#define NELEM(x) (sizeof(x)/sizeof((x)[0]))
