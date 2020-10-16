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
#include "reader.h"

typedef struct {
  unsigned char buf[4096];
  size_t in;
  size_t out;
  bool terminate;
  uint32_t timeout_ms;
  lexer_sync_t sync;
  reader_t *reader;
} lexer_input_t;

void lexer_finish(void *token) {
  lexer_terminate = true;
  end_reader();
}

void *lexer_init(int fd, uint32_t timeout_ms) {
  lexer_input_t *new_lexer = (lexer_input_t *)malloc(sizeof(lexer_input_t));
  if (!new_lexer) {
    return NULL;
  }
  const lexer_sync_t sync = {.mtx = PTHREAD_MUTEX_INITIALIZER,
                             .cond_input_available = PTHREAD_COND_INITIALIZER,
                             .condinput_fillable = PTHREAD_COND_INITIALIZER};
  lexer_input_t lexer_input = {.in = 1,
                               .out = 0,
                               .terminate = false,
                               .timeout_ms = timeout_ms,
                               .sync = sync};
  *new_lexer = lexer_input;
  reader_t *reader = start_reader(fd, &new_lexer->sync);
  if (!reader) {
    free(new_lexer);
    return NULL;
  }
  new_lexer->reader = reader;

  patterns = _patterns;
  num_patterns = _num_patterns;

  return new_lexer;
}

bool lexer_addpattern(char *pattern) { return true; }

// lexer() returns index of pattern or -1 when timeout expired
int lexer(void *token) {
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

