#include "robust.h"

FILE* Fopen(char* fname, char* mode){
  FILE* fp = fopen(fname, mode);
  if (fp == NULL){
    printf("Error opening file %s in mode %s\n", fname, mode);
  }
  return fp;
}

void Fread(void* buf, size_t size, size_t num_ele, FILE* fp){
  size_t bytesRead = fread(buf, size, num_ele, fp);
  if (bytesRead != num_ele){
    printf("Error reading from file!\n");
  }
}

void Fwrite(void* buf, size_t size, size_t num_ele, FILE* fp){
  size_t bytesWritten = fwrite(buf, size, num_ele, fp);
  if (bytesWritten != num_ele){
    printf("Error writing to file!\n");
  }
}

void* Malloc(size_t size){
  void* p = (void*) malloc(size);
  if (p == NULL){
    printf("Malloc failed: ");
  }
  return p;
}

void* Calloc(size_t num_ele, size_t ele_size){
  void* p = (void*) calloc(num_ele, ele_size);
  if (p == NULL){
    printf("Calloc failed: ");
  }
  return p;
}