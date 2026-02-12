#ifndef ROBUST_H
#define ROBUST_H
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

/*------------------------------------------------------------------------------------------------*/
/* same as the system calls, but with baked in error handling */
FILE* Fopen(char* fname, char* mode);
void Fread(void* buf, size_t size, size_t num_ele, FILE* fp);
void Fwrite(void* buf, size_t size, size_t num_ele, FILE* fp);
void* Malloc(size_t size);
void* Calloc(size_t num_ele, size_t ele_size);

/*------------------------------------------------------------------------------------------------*/

#endif