#include "types.h"
#include "stat.h"
#include "user.h"

#define N 10 //总共需fork的数目
#define TIMES 32767


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
  exit();
}

void schedtest(void){
  int n, pid;
  printf(1, "Forking children...\n");
  for(n=0;n<N;n++){
    pid=fork();
    if(pid<0)
      printf(1, "No room for forking.\n");
    if(pid==0){
      //子进程
      calcula();
    }
  }
  printf(1, "Waiting...\n");
  for(;n>0;n--){
    if(wait()<0){
      printf(1, "Wait stopped early, dead children may be found\n");
      exit();
    }
  }
  if(wait()!=-1){
    printf(1, "I got someone that is not my child.\n");
    exit();
  }
  printf(1, "All tests done!\n");
}



int main(void)
{
  schedtest();
  exit();

}
