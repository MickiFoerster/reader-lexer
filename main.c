#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int lexer(void);

int main() {
  for (;;) {
    int token = lexer();
    fprintf(stderr, "lexer returned %d\n", token);
  }

  return 0;
}
