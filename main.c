#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lexer.h"

int main() {
  int pipe_to_lexer[2];
  int rc = pipe(pipe_to_lexer);
  if (rc != 0) {
    perror("pipe() failed");
    exit(EXIT_FAILURE);
  }

  lexer_init(pipe_to_lexer[0]);
  const char *patterns[4] = {"AAAA", "A", "This is a test.",
                             "\n\nsuperhostname login: \n\n\n"};
  for (int i = 0; i < sizeof(patterns) / sizeof(patterns[0]); ++i) {
    fprintf(stderr, "give lexer the following input: %s\n", patterns[i]);
    write(pipe_to_lexer[1], patterns[i], strlen(patterns[i]));
    int token = lexer();
    fprintf(stderr, "lexer returned %d\n", token);
  }

  return 0;
}
