#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>

#define PIPE_READ 0
#define PIPE_WRITE 1

typedef struct {
  int turnoff;
  int input;
} reader_args_t;

int search_pattern(unsigned char *buf, size_t n);
int pipe_to_reader[2];
static pthread_t *tid_reader = NULL;
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

static void start_reader(void) {
  assert(tid_reader == NULL);
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

  reader_args_t args = {.turnoff = pipe_to_reader[PIPE_READ],
                        .input = STDIN_FILENO};
  reader_args_t *p = (reader_args_t*) malloc(sizeof(reader_args_t));
  if (p==NULL) {
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

  atexit(end_reader);
}

static void end_reader(void) {
  ssize_t n = write(pipe_to_reader[PIPE_WRITE], "", 1);
  if (n < 0) {
    perror("write() failed");
    exit(EXIT_FAILURE);
  }
  fprintf(stderr, "Reader was signalled to terminate\n");
  // fprintf(stderr, "Waiting for reader thread to join ... ");
  // int rc = pthread_join(*tid_reader, NULL);
  // if (rc != 0) {
  //  perror("pthread_join() for reader failed");
  //  exit(EXIT_FAILURE);
  //}
  // fprintf(stderr, "joined\n");

  int rc = pthread_cancel(*tid_reader);
  if (rc != 0) {
    perror("pthread_join() for reader failed");
    exit(EXIT_FAILURE);
  }

  free(tid_reader);
}

int lexer(void) {
  if (tid_reader == NULL) {
    start_reader();
  }

  int pattern_matches = -1;
  for (; pattern_matches == -1;) {
    pthread_mutex_lock(&lexer_input.mtx);
    {
      size_t j;
      for (;;) {
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
      for (;;) {
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

static void *reader_task(void *argv) {
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
    fprintf(stderr, "reader returned from epoll_wait\n");
    for (int i = 0; i < nfds; ++i) {
      if (events[i].data.fd == arg->turnoff) {
        close(arg->input);
        pthread_cond_signal(&lexer_input.cond_input_available);
        fprintf(stderr, "reader: thread terminates ...\n");
        free(argv);
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

