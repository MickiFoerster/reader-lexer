#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lexer.h"

pattern_t patterns[] = {
    {.pattern = "AAAA", .handler = NULL, .match_idx = 0},
    {.pattern = " login: ", .handler = NULL, .match_idx = 0}};

int main() {
  int pipe_to_lexer[2];
  int rc = pipe(pipe_to_lexer);
  if (rc != 0) {
    perror("pipe() failed");
    exit(EXIT_FAILURE);
  }

  lexer_init(pipe_to_lexer[0], patterns, 2);
  const char *input_text[] = {"BBABBAABBBBBBAAABBBBBBAAAABBBBBBBBBB", "\n\nsuperhostname login: \n\n\n"};
  for (int i = 0; i < sizeof(input_text) / sizeof(input_text[0]); ++i) {
    fprintf(stderr, "give lexer the following input: %s\n", input_text[i]);
    write(pipe_to_lexer[1], input_text[i], strlen(input_text[i]));
    int token = lexer();
    fprintf(stderr, "lexer returned %d\n", token);
    if (token==-2) {
        break; // timeout
    }
  }

  lexer_finish();

  return 0;
}
