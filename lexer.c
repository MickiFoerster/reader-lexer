#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

#include "reader.h"

typedef struct {
  char pattern[32];
  int match_idx;
  void *(*handler)(void *);
} pattern_t;

pattern_t patterns[2] = {
    {.pattern = " login: ", .handler = NULL, .match_idx = 0},
    {.pattern = "AAAA", .handler = NULL, .match_idx = 0}};
const unsigned num_patterns = sizeof(patterns) / sizeof(patterns[0]);

int search_pattern(unsigned char *buf, size_t n) {
  fprintf(stderr, "search pattern: '");
  for (int i = 0; i < n; ++i) {
    fprintf(stderr, "%c", buf[i]);
  }
  fprintf(stderr, "'\n");
  return -1;
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

