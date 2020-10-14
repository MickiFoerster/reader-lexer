#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <time.h>
#include <unistd.h>

#include "lexer.h"

#define PIPE_READ 0
#define PIPE_WRITE 1

typedef struct {
  int turnoff;
  int input;
} reader_args_t;

int search_pattern(unsigned char *buf, size_t n, pattern_t *patterns,
                   size_t num_patterns);
static int pipe_to_reader[2];
static pthread_t *tid_reader = NULL;
static bool lexer_terminate = false;
static bool reader_finished = false;
static uint32_t reader_timeout = -1; // -1 == no timeout in reader
static uint32_t lexer_timeout_ms = 100;
static pattern_t *patterns = NULL;
static size_t num_patterns = 0;

static void start_reader(int fd);
static void end_reader(void);
static void *reader_task(void *argv);

typedef struct {
  unsigned char buf[3];
  size_t in;
  size_t out;
  pthread_mutex_t mtx;
  pthread_cond_t cond_input_available;
  pthread_cond_t condinput_fillable;
} lexer_input_t;

lexer_input_t lexer_input = {.in = 1,
                             .out = 0,
                             .mtx = PTHREAD_MUTEX_INITIALIZER,
                             .cond_input_available = PTHREAD_COND_INITIALIZER,
                             .condinput_fillable = PTHREAD_COND_INITIALIZER};

static void start_reader(int fd) {
  assert(tid_reader == NULL);
  lexer_terminate = false;
  tid_reader = malloc(sizeof(pthread_t));
  if (tid_reader == NULL) {
    perror("malloc() failed");
    exit(EXIT_FAILURE);
  }

  int rc = pipe(pipe_to_reader);
  if (rc != 0) {
    perror("pipe() failed");
    exit(EXIT_FAILURE);
  }

  reader_args_t args = {.turnoff = pipe_to_reader[PIPE_READ], .input = fd};
  reader_args_t *p = (reader_args_t *)malloc(sizeof(reader_args_t));
  if (p == NULL) {
    perror("malloc() failed");
    exit(EXIT_FAILURE);
  }
  *p = args;
  rc = pthread_create(tid_reader, NULL, reader_task, p);
  if (rc != 0) {
    perror("pthread_create() for reader failed");
    free(tid_reader);
    exit(EXIT_FAILURE);
  }

  rc = pthread_detach(*tid_reader);
  if (rc != 0) {
    perror("pthread_detach() failed");
    free(tid_reader);
    exit(EXIT_FAILURE);
  }
}

static void end_reader(void) {
  ssize_t n = write(pipe_to_reader[PIPE_WRITE], "", 1);
  if (n < 0) {
    perror("write() failed");
    exit(EXIT_FAILURE);
  }
  // fprintf(stderr, "Reader was signalled to terminate\n");
  while (!reader_finished) {
    // Make sure threads are not stuck in waiting condition
    pthread_cond_broadcast(&lexer_input.cond_input_available);
    pthread_cond_broadcast(&lexer_input.condinput_fillable);
  }

  free(tid_reader);
  tid_reader = NULL;
}

void lexer_finish(void) {
  lexer_terminate = true;
  end_reader();
}

void lexer_init(int fd, pattern_t *_patterns, size_t _num_patterns,
                uint32_t timeout_ms) {
  if (tid_reader != NULL) {
    lexer_finish();
  }
  start_reader(fd);
  patterns = _patterns;
  num_patterns = _num_patterns;
  lexer_timeout_ms = timeout_ms;
}

