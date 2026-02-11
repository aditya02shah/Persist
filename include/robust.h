#ifndef ROBUST_H
#define ROBUST_H
#include <stdio.h>
#include <stdlib.h>

/* Provide same functionality as unix calls, but with error handling */
FILE* Fopen(char* fname, char* mode);
void Fread(void* buf, size_t size, size_t num_ele, FILE* fp);
void Fwrite(void* buf, size_t size, size_t num_ele, FILE* fp);

#endif