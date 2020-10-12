#pragma once

#include <unistd.h>

typedef struct {
  char pattern[256];
  int match_idx;
  void *(*handler)(void *);
} pattern_t;

void lexer_init(int fd, pattern_t *patterns, size_t num_patterns);
int lexer(void);
void lexer_finish(void);
