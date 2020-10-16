#pragma once

#include <stdbool.h>
#include <stdint.h>

// lexer_init() takes file descriptor from which input is read and a timeout
// in milliseconds how long read call is allowed to be idle.
// The return value is a token which must be provide to calls to other functions
// like lexer() or lexer_finish().
void *lexer_init(int fd, uint32_t timeout);
// lexer() takes token from lexer_init and returns next token index or -1 for
// timeout.
int lexer(void *token);
// lexer_finish() takes token from lexer_init() and shutsdown lexer internal
// data structures.
void lexer_finish(void *token);
// lexer_addpattern() takes a pattern and puts it into the list of patterns
// the lexer looks for.
bool lexer_addpattern(char *pattern);
