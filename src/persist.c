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
  printf("Timestamp: %lld\tKey Size:%d\tValue Size:%d\n", f->timestamp, f->key_size, f->value_size);
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

// helps get high resolution time
static inline int64_t
now_ns(clockid_t clock_id)
{
    struct timespec ts;
    clock_gettime(clock_id, &ts);
    return (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;
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
  snprintf(fname, FILE_NAME_LIMIT, "%s/%s", dir, ".metadata");
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

void read_entries_from_file(char* fname, char* hfname, hashmap* h){

  int fileid = get_fileid_from_name(fname);
  bool read_from_hintfile = false;

  if (fileid == -1){
    // invalid file
    return;
  }

  // check whether hint file exists
  if (does_file_exist(hfname)){
    // read entries from hint file, because its faster
    read_from_hintfile = true;
  }

  // check whether file exists
  if (!read_from_hintfile && !does_file_exist(fname)){
    return;
  }

  // open file
  FILE* fp = NULL;
  if (read_from_hintfile){
    // open hint file
    fp = Fopen(hfname, "rb");
  }
  else{
    // open merge file
    fp = Fopen(fname, "rb");
  }
 
  // allocate dynamic memory, to handle variable-length keys
  int bufsize = sizeof(byte) * 100;
  char* buf = Malloc(bufsize);

  fread_helper fbuf;
  size_t bytes_read;

  // in hint file, entry is stored as:
  // <int64_t timestamp> <int key_sz> <int val_sz> <long value_pos> <byte(varlen) key>

  // in merge file, entry is stored as:
  // <int64_t timestamp> <int key_sz> <int val_sz> <byte(varlen) key > <byte(varlen) val>

  // read <timestamp><key_sz><val_sz> 
  while ((bytes_read = fread(&fbuf, 1,  sizeof(fbuf), fp)) > 0){
      // read key (varlen)
      int req_size = fbuf.key_size * sizeof(byte);
      if (req_size > bufsize){
        // obtain enough memory to store key
        buf = Realloc((void*)buf, req_size);
        bufsize = req_size;
      }

      long pos = -1;
      // read pos, if we are reading from hintfile
      if (read_from_hintfile){
        if ((bytes_read = fread(&pos, sizeof(long), 1, fp)) <= 0){
          free(buf);
          return;
        }
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
        if (!read_from_hintfile){
          entry.value_pos = ftell(fp);
        }
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
            if (!read_from_hintfile){
              fseek(fp, entry.value_size, SEEK_CUR);
            }
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
      char fname[FILE_NAME_LIMIT];  // merge file
      char hfname[FILE_NAME_LIMIT]; // hint file
      snprintf(fname, FILE_NAME_LIMIT, "%s/%s", dir, entry->d_name);
      // check whether file has a valid name -> "file_id"
      int fid = get_fileid_from_name(fname);
      if (fid != -1){
        // file is valid. process file
        snprintf(hfname, FILE_NAME_LIMIT, "%s/%s_%d", dir, "hint", fid);
        read_entries_from_file(fname, hfname, h);
      }
  }
  closedir(d);
}

int create_entry(char* line, size_t bytes_read, file_entry* f){
  // create a file_entry, and fill it with information from the line buf
  // returns -1 if input is in invalid format, size of entry otherwise

  // f->timestamp = time(NULL);
  f->timestamp = now_ns(CLOCK_MONOTONIC);

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
  snprintf(fname, FILE_NAME_LIMIT, "%s/file_%d", dir, *p_curfile_idx);
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
    snprintf(fname, FILE_NAME_LIMIT, "%s/%s", dir, ".metadata");
    fp = Fopen(fname, "wb");
    Fwrite(p_curfile_idx, sizeof(int), 1, fp);
    fclose(fp);

    snprintf(fname, FILE_NAME_LIMIT, "%s/file_%d", dir, *p_curfile_idx); // reset fname
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
    Fwrite((void*)&(f->timestamp), sizeof(int64_t), 1, fp);
    Fwrite((void*)&(f->key_size), sizeof(int), 1, fp);
    Fwrite((void*)&(f->value_size), sizeof(int), 1, fp);
    Fwrite((void*)(f->key_data), sizeof(byte), f->key_size, fp);
  }
  else{
    Fwrite((void*)&(f->timestamp), sizeof(int64_t), 1, fp);
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

  // check whether file exists
  char fname[FILE_NAME_LIMIT];
  snprintf(fname, FILE_NAME_LIMIT, "%s/file_%d", dir, file_id);

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
  if (!(fread((void*)(value->data), sizeof(byte), value_size, fp) > 0)){
    perror("Read failed: ");
  }
  // close file
  fclose(fp);
 
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
    display_obj("deleting key which doesnt exist: ", &key, "\n", true);
    return;
  }

  // write tombstone to file
  // a tombstone is represented as an object of size 0, with its data ptr set to NULL
  file_entry f;
  f.timestamp = now_ns(CLOCK_MONOTONIC);
  f.key_size = key.num_bytes;
  f.value_size = 0;
  f.key_data = key.data;
  f.value_data = NULL;

  int entry_sz = sizeof(int) * 3;
  write_to_file(&f, entry_sz, dir, p_curfile_idx);
}

void handle_merge_request(char* line, char* dir, hashmap* h){
  // the merge operation discards the unused entries in a file
  // it also creates a "hint file" corresponding to each "merge file"
  // this allows for a quicker startup time, as keydir is built more quickly

  // input validation
  char* iter = line;
  while (*iter == ' '){
    iter++;
  }

  if (*iter != '\0' && *iter != '\n'){
    printf("Invalid input. Example use: merge\n");
    return;
  }

  // determine active file in dir
  char fname[FILE_NAME_LIMIT];
  snprintf(fname, FILE_NAME_LIMIT, "%s/%s", dir, ".metadata");
  if (!does_file_exist(fname)){
    // freshly opened dir. no data to merge
    return;
  }
  int active_file_idx = -1;
  FILE* f = Fopen(fname, "rb");
  Fread(&active_file_idx, sizeof(int), 1, f);

  // get list of inactive files in in descending order
  struct dirent **namelist;
  int n;

  n = scandir(dir, &namelist, NULL, alphasort);
  if (n == -1) {
      perror("scandir");
      exit(EXIT_FAILURE);
  }

  // store fileids for the inactive files to be merged, in descending order
  int merge_fid[MAX_FILE_MERGE];
  int idx = -1;
  
  while (n--) {
      snprintf(fname, FILE_NAME_LIMIT, "%s/%s", dir, namelist[n]->d_name);
      int file_id = get_fileid_from_name(fname);

      if (file_id != -1 && file_id != active_file_idx){
        idx++;
        merge_fid[idx] = file_id;
      }
      free(namelist[n]);
  }
  free(namelist);

  // perform the merge operation
  char rf_name[FILE_NAME_LIMIT]; // file to read from
  char wf_name[FILE_NAME_LIMIT]; // merge file
  char hf_name[FILE_NAME_LIMIT]; // hint file

  int merge_idx = 0; // track the file we are writing to (aka "merge file")
  int hint_idx = 0;  // track the "hint file" for corresponding "merge file" 
  size_t bytes_written = 0;

  // open merge file
  snprintf(fname, FILE_NAME_LIMIT, "merge_%d", merge_idx);
  snprintf(wf_name,FILE_NAME_LIMIT, "%s/%s", dir, fname);
  FILE* fw = Fopen(wf_name, "wb");

  // open hint file
  snprintf(fname, FILE_NAME_LIMIT, "hint_%d", hint_idx);
  snprintf(hf_name, FILE_NAME_LIMIT, "%s/%s", dir, fname);
  FILE* fh = Fopen(hf_name, "wb");

  FILE* fr = NULL;
  
  // allocate dynamic memory, to handle variable-length keys & values
  int kbuf_size = sizeof(byte) * 100;
  char* kbuf = Malloc(kbuf_size);
  int vbuf_size = sizeof(byte) * 100;
  char* vbuf = Malloc(vbuf_size);
  
  fread_helper fbuf;
  size_t bytes_read;

  // iterate through list of inactive files
  // for each file, only keep the entries which are actively in-use
  for (int i = 0; i <= idx; i++){
    int file_id = merge_fid[i];   // current file being processed
    snprintf(fname, FILE_NAME_LIMIT, "file_%d", file_id);
    snprintf(rf_name, FILE_NAME_LIMIT, "%s/%s", dir, fname);
 
    // read record from fr
    fr = Fopen(rf_name, "rb");

    // iterate through all entries in the current file
    while ((bytes_read = fread(&fbuf, 1,  sizeof(fbuf), fr)) > 0){
      // read key (varlen)
      int req_size = fbuf.key_size * sizeof(byte);
      if (req_size > kbuf_size){
        // obtain enough memory to store key
        kbuf = Realloc((void*)kbuf, req_size);
        kbuf_size = req_size;
      }

      obj key;
      // read key
      if ((bytes_read = fread(kbuf, sizeof(byte), fbuf.key_size, fr)) > 0){
        key.num_bytes = fbuf.key_size;
        key.data = (byte*)kbuf;

        // compare entry with hashmap entry to check whether it is uptodate
        keydir_entry* curentry = get_entry(h, &key);
        if (
          fbuf.value_size != 0 && 
          curentry && 
          curentry->timestamp == fbuf.timestamp && 
          file_id == curentry->file_id
        )
        {   
          // this is the latest entry. retain it
          req_size = fbuf.value_size * sizeof(byte);
          if (req_size > vbuf_size){
          // obtain enough memory to store value
            vbuf = Realloc((void*)vbuf, req_size);
            vbuf_size = req_size;
          }

          // read value
          obj value;
          if ((bytes_read = fread(vbuf, sizeof(byte), fbuf.value_size, fr)) > 0){
            value.num_bytes = fbuf.value_size;
            value.data = (byte*)vbuf;
      
            // write record to merge file
            Fwrite((void*)&(fbuf.timestamp), sizeof(int64_t), 1, fw);
            Fwrite((void*)&(fbuf.key_size), sizeof(int), 1, fw);
            Fwrite((void*)&(fbuf.value_size), sizeof(int), 1, fw);
            Fwrite((void*)(key.data), sizeof(byte), fbuf.key_size, fw);
            long pos = Ftell(fw); // store offset
            Fwrite((void*)(value.data), sizeof(byte), fbuf.value_size, fw);

            // write record to hint file
            Fwrite((void*)&(fbuf.timestamp), sizeof(int64_t), 1, fh);
            Fwrite((void*)&(fbuf.key_size), sizeof(int), 1, fh);
            Fwrite((void*)&(fbuf.value_size), sizeof(int), 1, fh);
            Fwrite((void*)&pos, sizeof(long), 1, fh);
            Fwrite((void*)(key.data), sizeof(byte), fbuf.key_size, fh);

            // update the keydir entry, since the entry was copied to another file
            keydir_entry entry;
            entry.timestamp = fbuf.timestamp;
            entry.value_size = fbuf.value_size;
            entry.value_pos = pos;     // new pos of entry
            entry.file_id = merge_idx; // new file entry resides in
            add_entry(h, &entry, &key);

            // create new write file if we have exceeded file size
            bytes_written += sizeof(fbuf) + fbuf.key_size + fbuf.value_size;
            if ((unsigned long)bytes_written >= FILE_SIZE){
              // close write file and open new one
              fclose(fw);
              merge_idx++;
              bytes_written = 0; // reset counter
              snprintf(fname, FILE_NAME_LIMIT, "merge_%d", merge_idx);
              snprintf(wf_name, FILE_NAME_LIMIT, "%s/%s", dir, fname);
              fw = Fopen(wf_name, "wb");

              // close hint file and open new one
              fclose(fh);
              hint_idx++;
              snprintf(fname, FILE_NAME_LIMIT, "hint_%d", hint_idx);
              snprintf(hf_name, FILE_NAME_LIMIT, "%s/%s", dir, fname);
              fh = Fopen(hf_name, "wb");
            };
          }
        }  
      }
      else{
        // skip to next entry
        fseek(fr, fbuf.value_size, SEEK_CUR);
      }
    }
    // close the file we just processed
    fclose(fr);
  }

  // release used resources
  fclose(fw);
  fclose(fh);
  free(kbuf);
  free(vbuf);

  // rename merge files to "file_fileno" format. this overwrites the files we just copied over
  char oldname[FILE_NAME_LIMIT];
  char newname[FILE_NAME_LIMIT];
  for (int i = 0; i <= merge_idx; i++){
    snprintf(fname, FILE_NAME_LIMIT, "%s_%d", "merge", i);
    snprintf(oldname, FILE_NAME_LIMIT, "%s/%s", dir, fname);

    snprintf(fname, FILE_NAME_LIMIT, "%s_%d", "file", i);
    snprintf(newname, FILE_NAME_LIMIT, "%s/%s", dir, fname);
    Rename(oldname, newname);
  }
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
        // display_hashmap(h);
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

    // merge files in dir
    if (strstr(line, "merge")){
      if (!dir_opened){
        printf("No directory selected!\n");
      }
      else{
        if (is_not_empty_command(line, "merge")){
          char* occurrence = ((char*)strstr(line, "merge")) + strlen("merge");
          handle_merge_request(occurrence, dir, h);
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
