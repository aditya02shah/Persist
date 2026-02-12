#ifndef BITCASK_H

#include <stdint.h>
#define BITCASK_H
#define FILE_NAME_LIMIT 40
#define BUF_SIZE 50
#define FILE_SIZE (1024) // 1 KB
typedef uint8_t byte;

/*------------------------------------------------------------------------------------------------*/
// representation of obj - a length-prefixed byte array
struct s_obj{
  int num_bytes;
  byte* data;
//   byte data[];
};
typedef struct s_obj obj;

// representation of file entry
struct s_file_entry{
    int timestamp;
    int key_size;
    int value_size;
    obj key;
    obj value;
};
typedef struct s_file_entry file_entry;

/*------------------------------------------------------------------------------------------------*/

/* displays the obj */
void display_obj(obj* o);
/*------------------------------------------------------------------------------------------------*/

#endif