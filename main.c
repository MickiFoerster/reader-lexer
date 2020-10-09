#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "reader.h"

static void *timeout(void *argv) {
  sleep(10);
  fprintf(stderr, "Signal thread to terminate\n");
  ssize_t n = write(pipe_to_reader[PIPE_WRITE], "", 1);
  if (n < 0) {
    perror("write() failed");
    exit(EXIT_FAILURE);
  }
  fprintf(stderr, "write() wrote %ld bytes\n", n);

  return NULL;
}

int main() {
  pthread_t tid_reader;
  pthread_t tid_timeout;
  int rc;

  rc = pipe(pipe_to_reader);
  if (rc != 0) {
    perror("pipe() failed");
    exit(EXIT_FAILURE);
  }

  reader_args_t args = {.turnoff = pipe_to_reader[PIPE_READ],
                        .input = STDIN_FILENO};
  rc = pthread_create(&tid_reader, NULL, reader_task, &args);
  if (rc != 0) {
    perror("pthread_create() for reader failed");
    exit(EXIT_FAILURE);
  }

  rc = pthread_create(&tid_timeout, NULL, timeout, NULL);
  if (rc != 0) {
    perror("pthread_create() for timeout failed");
    exit(EXIT_FAILURE);
  }

  rc = pthread_join(tid_reader, NULL);
  if (rc != 0) {
    perror("pthread_join() for reader failed");
    exit(EXIT_FAILURE);
  }

  rc = pthread_join(tid_timeout, NULL);
  if (rc != 0) {
    perror("pthread_join() for timeout failed");
    exit(EXIT_FAILURE);
  }

  return 0;
}
