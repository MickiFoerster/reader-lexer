#pragma once

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  pthread_mutex_t mtx;
  pthread_cond_t cond_input_available;
  pthread_cond_t condinput_fillable;
} lexer_sync_t;

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

// patterns_push_back() takes a pattern and puts it into the list of patterns
// the lexer looks for. The lexer returns id if the pattern is found.
void patterns_push_back(char *pattern, int id);

// patterns_get_pattern_from_ID(id) returns the string corresponding to id.
char *patterns_get_pattern_from_ID(int id);
