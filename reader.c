#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "reader.h"

#define PIPE_READ 0
#define PIPE_WRITE 1

#define READER_BUFFER_SIZE 4096

static void *reader_task(void *argv);

void *start_reader(int fd, lexer_sync_t *sync) {
  reader_t *reader = (reader_t *)malloc(sizeof(reader_t));
  if (!reader) {
    return NULL;
  }

  memset(reader, 0, sizeof(reader_t));
  int rc = pipe(reader->pipe_to_reader);
  if (rc != 0) {
    perror("pipe() failed");
    exit(EXIT_FAILURE);
  }
  reader->in = 1;
  reader->out = 0;
  reader->reader_timeout = 100;
  reader->terminate = false;
  reader->reader_finished = false;
  reader->sync = sync;
  const reader_args_t args = {.turnoff = reader->pipe_to_reader[PIPE_READ],
                              .input = fd};
  memcpy(&reader->args, &args, sizeof(reader_args_t));
  rc = pthread_create(&reader->tid_reader, NULL, reader_task, reader);
  if (rc != 0) {
    perror("pthread_create() for reader failed");
    exit(EXIT_FAILURE);
  }

  rc = pthread_detach(reader->tid_reader);
  if (rc != 0) {
    perror("pthread_detach() failed");
    exit(EXIT_FAILURE);
  }

  return reader;
}

void end_reader(void *token) {
  reader_t *reader = (reader_t *)token;
  ssize_t n = write(reader->pipe_to_reader[PIPE_WRITE], "", 1);
  if (n < 0) {
    perror("write() failed");
    exit(EXIT_FAILURE);
  }
  while (!reader->reader_finished) {
    usleep(100000);
  }

  free(reader);
}

static void fill_lexer_buffer(reader_t *reader, unsigned char *buf, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    pthread_mutex_lock(&reader->sync->mtx);
    {
      size_t j;
      for (;;) {
        j = (reader->in + 1) % (sizeof(reader->buf) / sizeof(reader->buf[0]));
        if (j != reader->out || reader->terminate) {
          break;
        }
        pthread_cond_wait(&reader->sync->condinput_fillable,
                          &reader->sync->mtx);
      }

      if (reader->terminate) {
        pthread_mutex_unlock(&reader->sync->mtx);
        break;
      }

      reader->buf[reader->in] = buf[i];
      reader->in = j;
      pthread_cond_signal(&reader->sync->cond_input_available);
    }
    pthread_mutex_unlock(&reader->sync->mtx);
  }
}

static void *reader_task(void *argv) {
  reader_t *reader = (reader_t *)argv;
  int epfd = epoll_create(4);

  struct epoll_event event;
  memset(&event, 0, sizeof(event));
  event.data.fd = reader->args.input;
  event.events = EPOLLIN;
  epoll_ctl(epfd, EPOLL_CTL_ADD, reader->args.input, &event);
  event.data.fd = reader->args.turnoff;
  event.events = EPOLLIN;
  epoll_ctl(epfd, EPOLL_CTL_ADD, reader->args.turnoff, &event);

  for (;;) {
    struct epoll_event events[2];
    int nfds = epoll_wait(epfd, events, 2, reader->reader_timeout);
    switch (nfds) {
    case -1:
      perror("epoll_wait() failed");
      close(reader->args.input);
      pthread_cond_signal(&reader->sync->cond_input_available);
      reader->reader_finished = true;
      exit(EXIT_FAILURE);
    case 0: // timeout
      continue;
    default:
      break;
    }
    for (int i = 0; i < nfds; ++i) {
      if (events[i].data.fd == reader->args.turnoff) {
        close(reader->args.input);
        pthread_cond_signal(&reader->sync->cond_input_available);
        fprintf(stderr, "exit from reader thread\n");
        reader->reader_finished = true;
        return NULL;
      }
      unsigned char buf[READER_BUFFER_SIZE];
      ssize_t n = read(events[i].data.fd, buf, sizeof(buf));
      // fprintf(stderr, "read() returned %ld bytes\n", n);
      if (n > 0) {
        buf[n] = '\0';
        printf("%s", buf);
        fflush(stdout);
        fill_lexer_buffer(reader, buf, n);
      }
    }
  }
  assert(0 && "This point will never be reached");
}
