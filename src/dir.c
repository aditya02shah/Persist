#include "dir.h"
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

bool does_dir_exit(char* name){
  struct stat sb;
  if (stat(name, &sb) == 0 && S_ISDIR(sb.st_mode)){
    return true;
  }
  return false;
}

bool create_dir(char* name){
  bool status = 0;
  // create directory if it doesn't exist
  status = mkdir(name, 0700);
  
  if (status != 0){
    perror("mkdir failed");
  }

  return status;
}
