#include "types.h"
#include "stat.h"
#include "user.h"
#include "param_yonix.h"
#include "semaphore.h"
#define NUM_THREADS 40
#define TIMES 5000
#define BASIC_THREAD_STACK_SZ 8192
int kk=0;
sem sema;

void thread_tsk(void *arg){
  printf(1, "thread %d started.\n", *(int*)arg);

  int klocal;
  int i;
	sleep(100);
       
  for(i=0;i<TIMES;i++)
    {
      //sem_p(2,&sema);
		mut_p(&sema);
      klocal=kk;
      sleep(0);
      klocal++;
      sleep(0);
      kk=klocal;
      //sem_v(2,&sema);
		mut_v(&sema);
    }

  exit();
}


int main(void){
  int i,j;

   mut_init(&sema);//互斥
   //sem_init(2,&sema);
  


	    
  // Stacks
	void *stacks[NUM_THREADS];
	// Args
	int *args[NUM_THREADS];

	// Allocate stacks and args and make sure we have them all
	// Bail if something fails
	for (i=0; i<NUM_THREADS; i++) {
		stacks[i] = (void*) malloc(BASIC_THREAD_STACK_SZ);
		if (!stacks[i]) {
			printf(1, "main: could not get stack for thread %d, exiting...\n");
			exit();
		}

		args[i] = (int*) malloc(sizeof(int));
		if (!args[i]) {
			printf(1, "main: could not get memory (for arg) for thread %d, exiting...\n");
			exit();
		}

		*args[i] = i;
	}

	for (i=0; i<NUM_THREADS; i++) {
		int pid = lwp_create(thread_tsk, args[i], stacks[i], BASIC_THREAD_STACK_SZ);
		printf(1, "main: created thread with pid %d\n", pid);
	}


  printf(1,"Threads created.\n");
  // Wait for all children
	for (i=0; i<NUM_THREADS; i++) {
		void *joinstack;
		lwp_join(&joinstack);
		for (j=0; j<NUM_THREADS; j++) {
			if (joinstack == stacks[i]) {
				printf(1, "main: thread %d joined...\n", i);
        free(joinstack);
				break;
			}
		}
	}
	printf(1, "kk: %d\n", kk);
  exit();
}

