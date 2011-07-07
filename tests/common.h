#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libburrow/burrow.h>

#define burrow_test(...) do \
{ \
  printf(__VA_ARGS__); \
  printf("\n"); \
} while (0);

#define burrow_test_error(...) do \
{ \
  printf("*** %s:%d *** ", __FILE__, __LINE__); \
  printf(__VA_ARGS__); \
  printf("\n"); \
  exit(1); \
} while (0);
