#include "robust.h"

FILE* Fopen(char* fname, char* mode){
  FILE* fp = fopen(fname, mode);
  if (fp == NULL){
    printf("Error opening file %s in mode %s\n", fname, mode);
    perror("File open error!\n");
  }
  return fp;
}

size_t Fread(void* buf, size_t size, size_t num_ele, FILE* fp){
  size_t bytesRead = fread(buf, size, num_ele, fp);
  if (bytesRead != num_ele){
    perror("Error reading from file!:");
  }
  return bytesRead;
}

void Fwrite(void* buf, size_t size, size_t num_ele, FILE* fp){
  size_t bytesWritten = fwrite(buf, size, num_ele, fp);
  if (bytesWritten != num_ele){
    perror("Error writing to file!\n");
  }
}

void* Malloc(size_t size){
  void* p = (void*) malloc(size);
  if (p == NULL){
    perror("Malloc failed: ");
  }
  return p;
}

void* Calloc(size_t num_ele, size_t ele_size){
  void* p = (void*) calloc(num_ele, ele_size);
  if (p == NULL){
    perror("Calloc failed: ");
  }
  return p;
}

void* Realloc(void* ptr, size_t size){
  if (size == 0){
    printf("Invalid input to Realloc!\n");
    return NULL;
  }
  void* p = (void*) realloc(ptr, size);
  if (p == NULL){
    perror("Calloc failed: ");
  }
  return p;
}

long Ftell(FILE* fp){
  int val = ftell(fp);
  if (val == -1){
    perror("Ftell failed: ");
  }
  return val;
}

void Fseek(FILE* fp, long offset, int mode){
  int status = fseek(fp, offset, mode);
  if (status != 0){
    perror("Fseek failed: ");
  }
}

void Remove(const char* path){
  int status = remove(path);
  if (status != 0){
    perror("Remove failed: ");
  }
}

void Rename(const char *oldpath, const char *newpath){
  int status = rename(oldpath, newpath);
  if (status != 0){
    perror("Rename failed: ");
  }
}