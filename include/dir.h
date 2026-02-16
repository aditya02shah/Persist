#ifndef DIR_H
#define DIR_H
#include <stdbool.h>

/*------------------------------------------------------------------------------------------------*/
/* checks whether file exists in name dir */
bool does_file_exist(char* name);

/* checks whether dir exists in cwd */
bool does_dir_exist(char* name);

/* create dir */
bool create_dir(char* name);

/* returns the size of file */
unsigned long get_filesize(char* fname);

/*------------------------------------------------------------------------------------------------*/

#endif