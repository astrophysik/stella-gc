#ifndef STELLA_TEST_CHECK_H
#define STELLA_TEST_CHECK_H

#include <stdio.h>
#include <stdlib.h>

/* Unlike assert(), checks remain enabled when compiled with -DNDEBUG. */
#define CHECK(condition) do { \
  if (!(condition)) { \
    fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #condition); \
    exit(EXIT_FAILURE); \
  } \
} while (0)

#endif
