#ifndef HASHMAP_H
#define HASHMAP_H
#include "bitcask.h"
#include <stdbool.h>
#include <time.h>

/*------------------------------------------------------------------------------------------------*/
// representation of keydir (an in-memory hashmap) entry
struct s_keydir_entry{
    int file_id;
    int value_size;
    long value_pos;
    time_t timestamp;
};
typedef struct s_keydir_entry keydir_entry;

// representation of an entry in the hashmap - contains metadata, key and keydir_entry
struct s_hashmap_entry{
    bool used;
    obj key;
    keydir_entry entry;
};
typedef struct s_hashmap_entry hashmap_entry;

// representation of the hashmap
struct s_hashmap{
    int capacity;
    int cursize;
    float threshold;
    struct s_hashmap_entry* map; // points to the base of the map
};
typedef struct s_hashmap hashmap;

/*------------------------------------------------------------------------------------------------*/

// NOTE: 
// We use the linear probing collision handling strategy, as opposed to a linked list
// this is done in favour of better cache locality

/*------------------------------------------------------------------------------------------------*/

/* creates a hashmap of size capacity, which doubles itself once size reaches threshold */
hashmap* create_hashmap(int capacity, float threshold);

/* displays all used entries in the hashmap */
void display_hashmap(hashmap* h);

/* add entry to hashmap */
void add_entry(hashmap* h, keydir_entry* entry, obj* key);

/* free all dynamically allocated memory used by the hashmap */
void free_hashmap(hashmap* h);

/*------------------------------------------------------------------------------------------------*/
#endif