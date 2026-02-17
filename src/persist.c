#include "persist.h"
#include "hashmap.h"
#include "input_handling.h"
#include "dir.h"
#include "robust.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>

/* functionality to display things */
void display_obj(char* prefix, obj* o, char* suffix, bool display_length){

  // printf("Obj is %d bytes long: ", o->num_bytes);
  if (prefix){
      printf("%s ", prefix);
  }
  if (display_length){
    printf("%d ", o->num_bytes);
  }
  byte* temp = o->data;
  int i = 0;
  while (i < o->num_bytes){
    printf("%c", *temp);
    temp++;
    i++;
  }
 if (suffix){
   printf("%s", suffix);
 }
}

void display_file_entry(file_entry* f){
  // display a file entry
  printf("Timestamp: %ld\tKey Size:%d\tValue Size:%d\n", f->timestamp, f->key_size, f->value_size);
  printf("Key details: \n");
  obj key;
  key.num_bytes = f->key_size;
  key.data = f->key_data;
  display_obj(NULL, &key, NULL, false);

  printf("Value details: \n");
  obj value;
  value.num_bytes = f->value_size;
  value.data = f->value_data;
  display_obj(NULL, &value, NULL, false);
  printf("\n");
}
/*------------------------------------------------------------------------------------------------*/

/* functionality to setup things before processing */
void handle_open_request(char* dir){
  // create directory if it doesnt exist
  bool dir_exists = does_dir_exist(dir);
  if (!dir_exists){
    create_dir(dir);
  }
}

void setup_dir(char* dir, int* p_file_idx){
  // setups directory with .metadata file, and latest file index
  char fname[FILE_NAME_LIMIT];
  FILE* f = NULL;
  
  // if .metadata file exists, read current file_idx from it
  sprintf(fname, "%s/%s", dir, ".metadata");
  if (does_file_exist(fname)){
    f = Fopen(fname, "rb");
    Fread(p_file_idx, sizeof(int), 1, f);
  }
  else{
    // create .metadata file if it doesnt exist
    f = Fopen(fname, "wb");
    (*p_file_idx)++;
    Fwrite(p_file_idx, sizeof(int), 1, f);
  }
  // close .metadata file
  fclose(f);
}

int get_fileid_from_name(char* name){
  if (name == NULL || !does_file_exist(name)){
    return -1;
  }

  if (!(strstr(name, "file_"))){
    return -1;
  }

  // find underscore
  char* underscore = strchr(name, '_');
  if (underscore == NULL){
      return -1;
  }

  // Move past underscore
  char* id_start = underscore + 1;
  if (*id_start == '\0'){
      return -1;
  }
  
  char* iter = id_start;
  while (*iter != '\0'){
    if (!isdigit((unsigned char)*iter)) {
        return -1;
    }
    iter++;
  }

  return atoi(id_start);
}

void read_entries_from_file(char* fname, hashmap* h){
  // check whether file exists
  if (!does_file_exist(fname)){
    return;
  }

  // entry is stored as:
  // <time_t timestamp><int key_sz><int val_sz><byte(varlen) key ><byte(varlen) val>

  // open file
  FILE* fp = Fopen(fname, "rb");
  int fileid = get_fileid_from_name(fname);
  if (fileid == -1){
    return;
  }

  // allocate dynamic memory, to handle variable-length keys
  int bufsize = sizeof(byte) * 100;
  char* buf = Malloc(bufsize);

  fread_helper fbuf;
  size_t bytes_read;

  // read <timestamp><key_sz><val_sz> 
  while ((bytes_read = fread(&fbuf, 1,  sizeof(fbuf), fp)) > 0){
      // read key (varlen)
      int req_size = fbuf.key_size * sizeof(byte);
      if (req_size > bufsize){
        // obtain enough memory to store key
        buf = Realloc((void*)buf, req_size);
        bufsize = req_size;
      }

      obj key;
      // read key
      if ((bytes_read = fread(buf, sizeof(byte), fbuf.key_size, fp)) > 0){
        key.num_bytes = fbuf.key_size;
        key.data = (byte*)buf;

        // create keydir entry
        keydir_entry entry;
        entry.timestamp = fbuf.timestamp;
        entry.value_size = fbuf.value_size;
        entry.value_pos = ftell(fp);
        entry.file_id = fileid;

        // check whether key already exists in keydir
        keydir_entry* curentry = get_entry(h, &key);
        if (entry.value_size == 0){
          // entry is a tombstone
          // delete entry if current records timestamp is more recent than keydir's
          if (curentry && curentry->timestamp < entry.timestamp){
            delete_entry(h, &key);
          }
        }
        else{
          // if key doesn't exist in keydir, add it
          // if key exists in keydir, but its timestamp is outdated, update it
          if (curentry == NULL || (curentry->timestamp < entry.timestamp)){
            // add key to keydir;
            add_entry(h, &entry, &key);
          }
            // skip to next entry
            fseek(fp, entry.value_size, SEEK_CUR);
        }
      }
  }

  // close file
  fclose(fp);
  // free heap memory
  free(buf);
}

