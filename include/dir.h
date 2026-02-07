#ifndef DIR_H
#define DIR_H
#include <stdbool.h>

bool does_file_exist(char* name);
bool does_dir_exist(char* name);
bool create_dir(char* name);
unsigned long get_filesize(char* file);

#endif