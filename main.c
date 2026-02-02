#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BUF_SIZE 50

int main(){
  size_t size = BUF_SIZE;
  char* line = malloc(size * sizeof(char));
  ssize_t nread;
  
  while ((nread = getline(&line, &size, stdin)) != -1){
    printf("Retrieved line of length %zd:\n", nread);
    fwrite(line, nread, 1, stdout);

    // exit cli
    if (strcmp(line, "exit\n") == 0){
      break;
    }

  }

  free(line);


  printf("Exiting db!\n");
  return 0;
}