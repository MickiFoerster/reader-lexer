#include <pthread.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>

typedef struct {
  int turnoff;
  int input;
} reader_args_t;

int pipe_to_reader[2];

extern unsigned char lexer_input[8];
extern size_t lexer_input_in;
extern size_t lexer_input_out;
extern pthread_mutex_t lexer_input_mtx;
extern pthread_cond_t lexer_input_available;
extern pthread_cond_t lexer_input_fillable;

static void fill_buffer(unsigned char *buf, size_t n) {
    fprintf(stderr, "fill_buffer() starts\n");
    for (size_t i=0; i<n; ++i) {
        pthread_mutex_lock(&lexer_input_mtx);
        {
            size_t j;
            for (;;) {
                j = (lexer_input_in+1) % 
                    (sizeof(lexer_input)/sizeof(lexer_input[0]));
                if (j!=lexer_input_out) {
                    break;
                }
                pthread_cond_wait(&lexer_input_fillable, &lexer_input_mtx);
            }
            lexer_input[j] = buf[i];
            lexer_input_in = j;
            pthread_cond_signal(&lexer_input_available);
        }
        pthread_mutex_unlock(&lexer_input_mtx);
    }
    fprintf(stderr, "fill_buffer() ends\n");
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
      //fprintf(stderr, "fd %d can be read\n", events[i].data.fd);
      unsigned char buf[4096];
      ssize_t n = read(events[i].data.fd, buf, sizeof(buf));
      //fprintf(stderr, "read() returned %ld bytes\n", n);
      if (n > 0) {
        fill_buffer(buf, n);
      }
    }
  }

  fprintf(stderr, "reader: thread terminates ...\n");
  return NULL;
}

void wait_for_reader(void) {}