void build_keydir_from_dir(char* dir, hashmap* h){
  // check if dir exists
  if (!does_dir_exist(dir)){
    printf("Dir %s doesn't exist. Can't build keydir!\n", dir);
    return;
  }
  
  // scan over files in dir
  struct dirent *entry;
  DIR *d = opendir(dir);
  if (d == NULL) {
      return;
  }

  // iterate over files in dir
  while ((entry = readdir(d)) != NULL) {
      char fname[FILE_NAME_LIMIT];
      sprintf(fname, "%s/%s", dir, entry->d_name);
      // check whether file has a valid name -> "file_id"
      int fid = get_fileid_from_name(fname);
      if (fid != -1){
        // file is valid. process file
        read_entries_from_file(fname, h);
      }
  }
  closedir(d);
}

int create_entry(char* line, size_t bytes_read, file_entry* f){
  // create a file_entry, and fill it with information from the line buf
  // returns -1 if input is in invalid format, size of entry otherwise

  f->timestamp = time(NULL);

  int entry_sz = 0; // in bytes
  size_t i = 0; // helps with input validation
  char* iter = line;
  char* key_offset = NULL;
  char* value_offset = NULL;

  // skip leading whitespace(s)
  while (*iter == ' '){
    if (i == bytes_read){
      // end of input - terminate
      return -1;
    }
    iter++;
    i++;
  }

  // we are at key
  int key_sz = 0;
  key_offset = iter;
  while (*iter != ' '){
    if (i == bytes_read){
      // end of input - terminate
      return -1;
    }
    iter++;
    key_sz++;
    i++;
  }

  // we are at the space(s) separating key and value
   while (*iter == ' '){
    if (i == bytes_read){
      // end of input - terminate
      return -1;
    }
    iter++;
    i++;
  }

  // we are at value
  int value_sz = 0;
  value_offset = iter;

  while (*iter != '\n'){
    if (i == bytes_read){
      // end of input - break
      break;
    }
    iter++;
    value_sz++;
    i++;
  }

  // set file_entry fields
  f->key_size = key_sz;
  f->value_size = value_sz;
  f->key_data = (byte*)key_offset;
  f->value_data = (byte*)value_offset;

  entry_sz = (sizeof(int) * 3) + (f->key_size + f->value_size);
  // display_file_entry(f);

  return entry_sz;
}

long write_to_file(file_entry* f,int entry_sz, char* dir, int* p_curfile_idx){
  // writes entry to disk

  // get current file size
  char fname[FILE_NAME_LIMIT];
  sprintf(fname, "%s/file_%d", dir, *p_curfile_idx);
  unsigned long fsize = 0;

  if (does_file_exist(fname)){
    // get file size if the file exists
    fsize = get_filesize(fname);
  }
  fsize += entry_sz;

  FILE* fp = NULL;
  // check file size
  if (fsize > (unsigned long)FILE_SIZE){
    // cannot write object to current file
    (*p_curfile_idx)++; // increment file_idx

    // update .metadata file with new file_idx
    sprintf(fname, "%s/%s", dir, ".metadata");
    fp = Fopen(fname, "wb");
    Fwrite(p_curfile_idx, sizeof(int), 1, fp);
    fclose(fp);

    sprintf(fname, "%s/file_%d", dir, *p_curfile_idx); // reset fname
  }

  // open file
  fp = Fopen(fname, "ab");

  bool is_tombstone = false;
  if (f->value_size == 0){
    is_tombstone = true;
  }

  long pos = 0; // needed to add entry to keydir

  // write data to file
  if (is_tombstone){
    Fwrite((void*)&(f->timestamp), sizeof(time_t), 1, fp);
    Fwrite((void*)&(f->key_size), sizeof(int), 1, fp);
    Fwrite((void*)&(f->value_size), sizeof(int), 1, fp);
    Fwrite((void*)(f->key_data), sizeof(byte), f->key_size, fp);
  }
  else{
    Fwrite((void*)&(f->timestamp), sizeof(time_t), 1, fp);
    Fwrite((void*)&(f->key_size), sizeof(int), 1, fp);
    Fwrite((void*)&(f->value_size), sizeof(int), 1, fp);
    Fwrite((void*)(f->key_data), sizeof(byte), f->key_size, fp);
    pos = Ftell(fp); // store offset (stored in keydir for fast reads)
    Fwrite((void*)(f->value_data), sizeof(byte), f->value_size, fp);
  }
 
  // close file
  fclose(fp);

  return pos;
}

/*------------------------------------------------------------------------------------------------*/

/* functionality to handle requests */
void handle_put_request(char* line, size_t bytes_read, char* dir, int* p_curfile_idx, hashmap* h){

  // create file entry from input
  file_entry f;
  int entry_sz = create_entry(line, bytes_read, &f);
  if (entry_sz == -1){
    printf("Invalid input: Syntax = put <key> <value>\n");
    printf("Example: put name xyz\n");
    return;
  }

  // write entry to file
  long pos = write_to_file(&f, entry_sz, dir, p_curfile_idx);

  // create entry for hash table
  keydir_entry entry;
  entry.file_id = *p_curfile_idx;
  entry.value_size = f.value_size;
  entry.value_pos = pos;
  entry.timestamp = f.timestamp;

  // add entry to keydir
  obj key;
  key.num_bytes = f.key_size;
  key.data = f.key_data;
  // hashmap makes a copy of key
  add_entry(h, &entry, &key);
  // display_hashmap(h);

}

