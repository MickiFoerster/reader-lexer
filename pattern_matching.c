#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

typedef struct {
  int id;
  char *pattern;
  bool regexp_started;
  int match_idx;
} pattern_t;

typedef struct tagListOfPatterns {
  pattern_t *pattern;
  struct tagListOfPatterns *next;
} listOfPatterns_t;

static listOfPatterns_t patterns = {
    .pattern = NULL,
    .next = NULL,
};

static listOfPatterns_t *first(void) { return patterns.next; }

void patterns_push_back(char *pattern, int id) {
  const size_t sz = sizeof(char) * (strlen(pattern) + 1);
  char *new_pattern_string = (char *)malloc(sz);
  if (new_pattern_string == NULL) {
    perror("malloc() failed");
    exit(EXIT_FAILURE);
  }
  strncpy(new_pattern_string, pattern, sz);

  pattern_t *new_pattern = (pattern_t *)malloc(sizeof(pattern_t));
  if (new_pattern == NULL) {
    perror("malloc() failed");
    exit(EXIT_FAILURE);
  }
  new_pattern->pattern = new_pattern_string;
  new_pattern->id = id;
  new_pattern->match_idx = 0;

  listOfPatterns_t *new_elem =
      (listOfPatterns_t *)malloc(sizeof(listOfPatterns_t));
  if (new_elem == NULL) {
    perror("malloc() failed");
    exit(EXIT_FAILURE);
  }

  new_elem->pattern = new_pattern;
  new_elem->next = NULL;

  listOfPatterns_t *elem = &patterns;
  listOfPatterns_t *enext = patterns.next;
  while (enext != NULL) {
    elem = enext;
    enext = enext->next;
  }

  // elem points to element where we append new element
  elem->next = new_elem;
}

void patterns_dump(void) {
  listOfPatterns_t *elem = first();
  while (elem != NULL) {
    printf("pattern: %s\n", elem->pattern->pattern);
    elem = elem->next;
  }
}

void patterns_clean(void) {
  listOfPatterns_t *elem = first();
  while (elem != NULL) {
    listOfPatterns_t *tmp = elem;
    elem = elem->next;
    free(tmp->pattern->pattern);
    free(tmp->pattern);
    free(tmp);
  }
}

static short regexp_digit_current_state = 0;
static void regexp_digit_init(void) {
  fprintf(stderr, "init statemachine [0-9]\n");
  regexp_digit_current_state = 0;
}

static bool regexp_digit_next(char ch) {
  fprintf(stderr, "statemachine [0-9]: %c\n", ch);
  if ('0' <= ch && ch <= '9') {
    if (regexp_digit_current_state == 0) {
      regexp_digit_current_state = 1;
    } else {
      regexp_digit_current_state = 1;
    }
    fprintf(stderr, "statemachine [0-9]: valid: state=%d\n",
            regexp_digit_current_state);
    return true;
  }
  regexp_digit_current_state = 0xFF;
  fprintf(stderr, "statemachine [0-9]: invalid: state=%d\n",
          regexp_digit_current_state);
  return false;
}

static bool regexp_digit_valid(void) {
  return (regexp_digit_current_state == 1);
}

// search_pattern returns index 0-n of pattern or -2 when no pattern matches
int search_pattern(unsigned char ch) {
  /*
  switch (ch) {
  case '\n':
    fprintf(stderr, "\\n");
    break;
  default:
    fprintf(stderr, "%c", ch);
    break;
  }
  */

  for (listOfPatterns_t *elem = first(); elem != NULL; elem = elem->next) {
    pattern_t *p = elem->pattern;
    const char regexp_nonempty_digits[] = "[0-9]+";
    if (p->pattern[p->match_idx] == regexp_nonempty_digits[0] &&
        strlen(&p->pattern[p->match_idx]) >= strlen(regexp_nonempty_digits) &&
        strncmp(&p->pattern[p->match_idx], regexp_nonempty_digits,
                strlen(regexp_nonempty_digits)) == 0) {
      if (!p->regexp_started) {
        p->regexp_started = true;
        regexp_digit_init();
      }
      bool valid = regexp_digit_valid();
      bool ok = regexp_digit_next(ch);
      if (!ok) {
        if (valid) {
          p->match_idx += strlen(regexp_nonempty_digits);
          if (p->pattern[p->match_idx] == '\0') {
            for (listOfPatterns_t *e = first(); e != NULL; e = e->next) {
              e->pattern->match_idx = 0;
            }
            return p->id;
          }
        } else {
          p->match_idx = 0;
        }
      }
    } else if (p->pattern[p->match_idx] == ch) {
      // fprintf(stderr, "character %c fits in pattern '%s', move forward ...",
      // ch, p->pattern);
      p->match_idx++;
      if (p->pattern[p->match_idx] == '\0') {
        // fprintf(stderr, "MATCH!\n");
        for (listOfPatterns_t *e = first(); e != NULL; e = e->next) {
          e->pattern->match_idx = 0;
        }
        return p->id;
      }
    } else {
      // fprintf(stderr, "character %c does not fit, pattern %s is out\n",
      //        buf[j], p->pattern);
      p->match_idx = 0;
    }
  }

  return -2;
}

