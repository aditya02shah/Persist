#include "hashmap.h"
#include "bitcask.h"
#include "robust.h"
#include <string.h>
#include <stdint.h>
#include <stddef.h>

uint32_t fnv1a_hash(const uint8_t *key, size_t len) {
    // http://isthe.com/chongo/tech/comp/fnv/

    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < len; i++) {
        hash ^= key[i];
        hash *= 16777619u;
    }
    return hash;
}

// if entry is already present, update it
// else create new slot for it
hashmap* create_hashmap(int capacity, float threshold){
  // create a hashmap of size capacity (entries)
  hashmap* h = Calloc(1, sizeof(hashmap));
  h->capacity = capacity;
  h->cursize = 0;
  h->threshold = threshold;
  h->map = (hashmap_entry*)Calloc(capacity, sizeof(hashmap_entry));
  return h;
}

void add_entry(hashmap* h, keydir_entry* new_entry, obj* key){
  uint32_t hash = fnv1a_hash(key->data, key->num_bytes);
  int start = hash % h->capacity;
  // printf("Hash capacity = %d\n", h->capacity);
  // printf("Hash = %u\tPos = %d\n", hash, start);

  // calloc sets used to 0
  // realloc after crossing threshold

  int idx = start;
  // linear probing
  while (h->map[idx].used){
    idx = (idx + 1) % h->capacity;
    if (idx == start) {
        printf("Hash table full!\n");
        return;
    }
  }

  hashmap_entry* dst = &(h->map[idx]);
  dst->key.num_bytes = key->num_bytes;
  // copy key data since it is overwritten in next iteration of user input
  // printf("Copying input!\n");
  dst->key.data = Calloc(key->num_bytes, sizeof(byte));
  memcpy(dst->key.data, key->data, sizeof(byte) * key->num_bytes);
  // printf("Finished copying input!\n");
  // printf("Displaying object!\n");
  // printf("Object before: \n");
  // display_obj(key);
  // printf("Object after: \n");
  // display_obj(&(dst->key));

  memcpy(&(dst->entry), new_entry, sizeof(keydir_entry));
  // dst->entry = new_entry;
  dst->used = true;
  h->cursize++;
}

void display_hashmap(hashmap* h){
  printf("Hashmap Utilization: %d / %d\n", h->cursize, h->capacity);
  for (int i = 0; i < h->capacity; i++){
    // if entry used
    if (h->map[i].used){
      printf("Key: ");
      display_obj(&(h->map[i].key));
      printf(
        "FileID: %d\tValue Size:%d\tValue pos:%lu\tTimestamp:%s\n", 
        h->map[i].entry.file_id,
        h->map[i].entry.value_size,
        h->map[i].entry.value_pos,
        ctime(&(h->map[i].entry.timestamp))
      );
    }
  }
}

void free_hashmap(hashmap* h){
  // visit every node and free its key.data
  for (int i = 0; i < h->capacity; i++){
    // if entry used
    if (h->map[i].used){
      free(h->map[i].key.data);
    }
  }
  free(h->map);
  free(h);
}