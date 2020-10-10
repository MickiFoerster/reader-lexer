#pragma once

#include <pthread.h>
#include <unistd.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

typedef struct {
  int turnoff;
  int input;
} reader_args_t;

extern int pipe_to_reader[2];

void *reader_task(void *argv);
