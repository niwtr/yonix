#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM_THREADS 10
#define TIMES 20000
#define BASIC_THREAD_STACK_SZ 8192
int kk=0;
void calcula(void){
  int i, j, k=0;
  for(i=0;i<TIMES;i++){
    for(j=0;j<TIMES;j++){
      if(j%2)k++;
      else k--; //doing some boring jobs.
    }
  }
  kk=k; //防止编译器编译优化。
}


void thread_tsk(void *arg){
  incnice(getpid()%2?5:-5);
  printf(1, "thread %d started.\n", *(int*)arg);
  calcula();
  printf(1, "thread %d finished.\n", *(int *)arg);
  exit();
}


int main(void){
  int i,j;
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
  printf(1, "hacks and glory awaits.\n");
  exit();
}
