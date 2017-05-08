#include "types.h"
#include "stat.h"
#include "user.h"
int main(int argc, char ** argv){
  if(toggle_debug()!=0)
    printf(1, "failed!\n");
  exit();
}
