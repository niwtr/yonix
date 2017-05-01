#include "types.h"
#include "stat.h"
#include "user.h"
#include "x86.h"
#define SZ 8192

void * stack_0, *stack_lev1, *stack_lev2, *stack_thmain;
int * arg;
dstate dynamic_state_lev1,dynamic_state_lev2, dynamic_state_thmain;


int kk=0;
int returned = 0;
void level3(void)
{
  printf(1,"I am in level 3.\n");
  if(kk < 20)
  {
    if(kk % 2)
    {
      printf(1,"Going to level 1.\n");
      dsrestart(stack_lev1, &dynamic_state_lev1, SZ);
    }
    else{
      printf(1,"Going to level 2.\n");
      dsrestart(stack_lev2, &dynamic_state_lev2, SZ);
    }
  }
  else
  {
    returned = 1;
    printf(1,"Going to thread main.\n");
    dsrestart(stack_thmain, &dynamic_state_thmain, SZ);
  }
}
void level2(void)
{
  dsstore(stack_lev2, &dynamic_state_lev2, SZ);
  printf(1,"I am in level 2.\n");
  kk++;
  level3();
}


void level1(void)
{
  dsstore(stack_lev1, &dynamic_state_lev1, SZ);
  printf(1,"I am in level 1.\n");
  kk++;
  level2();
}


void thread1(void * arg){
  printf(1, "Thread %d created.\n", *(int*)arg);
  dsstore(stack_thmain, &dynamic_state_thmain, SZ);
  if(returned) //returned from level3.
  {
    printf(1,"Returned directly from level3. \n");
    exit();
  }
  else
  {
    printf(1,"Going high!\n");
    level1();
  }
}


int main(void){


  stack_0=(void*)malloc(SZ);
  stack_lev1=(void*)malloc(SZ);
  stack_lev2=(void*)malloc(SZ);
  stack_thmain=(void*)malloc(SZ);
  arg=(int*)malloc(sizeof(int));
  if(!stack_0 || !stack_lev1 || !stack_lev2|| !stack_thmain || !arg){
    printf(1,"could not get enough room!\n");
    exit();
  }
  *arg=1;
  lwp_create(thread1, arg, stack_0, SZ);
  printf(1, "I let's rock!\n");
  void * joinstack;
  lwp_join(&joinstack);
  free(joinstack);
  printf(1,"joined.\n");
  free(stack_lev1);
  free(stack_thmain);
  exit();
}
