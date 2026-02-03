#include "input_handling.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

void replace_space_with_char(char* buf, char replacement){
  char* iter = buf;
  // iterate through buffer
  while (*iter != '\0'){
    if (*iter == ' '){
      // replace " " with replacement
      *iter = replacement;
    }
    iter++;
  }
}

bool get_str_following_command(char* line, char* command, char* dst){
  int len_command = strlen(command);
  int len_line = strlen(line);
  char last_letter = command[len_command - 1];
  char* iter = line;

  // skip to first character following the command
  while (*(iter++) != last_letter); // skips the first whitespace after the command

  if (*iter == '\n'){
    // empty command
    return false;
  }

  while (*(iter++) != '\n'){
    // copy string following the "open " pattern
    *(dst++) = *iter;
  }

  *dst = '\0'; // null-terminate buffer
  
  return true;
}