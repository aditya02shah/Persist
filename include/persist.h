#ifndef PERSIST_H
#define PERSIST_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#define FILE_NAME_LIMIT 40
#define MAX_FILE_MERGE 100
#define BUF_SIZE 50
#define FILE_SIZE (1024) // 1 KB
typedef uint8_t byte;

/*------------------------------------------------------------------------------------------------*/
// representation of obj - a length-prefixed byte array
typedef struct{
  int num_bytes;
  byte* data; // dynamically allocate if necessary
}obj;

// representation of file entry
typedef struct{
    int64_t timestamp;
    int key_size;
    int value_size;
    byte* key_data;
    byte* value_data;
}file_entry;

// helps read data from files
typedef struct{
  int64_t timestamp;
  int key_size;
  int value_size;
}fread_helper;

/*------------------------------------------------------------------------------------------------*/

/* displays an object as <prefix> <obj length> <obj data> <suffix> */
/* prefix, suffix can be set to NULL, and display_length to false */
void display_obj(char* prefix, obj* o, char* suffix, bool display_length);
/*------------------------------------------------------------------------------------------------*/

#endif