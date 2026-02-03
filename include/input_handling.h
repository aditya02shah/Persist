#ifndef INP_HANDLING_H
#define INP_HANDLING_H
#include <stdbool.h>

/* Replaces all occurances of " " in the buffer with <replacement>*/
void replace_space_with_char(char* buf, char replacement);

/* This function assumes that command substring already exists in the line
 *
 * Extracts the string following the command substring, into dst
 */
bool get_str_following_command(char* line, char* command, char* dst);

#endif