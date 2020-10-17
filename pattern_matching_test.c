#include "pattern_matching.h"
#include <stdio.h>
int main(void) {
  patterns_push_back("A", 1);
  patterns_push_back("B", 2);
  patterns_push_back("C", 123);
  patterns_push_back("ASDFASDFASDFASDFC", 23);
  patterns_dump();
  patterns_clean();
}
