#ifndef PERSIST_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#define PERSIST_H
#define FILE_NAME_LIMIT 40
#define BUF_SIZE 50
#define FILE_SIZE (1024) // 1 KB
typedef uint8_t byte;

/*------------------------------------------------------------------------------------------------*/
// representation of obj - a length-prefixed byte array
struct s_obj{
  int num_bytes;
  byte* data; // dynamically allocate if necessary
};
typedef struct s_obj obj;

// representation of file entry
struct s_file_entry{
    time_t timestamp;
    int key_size;
    int value_size;
    byte* key_data;
    byte* value_data;
};
typedef struct s_file_entry file_entry;

// helps read data from files
struct s_fread_helper{
  time_t timestamp;
  int key_size;
  int value_size;
};
typedef struct s_fread_helper fread_helper;

/*------------------------------------------------------------------------------------------------*/

/* displays an object as <prefix> <obj length> <obj data> <suffix> */
/* prefix, suffix can be set to NULL, and display_length to false */
void display_obj(char* prefix, obj* o, char* suffix, bool display_length);
/*------------------------------------------------------------------------------------------------*/

#endif