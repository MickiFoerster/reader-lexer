#include <pthread.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>

typedef struct {
  int turnoff;
  int input;
} reader_args_t;

int pipe_to_reader[2];

unsigned char lexer_input[8];
size_t lexer_input_in = 0;
size_t lexer_input_out = 0;
pthread_mutex_t lexer_input_mtx = PTHREAD_MUTEX_INITIALIZER;

static void fill_buffer(unsigned char *buf, size_t n) {
  // write n bytes into lexer_input
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
        return NULL;
      }
      fprintf(stderr, "fd %d can be read\n", events[i].data.fd);
      unsigned char buf[4096];
      ssize_t n = read(events[i].data.fd, buf, sizeof(buf));
      fprintf(stderr, "read() returned %ld bytes\n", n);
      if (n > 0) {
        fill_buffer(buf, n);
      }
    }
  }

  return NULL;
}

void wait_for_reader(void) {}

