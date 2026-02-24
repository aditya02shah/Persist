#include "robust.h"

FILE* Fopen(char* fname, char* mode){
  FILE* fp = fopen(fname, mode);
  if (fp == NULL){
    printf("Error opening file %s in mode %s\n", fname, mode);
    exit(EXIT_FAILURE);
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
    exit(EXIT_FAILURE);
  }
  return p;
}

void* Calloc(size_t num_ele, size_t ele_size){
  void* p = (void*) calloc(num_ele, ele_size);
  if (p == NULL){
    perror("Calloc failed: ");
    exit(EXIT_FAILURE);
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
    perror("Realloc failed: ");
    exit(EXIT_FAILURE);
  }
  return p;
}

long Ftell(FILE* fp){
  long val = ftell(fp);
  if (val == -1){
    perror("Ftell failed: ");
    exit(EXIT_FAILURE);
  }
  return val;
}

void Fseek(FILE* fp, long offset, int mode){
  int status = fseek(fp, offset, mode);
  if (status != 0){
    perror("Fseek failed: ");
    exit(EXIT_FAILURE);
  }
}

void Remove(const char* path){
  int status = remove(path);
  if (status != 0){
    perror("Remove failed: ");
    exit(EXIT_FAILURE);
  }
}

void Rename(const char *oldpath, const char *newpath){
  int status = rename(oldpath, newpath);
  if (status != 0){
    perror("Rename failed: ");
    exit(EXIT_FAILURE);
  }
}