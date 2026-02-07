#ifndef BITCASK_H
#include <stdint.h>
#define BITCASK_H
#define FILE_NAME_LIMIT 40
#define BUF_SIZE 50
#define FILE_SIZE 1024 // 1 GB
typedef uint8_t byte;

// representation of obj - a length-prefixed byte array
struct s_obj{
  int num_bytes;
  byte* data;
//   byte data[];
};
typedef struct s_obj obj;

// representation of keydir (an in memory hashmap) entry
struct s_keydir_entry{
    int file_id;
    int value_size;
    int value_pos;
    int timestamp;
};
typedef struct s_keydir_entry keydir_entry;

// representation of file entry
struct s_file_entry{
    byte timestamp[4];
    byte key_size[4];
    byte value_size[4];
    obj key;
    obj value;
};
typedef struct s_file_entry file_entry;

#endif