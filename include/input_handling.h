#ifndef INP_HANDLING_H
#define INP_HANDLING_H
#include <stdbool.h>

/*------------------------------------------------------------------------------------------------*/
/* replaces all occurances of char1 in the buffer with char2*/
void replace_char1_with_char2(char* buf, char char1, char char2);

/* this function assumes that command substring already exists in the line
 * extracts the string following the command substring, into dst
 */
bool get_str_following_command(char* line, char* command, char* dst);

/* checks whether command is followed by only whitespace and newline characters */
bool is_not_empty_command(char* line, char* command);

/*------------------------------------------------------------------------------------------------*/

#endif