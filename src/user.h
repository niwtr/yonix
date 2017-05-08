struct stat;
struct rtcdate;
#include "param_yonix.h"
typedef struct trapframe dstate;
typedef int sem;

// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(char*, int);
int mknod(char*, short, short);
int unlink(char*);
int fstat(int fd, struct stat*);
int link(char*, char*);
int mkdir(char*);
int chdir(char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
int incnice(int);
int decnice(int);
int sched(int);
int lwp_create(void*, void*, void*, int);
int lwp_join(void **);
int sched_name(char *);
int dsstore(void *, dstate * , int);
int dsrestart(void *, dstate * , int);
//atomic and semaphores
int atom_add(int,int,int*);
int atom_sub(int,int,int*);
int atom_mul(int,int,int*);
int atom_div(int,int,int*);
int atom_mod(int,int,int*);
int atom_set(void*,void*);
int atom_swp(void*,void*);
int sem_init(int, sem *);
int sem_p(int, sem * );
int sem_v(int,sem * );
int mut_init(sem *);
int mut_p(sem *);
int mut_v(sem *);
int toggle_debug(void);



enum sched_method {
  SCHED_FIFO,
  SCHED_RR,
  SCHED_PRI,
};



// ulib.c
int stat(char*, struct stat*);
char* strcpy(char*, char*);
void *memmove(void*, void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void printf(int, char*, ...);
char* gets(char*, int max);
uint strlen(char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);
