#ifndef ROBUST_H
#define ROBUST_H
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

/*------------------------------------------------------------------------------------------------*/
/* same as the system calls, but with baked in error handling */
FILE* Fopen(char* fname, char* mode);
size_t Fread(void* buf, size_t size, size_t num_ele, FILE* fp);
void Fwrite(void* buf, size_t size, size_t num_ele, FILE* fp);
void* Malloc(size_t size);
void* Calloc(size_t num_ele, size_t ele_size);
void* Realloc(void* ptr, size_t size);
long Ftell(FILE* fp);
void Fseek(FILE* fp, long offset, int mode);
void Remove(const char* path);
void Rename(const char *oldpath, const char *newpath);
/*------------------------------------------------------------------------------------------------*/

#endif