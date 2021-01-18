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

char *patterns_get_pattern_from_ID(int id) {
  listOfPatterns_t *elem = first();
  while (elem != NULL) {
    if (elem->pattern->id == id) {
      return elem->pattern->pattern;
    }
    elem = elem->next;
  }
  return NULL;
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

  pattern_t *p;
  for (listOfPatterns_t *elem = first(); elem != NULL; elem = elem->next) {
    p = elem->pattern;
    const char regexp_digit[] = "[0-9]";
    if (p->pattern[p->match_idx] == regexp_digit[0] &&
        strlen(&p->pattern[p->match_idx]) >= strlen(regexp_digit) &&
        strncmp(&p->pattern[p->match_idx], regexp_digit,
                strlen(regexp_digit)) == 0 &&
        ('0' <= ch && ch <= '9')) {
      p->match_idx += strlen(regexp_digit);
      if (p->pattern[p->match_idx] == '\0') {
        goto MATCH;
      }
    } else if (p->pattern[p->match_idx] == ch) {
      p->match_idx++;
      if (p->pattern[p->match_idx] == '\0') {
        goto MATCH;
      }
    } else {
      // fprintf(stderr, "character %c does not fit, pattern %s is out\n",
      //        buf[j], p->pattern);
      p->match_idx = 0;
    }
  }

  return -2;
MATCH:
  // fprintf(stderr, "MATCH!\n");
  for (listOfPatterns_t *e = first(); e != NULL; e = e->next) {
    e->pattern->match_idx = 0;
  }
  return p->id;
}
