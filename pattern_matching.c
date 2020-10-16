#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "lexer.h"

typedef struct {
  char *pattern;
  int match_idx;
} pattern_t;

static pattern_t *patterns = NULL;
static size_t num_patterns = 0;

// search_pattern returns index 0-n of pattern or -2 when no pattern matches
int search_pattern(unsigned char *buf, size_t n, pattern_t *patterns,
                   size_t num_patterns) {
  /*
  fprintf(stderr, "search pattern (%ld): '", n);
  for (int i = 0; i < n; ++i) {
    switch (buf[i]) {
    case '\n':
      fprintf(stderr, "\\n");
      break;
    default:
      fprintf(stderr, "%c", buf[i]);
      break;
    }
  }
  fprintf(stderr, "'\n");
  */

  for (int j = 0; j < n; ++j) {
    for (int i = 0; i < num_patterns; ++i) {
      pattern_t *p = &patterns[i];
      if (p->pattern[p->match_idx] == buf[j]) {
        // fprintf(stderr, "character %c fits in pattern '%s', move forward
        // ...", buf[j], p->pattern);
        p->match_idx++;
        if (p->pattern[p->match_idx] == '\0') {
          // fprintf(stderr, "MATCH!\n");
          j += strlen(p->pattern);
          for (int j = 0; j < num_patterns; ++j) {
            patterns[j].match_idx = 0;
          }
          return i;
        } else {
          size_t l = p->match_idx;
          while (p->pattern[l] != '\0') {
            l++;
          }
          // fprintf(stderr, "still %ld characters must fit\n", l -
          // p->match_idx);
        }
      } else {
        // fprintf(stderr, "character %c does not fit, pattern %s is out\n",
        //        buf[j], p->pattern);
        p->match_idx = 0;
      }
    }
    }

  return -2;
}

