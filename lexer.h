#pragma once

#include <stdint.h>
#include <unistd.h>

typedef struct {
  char pattern[256];
  int match_idx;
  void *(*handler)(void *);
} pattern_t;

void lexer_init(int fd, pattern_t *patterns, size_t num_patterns,
                uint32_t timeout);
int lexer(void);
void lexer_finish(void);
