#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>

typedef struct {
  int turnoff;
  int input;
} reader_args_t;

int pipe_to_reader[2];
extern bool timeout_occurred;

typedef struct {
  unsigned char buf[8];
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

int search_pattern(unsigned char *buf, size_t n);

int lexer(void) {
  int pattern_matches = -1;
  for (; pattern_matches == -1;) {
    pthread_mutex_lock(&lexer_input.mtx);
    {
      size_t j;
      for (; !timeout_occurred;) {
        j = (lexer_input.out + 1) %
            (sizeof(lexer_input.buf) / sizeof(lexer_input.buf[0]));
        if (j != lexer_input.in) {
          break;
        }
        pthread_cond_wait(&lexer_input.cond_input_available, &lexer_input.mtx);
      }

      int search_space_start = -1;
      int search_space_end = -1;
      size_t len = 0;
      for (; !timeout_occurred;) {
        size_t i = (lexer_input.out + 1) %
                   (sizeof(lexer_input.buf) / sizeof(lexer_input.buf[0]));
        if (i == lexer_input.in) {
          fprintf(stderr, "\n");
          search_space_end = lexer_input.out;
          break;
        }
        if (search_space_start == -1) {
          search_space_start = i;
        }
        len++;
        fprintf(stderr, "%c", lexer_input.buf[i]);
        lexer_input.out = i;
      }

      len *= sizeof(unsigned char);
      fprintf(stderr, "allocate %ld bytes\n", len);
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
        pattern_matches = search_pattern(buf, len);
        free(buf);
      }

      pthread_cond_signal(&lexer_input.condinput_fillable);
    }
    pthread_mutex_unlock(&lexer_input.mtx);

    if (timeout_occurred) {
      return -1;
    }
  }
  return pattern_matches;
}

static void fill_lexer_buffer(unsigned char *buf, size_t n) {
  fprintf(stderr, "fill_lexer_buffer() starts\n");
  for (size_t i = 0; i < n; ++i) {
    pthread_mutex_lock(&lexer_input.mtx);
    {
      size_t j;
      for (;;) {
        j = (lexer_input.in + 1) %
            (sizeof(lexer_input.buf) / sizeof(lexer_input.buf[0]));
        if (j != lexer_input.out) {
          break;
        }
        pthread_cond_wait(&lexer_input.condinput_fillable, &lexer_input.mtx);
      }

      lexer_input.buf[lexer_input.in] = buf[i];
      lexer_input.in = j;
      pthread_cond_signal(&lexer_input.cond_input_available);
    }
    pthread_mutex_unlock(&lexer_input.mtx);
  }
  fprintf(stderr, "fill_lexer_buffer() ends\n");
}

void *reader_task(void *argv) {
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
    int nfds = epoll_wait(epfd, events, 2, 10000);
    for (int i = 0; i < nfds; ++i) {
      if (events[i].data.fd == arg->turnoff) {
        close(arg->input);
        pthread_cond_signal(&lexer_input.cond_input_available);
        fprintf(stderr, "reader: thread terminates ...\n");
        return NULL;
      }
      fprintf(stderr, "fd %d can be read\n", events[i].data.fd);
      unsigned char buf[4096];
      ssize_t n = read(events[i].data.fd, buf, sizeof(buf));
      fprintf(stderr, "read() returned %ld bytes\n", n);
      if (n > 0) {
        fill_lexer_buffer(buf, n);
      }
    }
  }

  assert(0 && "This point will never be reached");

  return NULL;
}
