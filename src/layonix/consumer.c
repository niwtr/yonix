    #include "types.h"
#include "stat.h"
#include "user.h"
#include "param_yonix.h"
#include "semaphore.h"

//多线程的生产者消费者问题


#define BUFFERSIZE 8
#define CONSUMERNUM 4
#define PRODUCERNUM 4
#define BASIC_THREAD_STACK_SZ 8192

//semaphore
sem empty;
sem full;
//mutex
sem mutex;
//buffer queue
int buffer[BUFFERSIZE];
int head,tail;

void consumerFun(void *param);
void producerFun(void *param);

int main()
{
    int i,j;



    //initiate mutex
    mut_init(&mutex);

    //initiate semaphore
    sem_init(BUFFERSIZE,&empty);
    sem_init(0,&full);

    printf(1,"emp: %p, ful: %p, mut: %p", &empty, &full, &mutex);

    // Stacks
    void *pro_stacks[PRODUCERNUM],*cum_stacks[CONSUMERNUM];
    // Args
    int *pro_args[PRODUCERNUM],*cum_args[CONSUMERNUM];

    // Allocate stacks and args and make sure we have them all
    // Bail if something fails
    for (i=0; i<PRODUCERNUM; i++) {
        pro_stacks[i] = (void*) malloc(BASIC_THREAD_STACK_SZ);
        if (!pro_stacks[i]) {
            printf(1, "main: could not get stack for thread %d, exiting...\n");
            exit();
        }

        pro_args[i] = (int*) malloc(sizeof(int));
        if (!pro_args[i]) {
            printf(1, "main: could not get memory (for arg) for thread %d, exiting...\n");
            exit();
        }

        *pro_args[i] = i;
    }
    //create productor thread
    for (i=0; i<PRODUCERNUM; i++) {
        int pid = lwp_create(producerFun, pro_args[i], pro_stacks[i], BASIC_THREAD_STACK_SZ);
        printf(1, "main: created producer thread with pid %d\n", pid);
    }

    printf(1,"Producer threads created.\n");


    // Allocate stacks and args and make sure we have them all
    // Bail if something fails
    for (j=0; j<CONSUMERNUM; j++) {
        cum_stacks[j] = (void*) malloc(BASIC_THREAD_STACK_SZ);
        if (!cum_stacks[j]) {
            printf(1, "main: could not get stack for thread %d, exiting...\n");
            exit();
        }

        cum_args[j] = (int*) malloc(sizeof(int));
        if (!cum_args[j]) {
            printf(1, "main: could not get memory (for arg) for thread %d, exiting...\n");
            exit();
        }

        *pro_args[j] = j;
    }
    //create consumer thread
    for (j=0; j<CONSUMERNUM; j++) {
        int pid = lwp_create(consumerFun, cum_args[j], cum_stacks[j], BASIC_THREAD_STACK_SZ);
        printf(1, "main: created consumer thread with pid %d\n", pid);
    }

    printf(1,"Consumer threads created.\n");


    // Wait for all children
    //productor
    for (i=0; i<PRODUCERNUM; i++) {
        void *joinstack;
        lwp_join(&joinstack);
        for (j=0; j<PRODUCERNUM; j++) {
            if (joinstack == pro_stacks[i]) {
                printf(1, "main: producer thread %d joined...\n", i);
                free(joinstack);
                break;
            }
        }
    }
    //consumer
    for (i=0; i<CONSUMERNUM; i++) {
        void *joinstack;
        lwp_join(&joinstack);
        for (j=0; j<CONSUMERNUM; j++) {
            if (joinstack == cum_stacks[i]) {
                printf(1, "main: consumer thread %d joined...\n", i);
                free(joinstack);
                break;
            }
        }
    }

    exit();
}

void producerFun(void *param)
{
    int tid;
    tid=getpid();
    for(int i=0;i<10;i++)
    {
        sem_p(2,&empty);    //wait empty
        mut_p(&mutex);      //wait mutex

        //add item to buffer

        buffer[tail]=1;     //produce
        tail=(tail+1)%BUFFERSIZE;
        buffer[tail]=1;
        tail=(tail+1)%BUFFERSIZE;
        printf(1,"tid:%d,producer----buffer status:\n",tid);
        //current buffer state
        for(int i=0;i<BUFFERSIZE;i++)
        {
            printf(1,"%d  ",buffer[i]);
        }
        printf(1,"\n");

        mut_v(&mutex);      //signal mutex
        sem_v(2,&full);     //signal full
        printf(1,"full %d,empty %d\n",full,empty);
        sleep(20);
    }
    exit();
}

void consumerFun(void *param)
{
    int tid;
    tid=getpid();
    for(int i=0;i<20;i++)
    {

        sem_p(1,&full);     //wait full
        mut_p(&mutex);      //wait mutex

        //remove an item from buffer
        buffer[head]=0;     //consume
        head=(head+1)%BUFFERSIZE;
        //show current state
        printf(1,"tid:%d,consumer----buffer status:\n",tid);
        for(int i=0;i<BUFFERSIZE;i++)
        {
            printf(1,"%d  ",buffer[i]);
        }
        printf(1,"\n");

        mut_v(&mutex);      //signal mutex
        sem_v(1,&empty);    //signal empty
        printf(1,"full %d,empty %d\n",full,empty);
        sleep(20);
    }
    printf(1, "I exit.\n");
    exit();
}
