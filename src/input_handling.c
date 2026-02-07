#include "input_handling.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

bool get_str_following_command(char* line, char* command, char* dst){
  char* iter = line;
  int len_line = strlen(line);
  int len_command = strlen(command);
  char last_letter = command[len_command - 1];
  int idx = 0;

  while (*iter != last_letter){
    iter++;
    idx++;
  }
  iter++; // move past command
  idx++;

  // now iter is at first letter beyond command

  bool has_non_ws = false; // is stream full of whitespaces and/or newlines

  // check whether line is just full of whitespaces and/or newlines
  char* temp = iter;

  int i = idx;
  while (i < len_line){
    char curchar = *temp;
    
    if (curchar == '\0'){
      // better to be safe than sorry
      break;
    }

    if (curchar != ' ' && curchar != '\n'){
      // found parse-able token. break.
      // printf("Curchar is '%c' (ASCII %d)\n", curchar, curchar);
      has_non_ws = true;
      break;
    }
    temp++;
    i++;
  }

  if (!has_non_ws){
    return false;
  }

  // line has parse-able information

  // skip whitespace
  while (*iter == ' '){
    iter++;
  }

  // process & copy input from line into dst
  while (idx < len_line){
    if (*iter == '\n'){
      // don't copy newline into dst. Terminate dst instead & break
      *dst = '\0';
      break;
    }
    if (*iter == ' '){
      // replace all instances of " " with " "
      *dst = '_';
    }
    else{
       *dst = *iter;
    }
    idx++;
    iter++;
    dst++;
  }

  return true;
}

bool is_not_empty_command(char* line, char* command){
  char* iter = line;
  int len_line = strlen(line);
  int len_command = strlen(command);
  char last_letter = command[len_command - 1];
  int idx = 0;
  bool has_non_ws = false;

  while (*iter != last_letter){
    iter++;
    idx++;
  }
  iter++; // move past command
  idx++;

  char* temp = iter;

  int i = idx;
  while (i < len_line){
    char curchar = *temp;
    
    if (curchar == '\0'){
      // better to be safe than sorry
      break;
    }

    if (curchar != ' ' && curchar != '\n'){
      // found parse-able token. break.
      has_non_ws = true;
      break;
    }
    temp++;
    i++;
  }

  return has_non_ws;
}
