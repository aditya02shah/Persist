#ifndef DIR_H
#define DIR_H
#include <stdbool.h>

/* Checks whether file exists in name dir */
bool does_file_exist(char* name);
/* Checks whether dir exists in cwd */
bool does_dir_exist(char* name);
/* Create dir */
bool create_dir(char* name);
/* Returns the size of file */
unsigned long get_filesize(char* file);

#endif