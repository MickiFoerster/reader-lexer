#pragma once

#include "lexer.h"

typedef struct {
  int turnoff;
  int input;
} reader_args_t;

typedef struct {
  unsigned char buf[4096];
  size_t in;
  size_t out;
  reader_args_t args;
  int pipe_to_reader[2];
  pthread_t tid_reader;
  bool terminate;
  bool reader_finished;
  uint32_t reader_timeout;
  lexer_sync_t *sync;
} reader_t;

void *start_reader(int fd, lexer_sync_t *sync);
void end_reader(void *token);
