// init: The initial user-level program

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

char *argv[] = { "sh", 0 };

int
main(void)
{
  int pid, wpid;

  if(open("console", O_RDWR) < 0){
    mknod("console", 1, 1);
    open("console", O_RDWR);
  }
  dup(0);  // stdout
  dup(0);  // stderr

  for(;;){
    printf(1, "YONIX: starting shell...\n");
    pid = fork();

    if(pid < 0){
      printf(1, "init: fork failed\n");
      exit();
    }
    if(pid == 0){//子进程
      exec("sh", argv);
      printf(1, "init: exec sh failed\n");
      exit();
    }
    //printf(1,"init: forked:%d\n", pid);
    printf(1,"YONIX: shell ready on PID %d\n", pid);

    while((wpid=wait()) >= 0 && wpid != pid){
      printf(1, "YONIX: I caught a zombie! PID: %d\n", wpid);
    }
  }
}
