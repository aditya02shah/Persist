#include "hashmap.h"
#include "bitcask.h"
#include "robust.h"
#include <string.h>
#include <stdint.h>
#include <stddef.h>

/* helper functionality */
uint32_t fnv1a_hash(const uint8_t *key, size_t len) {
  // ref - http://isthe.com/chongo/tech/comp/fnv/

  uint32_t hash = 2166136261u;
  for (size_t i = 0; i < len; i++) {
      hash ^= key[i];
      hash *= 16777619u;
  }
  return hash;
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

bool keys_are_equal(obj* key1, obj* key2){
  // checks whether two obj keys are equal
  if (key1->num_bytes != key2->num_bytes){
    return false;
  }

  // perform a byte-wise comparison
  int i = 0;
  int num_bytes = key1->num_bytes;
  byte* iter_1 = key1->data;
  byte* iter_2 = key2->data;

  while (i < num_bytes){
    if (*iter_1 != *iter_2){
      return false;
    }
    iter_1++;
    iter_2++;
    i++;
  }

  return true;
}
/*------------------------------------------------------------------------------------------------*/

/* primary functionality */
hashmap* create_hashmap(int capacity, float threshold){
  // create a hashmap of size capacity (entries)
  hashmap* h = Calloc(1, sizeof(hashmap));
  h->capacity = capacity;
  h->cursize = 0;
  h->threshold = threshold;
  // allocate space to store entries
  h->map = (hashmap_entry*)Calloc(capacity, sizeof(hashmap_entry));
  return h;
}

void add_entry(hashmap* h, keydir_entry* new_entry, obj* key){
  // compute hash from key
  uint32_t hash = fnv1a_hash(key->data, key->num_bytes);

  // determine position
  int start = hash % h->capacity;
  int idx = start;

  // linear probing
  while ((h->map[idx].used) && (!keys_are_equal(key, &(h->map[idx].key)))){
    // if entry already used and key is not the same, look for the next unused one
    idx = (idx + 1) % h->capacity;
    if (idx == start) {
        printf("Hash table full!\n");
        return;
    }
  }
  
  // update entry
  hashmap_entry* dst = &(h->map[idx]);
  dst->key.num_bytes = key->num_bytes;
  
  // copy key data since it is overwritten in next iteration of user input
  dst->key.data = Calloc(key->num_bytes, sizeof(byte));
  memcpy(dst->key.data, key->data, sizeof(byte) * key->num_bytes);
  memcpy(&(dst->entry), new_entry, sizeof(keydir_entry));

  dst->used = true;
  h->cursize++;
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
