#include "input_handling.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#define BUF_SIZE 50

int main(){
  // handle user input
  size_t size = BUF_SIZE;
  char* line = malloc(size * sizeof(char));
  ssize_t nread;

  char* substr = NULL;

  // get user input 
  while ((nread = getline(&line, &size, stdin)) != -1){
    char dir[BUF_SIZE]; // should be strdup'd to avoid bugs
    // printf("Retrieved line of length %zd:\n", nread);
    fwrite(line, nread, 1, stdout);

    // open directory
    if (strstr(line, "open")){
      if (get_str_following_command(line, "open", dir)){
        replace_space_with_char(dir, '_'); // sanitize directory name
        if (strlen(dir) > 0){
          printf("Extraced filename: %s\n", dir);
        }
         
      };
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
