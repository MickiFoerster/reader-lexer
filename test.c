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
  patterns_push_back("AAAA", 23);
  patterns_push_back(" login: ", 42);
  patterns_push_back(" test 2323", 2323);
  patterns_push_back(" errorcode [0-9][0-9][0-9]", 127);

  int pipe_to_lexer[2];
  int rc = pipe(pipe_to_lexer);
  if (rc != 0) {
    perror("pipe() failed");
    exit(EXIT_FAILURE);
  }

  void *lexer_instance = lexer_init(pipe_to_lexer[0], 100);
  assert(lexer_instance);

  const char *input_text[] = {
      "BBABBAABBBBBBAAABBBBBBAAAABBBBBBBBBB\nhostname login: ",
      "\n\nsuperhostname login: \n\n\n",
      "last command failed with errorcode 127 which is a test 2323 and should "
      "fit",
  };
  for (int i = 0; i < sizeof(input_text) / sizeof(input_text[0]); ++i) {
    fprintf(stderr, "give lexer the following input (%d): %s\n", i,
            input_text[i]);
    ssize_t n = write(pipe_to_lexer[1], input_text[i], strlen(input_text[i]));
    assert(n == strlen(input_text[i]));

    switch (i) {
    case 0: {
      int expected_token_sequence[] = {23, 42};
      for (int j = 0; j < sizeof(expected_token_sequence) /
                              sizeof(expected_token_sequence[0]);
           ++j) {
        int token;
        for (;;) {
          token = lexer(lexer_instance);
          fprintf(stderr, "lexer returned token %d \n", token);
          if (token == expected_token_sequence[j]) {
            break;
          } else if (token == -1) {
            usleep(1000000);
            continue;
          }
          fprintf(stderr, "unexpected token %d \n", token);
          exit(EXIT_FAILURE);
        }
        fprintf(stderr, "lexer returned token %d as expected\n", token);
      }
    } break;
    case 1: {
      int expected_token_sequence[] = {42};
      for (int j = 0; j < sizeof(expected_token_sequence) /
                              sizeof(expected_token_sequence[0]);
           ++j) {
        int token;
        for (;;) {
          token = lexer(lexer_instance);
          fprintf(stderr, "lexer returned token %d \n", token);
          if (token == expected_token_sequence[j]) {
            break;
          } else if (token == -1) {
            usleep(1000000);
            continue;
          }
          fprintf(stderr, "unexpected token %d \n", token);
          exit(EXIT_FAILURE);
        }
        fprintf(stderr, "lexer returned token %d as expected\n", token);
      }
    } break;
    case 2: {
      int expected_token_sequence[] = {127, 2323};
      for (int j = 0; j < sizeof(expected_token_sequence) /
                              sizeof(expected_token_sequence[0]);
           ++j) {
        int token;
        for (;;) {
          token = lexer(lexer_instance);
          fprintf(stderr, "lexer returned token %d \n", token);
          if (token == expected_token_sequence[j]) {
            break;
          } else if (token == -1) {
            usleep(1000000);
            continue;
          }
          fprintf(stderr, "unexpected token %d \n", token);
          exit(EXIT_FAILURE);
        }
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