bool handle_get_request(char* line, size_t bytes_read, char* dir, hashmap* h, obj* value){
  // parse input
  char* iter = line;
  char* key_offset = NULL;

  // skip whitespaces
  while (*iter == ' '){
    iter++;
  }

  // we are at key
  int key_sz = 0;
  key_offset = iter;
  while (*iter != '\n'){
    iter++;
    key_sz++;
  }

  // store key
  obj key;
  key.data = (byte*)key_offset;
  key.num_bytes = key_sz;

  keydir_entry* entry = get_entry(h, &key);
  if (!entry){
    printf("Entry doesn't exist!\n");
    return false;
  }

  // fetch information from entry
  int file_id = entry->file_id;
  int value_size = entry->value_size;
  long value_pos = entry->value_pos;
  // time_t timestamp = entry->timestamp;

  // check whether file exists
  char fname[FILE_NAME_LIMIT];
  sprintf(fname, "%s/file_%d", dir, file_id);

  if (!does_file_exist(fname)){
    printf("Invalid entry!\n");
    return false;
  }

  // open file
  FILE* fp = Fopen(fname, "rb");

  // setup value obj
  value->num_bytes = value_size;
  value->data = Malloc(value_size * sizeof(byte));

  // read value
  Fseek(fp, value_pos, SEEK_SET);
  Fread((void*)(value->data), sizeof(byte), value_size, fp);

  return true;
}

void handle_delete_request(char* line, size_t bytes_read, char* dir, int* p_curfile_idx, hashmap* h){
  // parse input
  char* iter = line;
  char* key_offset = NULL;

  // skip whitespaces
  while (*iter == ' '){
    iter++;
  }

  // we are at key
  int key_sz = 0;
  key_offset = iter;
  while (*iter != '\n'){
    iter++;
    key_sz++;
  }

  // store key
  obj key;
  key.data = (byte*)key_offset;
  key.num_bytes = key_sz;
  bool entry_exists = delete_entry(h, &key);
  if (!entry_exists){
    printf("Entry doesn't exist!\n");
    return;
  }

  // write tombstone to file
  // a tombstone is represented as an object of size 0, with its data ptr set to NULL
  file_entry f;
  f.timestamp = time(NULL);
  f.key_size = key.num_bytes;
  f.value_size = 0;
  f.key_data = key.data;
  f.value_data = NULL;

  int entry_sz = sizeof(int) * 3;
  write_to_file(&f, entry_sz, dir, p_curfile_idx);
}
/*------------------------------------------------------------------------------------------------*/

/* interactive loop to get user input */
int main(){
  // handle user input
  size_t size = BUF_SIZE;
  char* line = malloc(size * sizeof(char));
  size_t nread;
  char dir[BUF_SIZE];
  bool dir_opened = false;
  
  // store data for cur dir
  int file_idx = -1;

  // instantiate the keydir(in-memory hashmap)
  int map_size = 100;
  float threshold = 0.8;
  hashmap* h = create_hashmap(map_size, threshold);

  // get user input 
  while ((nread = getline(&line, &size, stdin)) != -1){
    // open directory
    if (strstr(line, "open")){
      if (get_str_following_command(line, "open", dir)){
        handle_open_request(dir);
        dir_opened = true;
        file_idx = -1; // reset file_idx
        setup_dir(dir, &file_idx);
        // create in-memory hashmap from dir entries
        build_keydir_from_dir(dir, h);
      };
    }

    // put entry
    if (strstr(line, "put")){
      if (!dir_opened){
        printf("No directory selected!\n");
      }
      else{
        if (is_not_empty_command(line, "put")){
          char* occurrence = ((char*)strstr(line, "put")) + strlen("put");
          handle_put_request(occurrence, nread, dir, &file_idx, h);
        }
      }
    }

    // get entry
    if (strstr(line, "get")){
      if (!dir_opened){
        printf("No directory selected!\n");
      }
      else{
        if (is_not_empty_command(line, "get")){
          char* occurrence = ((char*)strstr(line, "get")) + strlen("get");
          obj value;
          if (handle_get_request(occurrence, nread, dir, h, &value)){
            // for debugging
            display_obj("--->", &value, "\n", false);
            free(value.data);
          };
          
        }
      }
    }

    // delete entry
    if (strstr(line, "delete")){
      if (!dir_opened){
        printf("No directory selected!\n");
      }
      else{
        if (is_not_empty_command(line, "delete")){
          char* occurrence = ((char*)strstr(line, "delete")) + strlen("delete");
          handle_delete_request(occurrence, nread, dir, &file_idx, h);
        }
      }
    }
  
    // exit cli
    if (strcmp(line, "exit\n") == 0){
      printf("Exiting db!\n");
      break;
    }
  }

  // free dynamically allocated memory
  free(line);
  free_hashmap(h);
  
  return 0;
}