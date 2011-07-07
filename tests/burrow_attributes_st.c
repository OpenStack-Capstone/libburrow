#include "tests/common.h"

int main(void)
{
  burrow_st *burrow;

  if ((burrow = burrow_create(NULL, "dummy")) == NULL)
    burrow_test_error("burrow_create")
}