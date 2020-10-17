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
#include "pattern_matching.h"
#include "reader.h"

typedef struct {
  bool terminate;
  uint32_t timeout_ms;
  lexer_sync_t sync;
  reader_t *reader;
} lexer_input_t;

void *lexer_init(int fd, uint32_t timeout_ms) {
  lexer_input_t *new_lexer = (lexer_input_t *)malloc(sizeof(lexer_input_t));
  if (!new_lexer) {
    return NULL;
  }
  const lexer_sync_t sync = {.mtx = PTHREAD_MUTEX_INITIALIZER,
                             .cond_input_available = PTHREAD_COND_INITIALIZER,
                             .condinput_fillable = PTHREAD_COND_INITIALIZER};
  lexer_input_t lexer_input = {
      .terminate = false, .timeout_ms = timeout_ms, .sync = sync};
  *new_lexer = lexer_input;
  reader_t *reader = start_reader(fd, &new_lexer->sync);
  if (!reader) {
    free(new_lexer);
    return NULL;
  }
  new_lexer->reader = reader;

  return new_lexer;
}

void lexer_finish(void *token) {
  lexer_input_t *this = (lexer_input_t *)token;
  this->terminate = true;
  end_reader(this->reader);
}

// lexer() returns index of pattern or -1 when timeout expired
int lexer(void *token) {
  lexer_input_t *this = (lexer_input_t *)token;

  int pattern_matches = -2;
  for (; pattern_matches == -2;) {
    pthread_mutex_lock(&this->sync.mtx);
    {
      size_t j;
      for (;;) {
        j = (this->reader->out + 1) %
            (sizeof(this->reader->buf) / sizeof(this->reader->buf[0]));
        if (j != this->reader->in || this->terminate) {
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
          timeout.tv_sec += this->timeout_ms / 1000;
          timeout.tv_nsec += this->timeout_ms % 1000;
          err = pthread_cond_timedwait(&this->sync.cond_input_available,
                                       &this->sync.mtx, &timeout);
          if (err) {
            pthread_mutex_unlock(&this->sync.mtx);
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

      if (this->terminate) {
        pattern_matches = -1;
        pthread_mutex_unlock(&this->sync.mtx);
        continue;
      }

      int search_space_start = -1;
      int search_space_end = -1;
      size_t len = 0;
      for (;;) {
        size_t i = (this->reader->out + 1) %
                   (sizeof(this->reader->buf) / sizeof(this->reader->buf[0]));
        if (i == this->reader->in) {
          search_space_end = this->reader->out;
          break;
        }
        if (search_space_start == -1) {
          search_space_start = i;
        }
        len++;
        this->reader->out = i;
      }

      len *= sizeof(unsigned char);
      unsigned char *buf = (unsigned char *)malloc(len);
      size_t l = 0;
      assert(buf);
      if (buf) {
        for (size_t i = search_space_start;;) {
          buf[l] = this->reader->buf[i];
          if (i == search_space_end)
            break;
          l++;
          i = (i + 1) %
              (sizeof(this->reader->buf) / sizeof(this->reader->buf[0]));
        }
        pattern_matches = search_pattern(buf, len);
        free(buf);
      }

      pthread_cond_signal(&this->sync.condinput_fillable);
    }
    pthread_mutex_unlock(&this->sync.mtx);
  }

  return pattern_matches;
}

