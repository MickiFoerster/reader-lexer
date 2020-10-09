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
extern unsigned char lexer_input[8];
extern size_t lexer_input_in;
extern size_t lexer_input_out;
extern pthread_mutex_t lexer_input_mtx;

void *reader_task(void *argv);
void wait_for_reader(void);
