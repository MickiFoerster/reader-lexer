#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lexer.h"
#include "pattern_matching.h"

int main() {
  patterns_push_back("AAAA", 0);
  patterns_push_back(" login: ", 0);

  int pipe_to_lexer[2];
  int rc = pipe(pipe_to_lexer);
  if (rc != 0) {
    perror("pipe() failed");
    exit(EXIT_FAILURE);
  }

  void *lexer_instance = lexer_init(pipe_to_lexer[0], 500);
  assert(lexer_instance);

  const char *input_text[] = {
      "BBABBAABBBBBBAAABBBBBBAAAABBBBBBBBBB\nhostname login: ",
      "\n\nsuperhostname login: \n\n\n"};
  for (int i = 0; i < sizeof(input_text) / sizeof(input_text[0]); ++i) {
    fprintf(stderr, "give lexer the following input (%d): %s\n", i,
            input_text[i]);
    ssize_t n = write(pipe_to_lexer[1], input_text[i], strlen(input_text[i]));
    assert(n == strlen(input_text[i]));

    switch (i) {
    case 0: {
      int expected_token_sequence[] = {0, 1, -1, -1, -1};
      for (int j = 0; j < sizeof(expected_token_sequence) /
                              sizeof(expected_token_sequence[0]);
           ++j) {
        int token = lexer(lexer_instance);
        assert(token == expected_token_sequence[j]);
        fprintf(stderr, "lexer returned token %d as expected\n", token);
      }
    } break;
    case 1: {
      int expected_token_sequence[] = {1, -1, -1, -1};
      for (int j = 0; j < sizeof(expected_token_sequence) /
                              sizeof(expected_token_sequence[0]);
           ++j) {
        int token = lexer(lexer_instance);
        assert(token == expected_token_sequence[j]);
        fprintf(stderr, "lexer returned token %d as expected\n", token);
      }
    } break;
    default:
      assert(0);
    }
  }
  lexer_finish(lexer_instance);

  return 0;
}
