/*
 * libburrow/tests -- Burrow Client Library Unit Tests
 *
 * Copyright (C) 2011 Tony Wooster (twooster@gmail.com)
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in this directory for full text.
 */

/**
 * @file
 * @brief Burrow_st tests
 */

#include "tests/common.h"

int main(void)
{
  burrow_st *burrow;
  ssize_t size;
  
  burrow_test("burrow_size nonsense");
  if ((size = burrow_size("nonsense")) != -1)
    burrow_test_error("returned non -1 size");

  burrow_test("burrow_size NULL");
  if ((size = burrow_size(NULL)) != -1)
    burrow_test_error("returned non -1 size");

  burrow_test("burrow_create nonsense");
  if ((burrow = burrow_create(NULL, "nonsense")) != NULL)
    burrow_test_error("returned non-NULL");

  burrow_test("burrow_create NULL");
  if ((burrow = burrow_create(NULL, NULL)) != NULL)
    burrow_test_error("returned non-NULL");

  /* This test may fail if the dummy is removed in the future */
  burrow_test("burrow_create dummy");
  if ((burrow = burrow_create(NULL, "dummy")) == NULL)
    burrow_test_error("returned NULL");
  
  burrow_test("burrow_free dummy");
  burrow_free(burrow);
  
  /* set options, get options */
  /* set callbacks, test callbacks */
  /* set malloc/free, test malloc/free */
}
