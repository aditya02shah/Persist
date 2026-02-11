#include "bitcask.h"
#include "input_handling.h"
#include "dir.h"
#include "robust.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

void display_obj(obj* o){
  printf("Obj is %d bytes long\n", o->num_bytes);
  byte* temp = o->data;
  int i = 0;
  while (i < o->num_bytes){
    printf("%c ", *temp);
    temp++;
    i++;
  }
  printf("\n");
}

void display_file_entry(file_entry* f){
  printf("Timestamp: %d\tKey Size:%d\tValue Size:%d\n", f->timestamp, f->key_size, f->value_size);
  printf("Key details: \n");
  display_obj(&(f->key));
  printf("Value details: \n");
  display_obj(&(f->value));
  printf("\n");
}

void handle_open_request(char* dir){
  bool dir_exists = does_dir_exist(dir);

  if (!dir_exists){
    create_dir(dir);
  }
}

int create_entry(char* line, ssize_t bytes_read, file_entry* f){
  f->timestamp = (int)time(NULL);

  int entry_sz = 0; // in bytes

  ssize_t i = 0;
  char* iter = line;
  char* key_offset = NULL;
  char* value_offset = NULL;

  // skip leading whitespace(s)
  while (*iter == ' '){
    if (i == bytes_read){
      return -1;
    }
    iter++;
  }

  // we are at key
  int key_sz = 0;
  key_offset = iter;
  while (*iter != ' '){
    if (i == bytes_read){
      return -1;
    }
    iter++;
    key_sz++;
  }

  // we are at the space(s) separating key and value
   while (*iter == ' '){
    if (i == bytes_read){
      return -1;
    }
    iter++;
  }

  // we are at value
  int value_sz = 0;
  value_offset = iter;

  while (*iter != '\n'){
    if (i == bytes_read){
      break;
    }
    iter++;
    value_sz++;
  }

  // set file_entry fields
  f->key_size = key_sz;
  f->value_size = value_sz;

  f->key.num_bytes = key_sz;
  f->key.data = (byte*)key_offset;

  f->value.num_bytes = value_sz;
  f->value.data = (byte*)value_offset;

  entry_sz = (sizeof(int) * 5) + (f->key_size + f->value_size);

  display_file_entry(f);
  return entry_sz;
}

void setup_dir(char* dir, int* p_file_idx){
  char fname[FILE_NAME_LIMIT];
  FILE* f = NULL;
  
  // if .metadata file exists, read current file_idx from it
  sprintf(fname, "%s/%s", dir, ".metadata");
  if (does_file_exist(fname)){
    f = Fopen(fname, "r");
    Fread(p_file_idx, sizeof(int), 1, f);
  }
  else{
    // create .metadata file if it doesnt exist
    f = Fopen(fname, "w");
    (*p_file_idx)++;
    Fwrite(p_file_idx, sizeof(int), 1, f);
  }
  // close .metadata file
  fclose(f);
}

void handle_put_request(char* line, ssize_t bytes_read, char* dir, int* p_curfile_idx){
  // extract key and value from input
  file_entry f;
  int entry_sz = create_entry(line, bytes_read, &f);
  if (entry_sz == -1){
    printf("scanning entry failed!\n");
  }
  printf("Created entry of size %d!\n", entry_sz);
  
  // setup .metadata file
  setup_dir(dir, p_curfile_idx);

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
  if (fsize > FILE_SIZE){
    // cannot write object to current file
    (*p_curfile_idx)++; // increment file_idx

    // update .metadata file with new file_idx
    sprintf(fname, "%s/%s", dir, ".metadata");
    fp = Fopen(fname, "w");
    Fwrite(p_curfile_idx, sizeof(int), 1, fp);
    fclose(fp);

    sprintf(fname, "%s/file_%d", dir, *p_curfile_idx); // reset fname
  }

  // open file
  fp = Fopen(fname, "ab");

  // write data to file
  // Fwrite((void*)&o->num_bytes, sizeof(byte), 1, fp);
  // Fwrite((void*)&(o->data), sizeof(byte), o->num_bytes, fp);
  Fwrite((void*)&(f.timestamp), sizeof(int), 1, fp);
  Fwrite((void*)&(f.key_size), sizeof(int), 1, fp);
  Fwrite((void*)&(f.value_size), sizeof(int), 1, fp);
  Fwrite((void*)&(f.key.num_bytes), sizeof(byte), 1, fp);
  Fwrite((void*)&(f.key.data), sizeof(byte), f.key.num_bytes, fp);
  Fwrite((void*)&(f.value.num_bytes), sizeof(byte), 1, fp);
  Fwrite((void*)&(f.value.data), sizeof(byte), f.value.num_bytes, fp);

  printf("Data written successfully\n");

  // close file
  fclose(fp);
}

int main(){
  // handle user input
  size_t size = BUF_SIZE;
  char* line = malloc(size * sizeof(char));
  ssize_t nread;
  char dir[BUF_SIZE];
  bool dir_opened = false;
  
  // store data for cur dir
  int file_idx = -1;

  // get user input 
  while ((nread = getline(&line, &size, stdin)) != -1){
    printf("Retrieved line of length %zd:\n", nread);
    // fwrite(line, nread, 1, stdout);

    // open directory
    if (strstr(line, "open")){
      if (get_str_following_command(line, "open", dir)){
        handle_open_request(dir);
        dir_opened = true;
        file_idx = -1; // reset file_idx
      };
    }

    // put directory
    if (strstr(line, "put")){
      if (!dir_opened){
        printf("No directory selected!\n");
      }
      else{
        if (is_not_empty_command(line, "put")){
          handle_put_request(line, nread, dir, &file_idx);
        }
      }
    }
  
    // exit cli
    if (strcmp(line, "exit\n") == 0){
      break;
    }
  }

  free(line);

  printf("Exiting db!\n");
  return 0;
}