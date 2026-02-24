#ifndef HASHMAP_H
#define HASHMAP_H
#include "persist.h"
#include <stdbool.h>
#include <time.h>

/*------------------------------------------------------------------------------------------------*/
// representation of keydir (an in-memory hashmap) entry
typedef struct{
    int file_id;
    int value_size;
    long value_pos;
    int64_t timestamp;
}keydir_entry;

typedef enum{
    empty = 0,
    used = 1,
    deleted = 2
}slot_status;

// representation of an entry in the hashmap - contains metadata, key and keydir_entry
typedef struct{
    slot_status status;
    obj key;
    keydir_entry entry;
}hashmap_entry;

// representation of the hashmap
typedef struct{
    int capacity;
    int cursize;
    float threshold;
    hashmap_entry* map; // points to the base of the map
}hashmap;



/*------------------------------------------------------------------------------------------------*/

// NOTE: 
// We use the linear probing collision handling strategy, as opposed to a linked list
// this is done in favour of better cache locality

/*------------------------------------------------------------------------------------------------*/

/* creates a hashmap of size capacity, which doubles itself once size reaches threshold */
hashmap* create_hashmap(int capacity, float threshold);

/* displays the hashmap entry*/
void display_entry(keydir_entry* entry);

/* displays all used entries in the hashmap */
void display_hashmap(hashmap* h);

/* add entry to hashmap; creates a copy of key*/
void add_entry(hashmap* h, keydir_entry* entry, obj* key);

/* returns keydir entry if key is present in hashmap, NULL otherwise */
keydir_entry* get_entry(hashmap* h, obj* key);

/* deletes entry from hashmap. returns false if entry doesn't exist, true otherwise */
bool delete_entry(hashmap* h, obj* key);

/* free all dynamically allocated memory used by the hashmap */
void free_hashmap(hashmap* h);

/*------------------------------------------------------------------------------------------------*/
#endif