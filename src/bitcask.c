#include "bitcask.h"
#include "input_handling.h"
#include "dir.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

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

void handle_open_request(char* dir){
  bool dir_exists = does_dir_exist(dir);

  if (!dir_exists){
    create_dir(dir);
  }
}

void setup_dir(char* dir, int* p_file_idx){
  // add .metadata file if not present
  // setup fp to point to latest file (or create one if not exist) in append mode
  // if file_idx == -1, update it
  char fname[FILE_NAME_LIMIT];
  FILE* f = NULL;
  FILE* fp = NULL;
  
  // If .metadata file exists, read current file_idx from it
  sprintf(fname, "%s/%s", dir, ".metadata");
  if (does_file_exist(fname)){
    f = Fopen(fname, "r");
    if (!f){
      return;
    }
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

void handle_put_request(char* dir, int* p_file_idx, obj* o){
  // setup .metadata file
  setup_dir(char* dir, int* p_file_idx);

  // get current file size
  int obj_size = o->num_bytes;
  char fname[FILE_NAME_LIMIT];
  sprintf(fname, "%s/file_%d", dir, *p_file_idx);
  unsigned long fsize = get_filesize(fname);
  fsize += obj_size;

  FILE* fp = NULL;
  // check file size
  if (fsize > FILE_SIZE){
    // cannot write object to current file
    (*p_file_idx)++; // increment file_idx

    // update .metadata file with new file_idx
    sprintf(fname, "%s/%s", dir, ".metadata");
    fp = Fopen(fname, "w");
    Fwrite(p_file_idx, sizeof(int), 1, fp);
    fclose(p);

    sprintf(fname, "%s/file_%d", dir, *p_file_idx); // reset fname
  }

  // open file
  fp = Fopen(fname, "ab");

  // write data to file
  Fwrite(o->num_bytes, sizeof(byte), 1, fp);
  Fwrite(o->data, sizeof(byte), o->num_bytes, fp);

  // close open
  fclose(fp);
}

int main(){
  // handle user input
  size_t size = BUF_SIZE;
  char* line = malloc(size * sizeof(char));
  ssize_t nread;
  char dir[BUF_SIZE];
  bool dir_opened = false;
  
  // handle file R/W
  int file_idx = -1;
  FILE* fp = NULL; // current file handler

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
          // convert request into bytes
          obj newobj;
          newobj.num_bytes = bytes_read;
          newobj.data = (byte*)buf; // buf is overwritten in next iteration, but we write to file
          handle_put_request(dir, &file_idx);
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
