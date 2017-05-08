#include "types.h"
#include "stat.h"
#include "user.h"
int main(int argc, char ** argv){
  if(argc<=1)exit();
  else{
    int n=atoi(argv[1]);

    if(sched(n) < 0) //failure.
    {
      printf(1,"Operation refused by yonix.\n");
      exit();
    }

    char name[100];
    sched_name(name);
    printf(1, "Set sched method: %s\n",name);

  }
  exit();
}
