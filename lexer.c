#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

#include "reader.h"

typedef struct {
    char pattern[32];
    int match_idx;
    void* (*handler)(void*);
} pattern_t;

pattern_t patterns[2] = {
    {.pattern = " login: ", .handler = NULL, .match_idx = 0},
    {.pattern = "AAAA", .handler = NULL, .match_idx = 0}};
const unsigned num_patterns = sizeof(patterns)/sizeof(patterns[0]);

unsigned char lexer_input[8];
size_t lexer_input_in = 1;
size_t lexer_input_out = 0;
pthread_mutex_t lexer_input_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t lexer_input_available = PTHREAD_COND_INITIALIZER;
pthread_cond_t lexer_input_fillable = PTHREAD_COND_INITIALIZER;

int lexer(void) {
    pthread_mutex_lock(&lexer_input_mtx);
    {
        size_t j;
        for (;;) {
            j = (lexer_input_out+1) % 
                (sizeof(lexer_input)/sizeof(lexer_input[0]));
            if (j!=lexer_input_in) {
                break;
            }
            pthread_cond_wait(&lexer_input_available, &lexer_input_mtx);
        }

        for(;;) {
            size_t i = (lexer_input_out+1) % (sizeof(lexer_input)/sizeof(lexer_input[0]));
            if (i==lexer_input_in) {
                break;
            }
            fprintf(stderr, "%c", lexer_input[i]);
            lexer_input_out = i;
        }
        fprintf(stderr, "\n");

        pthread_cond_signal(&lexer_input_fillable);
    }
    pthread_mutex_unlock(&lexer_input_mtx);

  return 0;
#if 0
  /*
      * at least one match (first match counts)
      * no pattern matches
          * is enough input available or could a pattern match as soon as more
     bytes are available
             -> read more until it is clear that no pattern matches
          * enough input available -> go to next position



 What if all patterns do not match? Then search starts from next position
in buffer. But this is potentially from last call to lexer()
  */
  // for (size_t i = 0; i < n; ++i) 
  for (;;) {
    bool at_least_one_pattern_can_match = false;
    for (size_t j = 0; j < num_patterns; ++j) {
      if (patterns[j].match_idx < 0) {
        continue;
      }
      at_least_one_pattern_can_match = true;
      pattern_t *p = &patterns[j];
      if (p->pattern[p->match_idx] == lexer_input[lexer_input_out]) {
        p->match_idx++;
      } else {
        p->match_idx = -1;
      }

      // End of pattern? => match found
      if (p->pattern[p->match_idx] == '\0') {
        for (size_t k = 0; k < num_patterns; ++k) {
          patterns[k].match_idx = 0;
        }
        return j;
      }
    }

    if (!at_least_one_pattern_can_match) {
      wait_for_reader();
    }
  }

  assert(0 && "This point should not be reachable at runtime");
  return -1;
#endif
}

