#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM_THREADS 7
#define TIMES 50000
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
  printf(1, "Child %d Calcula finish!\n", getpid());//TODO 添加一个系统调用，获得pid。
}

void slp(void){
  sleep(800);
  printf(1, "Child %d Sleep finish!\n", getpid());
}
void thread_tsk_c(void *arg){
  printf(1, "active thread %d started.\n", *(int*)arg);
  incnice(*(int*)arg);
  calcula();
  printf(1, "active thread %d finished.\n", *(int *)arg);
  exit();
}

void thread_tsk_s(void * arg){
  printf(1, "lazy thread %d started.\n", *(int*)arg);
  calcula();
  printf(1, "lazy thread %d finished.\n", *(int *)arg);
  exit();
}
int times = 0;
void thread_tsk_z(void * arg){
  printf(1, "setsched thread %d started.\n", *(int*)arg);
  sleep(200);
  printf(1, "Set scheme to PRIORITY\n");
  sched(2);
  sleep(200);
  printf(1, "Set scheme to RR\n");
  sched(1);
  sleep(200);
  printf(1, "Set scheme to FIFO\n");
  sched(0);
  printf(1, "setsched thread %d finished.\n", *(int *)arg);
  exit();
}




int main(void){
  toggle_debug();
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
    void (*p) (void*);
    if(i < 3) p=thread_tsk_c;
    else if(i>=3 && i<6)
      p=thread_tsk_s;
    else p=thread_tsk_z;
		int pid = lwp_create(p, args[i], stacks[i], BASIC_THREAD_STACK_SZ);
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
  toggle_debug();
  sched(1);
  exit();
}
