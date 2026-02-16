#include "dir.h"
#include "robust.h"
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

unsigned long get_filesize(char* fname)
{
  // ref - https://stackoverflow.com/a/8247
    FILE * f = Fopen(fname, "rb");
    fseek(f, 0, SEEK_END);
    unsigned long len = (unsigned long)ftell(f);
    fclose(f);
    return len;
}

bool does_file_exist(char* name){
  FILE* fp = fopen(name, "r");
  if (fp != NULL){
    // file exists
    fclose(fp);
    return true;
  }
  return false;
}

bool does_dir_exist(char* name){
  struct stat sb;
  if (stat(name, &sb) == 0 && S_ISDIR(sb.st_mode)){
    // dir exists
    return true;
  }
  return false;
}

bool create_dir(char* name){
  bool status = 0;
  // create directory if it doesn't exist
  status = mkdir(name, 0700);
  
  if (status != 0){
    printf("mkdir failed");
  }

  return status;
}