// lexer() returns index of pattern or -1 when lexer is terminated
int lexer(void) {
  if (tid_reader == NULL) {
    fprintf(stderr, "error: lexer must be initialized first\n");
    exit(EXIT_FAILURE);
  }

  int pattern_matches = -2;
  for (; pattern_matches == -2;) {
    pthread_mutex_lock(&lexer_input.mtx);
    {
      size_t j;
      for (;;) {
        j = (lexer_input.out + 1) %
            (sizeof(lexer_input.buf) / sizeof(lexer_input.buf[0]));
        if (j != lexer_input.in || lexer_terminate) {
          break;
        }

        // Wait till input is available or timeout expired
        for (;;) {
          struct timespec timeout = {0, 0};
          int err = clock_gettime(CLOCK_REALTIME, &timeout);
          if (err) {
            perror("clock_gettime() failed");
            exit(EXIT_FAILURE);
          }
          timeout.tv_sec += lexer_timeout_ms / 1000;
          timeout.tv_nsec += lexer_timeout_ms % 1000;
          err = pthread_cond_timedwait(&lexer_input.cond_input_available,
                                       &lexer_input.mtx, &timeout);
          if (err) {
            pthread_mutex_unlock(&lexer_input.mtx);
            switch (err) {
            case ETIMEDOUT:
              return -1;
            case EINTR:
              continue;
            default:
              perror("pthread_cond_timedwait() failed");
              exit(EXIT_FAILURE);
            }
          }
          break; // no timeout or error means condition is fullfilled
        }
        //pthread_cond_wait(&lexer_input.cond_input_available, &lexer_input.mtx);
      }

      if (lexer_terminate) {
        pattern_matches = -1;
        pthread_mutex_unlock(&lexer_input.mtx);
        continue;
      }

      int search_space_start = -1;
      int search_space_end = -1;
      size_t len = 0;
      for (;;) {
        size_t i = (lexer_input.out + 1) %
                   (sizeof(lexer_input.buf) / sizeof(lexer_input.buf[0]));
        if (i == lexer_input.in) {
          search_space_end = lexer_input.out;
          break;
        }
        if (search_space_start == -1) {
          search_space_start = i;
        }
        len++;
        lexer_input.out = i;
      }

      len *= sizeof(unsigned char);
      unsigned char *buf = (unsigned char *)malloc(len);
      size_t l = 0;
      assert(buf);
      if (buf) {
        for (size_t i = search_space_start;;) {
          buf[l] = lexer_input.buf[i];
          if (i == search_space_end)
            break;
          l++;
          i = (i + 1) % (sizeof(lexer_input.buf) / sizeof(lexer_input.buf[0]));
        }
        pattern_matches = search_pattern(buf, len, patterns, num_patterns);
        free(buf);
      }

      pthread_cond_signal(&lexer_input.condinput_fillable);
    }
    pthread_mutex_unlock(&lexer_input.mtx);
  }

  return pattern_matches;
}

static void fill_lexer_buffer(unsigned char *buf, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    pthread_mutex_lock(&lexer_input.mtx);
    {
      size_t j;
      for (;;) {
        j = (lexer_input.in + 1) %
            (sizeof(lexer_input.buf) / sizeof(lexer_input.buf[0]));
        if (j != lexer_input.out || lexer_terminate) {
          break;
        }
        pthread_cond_wait(&lexer_input.condinput_fillable, &lexer_input.mtx);
      }

      if (lexer_terminate) {
        pthread_mutex_unlock(&lexer_input.mtx);
        break;
      }

      lexer_input.buf[lexer_input.in] = buf[i];
      lexer_input.in = j;
      pthread_cond_signal(&lexer_input.cond_input_available);
    }
    pthread_mutex_unlock(&lexer_input.mtx);
  }
}

static void *reader_task(void *argv) {
  reader_finished = false;
  reader_args_t *arg = (reader_args_t *)argv;
  int epfd = epoll_create(4);

  struct epoll_event event;
  event.data.fd = arg->input;
  event.events = EPOLLIN;
  epoll_ctl(epfd, EPOLL_CTL_ADD, arg->input, &event);
  event.data.fd = arg->turnoff;
  event.events = EPOLLIN;
  epoll_ctl(epfd, EPOLL_CTL_ADD, arg->turnoff, &event);

  for (;;) {
    struct epoll_event events[2];
    int nfds = epoll_wait(epfd, events, 2, reader_timeout);
    switch (nfds) {
    case -1:
      perror("epoll_wait() failed");
      goto error;
    case 0: // timeout
      continue;
    default:
      break;
    }
    for (int i = 0; i < nfds; ++i) {
      if (events[i].data.fd == arg->turnoff) {
        goto shutdown;
      }
      unsigned char buf[4096];
      ssize_t n = read(events[i].data.fd, buf, sizeof(buf));
      // fprintf(stderr, "read() returned %ld bytes\n", n);
      if (n > 0) {
        fill_lexer_buffer(buf, n);
      }
    }
  }
  assert(0 && "This point will never be reached");

error:
  close(arg->input);
  pthread_cond_signal(&lexer_input.cond_input_available);
  free(argv);
  reader_finished = true;
  exit(EXIT_FAILURE);
shutdown:
  // fprintf(stderr, "reader: thread terminates ...\n");
  close(arg->input);
  pthread_cond_signal(&lexer_input.cond_input_available);
  free(argv);
  reader_finished = true;
  return NULL;
}

