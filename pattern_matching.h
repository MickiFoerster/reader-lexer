#pragma once

#include <unistd.h>

int search_pattern(unsigned char ch);
void patterns_dump(void);
void patterns_clean(void);
void patterns_push_back(char *pattern, int id);
