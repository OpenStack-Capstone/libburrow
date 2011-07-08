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
 * @brief Burrow attributes tests
 */
#include "tests/common.h"

int main(void)
{
  const int COUNT = 7;
  const int TTL = 100;
  const int HIDE = 10;
  
  burrow_st *burrow;
  burrow_attributes_st *attr, *attr2, *attr3, *attr4, *attr5;
  int32_t v;
  size_t size;
  int i;
  
  burrow_test("burrow_attributes_size");
  if ((size = burrow_attributes_size()) == 0)
    burrow_test_error("returned 0 size");
  
  burrow_test("burrow_create");
  if ((burrow = burrow_create(NULL, "dummy")) == NULL)
    burrow_test_error("returned NULL");

  burrow_test("burrow_attributes_create managed");
  // Test managed structures
  if ((attr = burrow_attributes_create(NULL, burrow)) == NULL)
    burrow_test_error("returned NULL");

  // Test default values
  burrow_test("burrow_attributes_get_ttl default");
  if ((v = burrow_attributes_get_ttl(attr)) != -1)
    burrow_test_error("expected -1, got: %d", v);

  burrow_test("burrow_attributes_get_hide default");
  if ((v = burrow_attributes_get_hide(attr)) != -1)
    burrow_test_error("expected -1, got: %d", v);

  burrow_test("burrow_attributes ttl set get")
  burrow_attributes_set_ttl(attr, TTL);
  if ((v = burrow_attributes_get_ttl(attr)) != TTL)
    burrow_test_error("expected 100, got: %d", v);
  
  burrow_test("burrow_attributes hide set get")
  burrow_attributes_set_hide(attr, HIDE);
  if ((v = burrow_attributes_get_hide(attr)) != HIDE)
    burrow_test_error("hide: expected 10, got: %d", v);
    

  burrow_test("burrow_attributes_clone")
  if ((attr2 = burrow_attributes_clone(NULL, attr)) == NULL)
    burrow_test_error("returned NULL");
  if (burrow_attributes_get_ttl(attr2) != TTL)
    burrow_test_error("ttl not copied");
  if (burrow_attributes_get_hide(attr2) != HIDE)
    burrow_test_error("hide not copied");


  burrow_test("burrow_attributes_free");
  burrow_attributes_free(attr);
  
  burrow_test("burrow_free");
  burrow_free(burrow);
 
  /* Test multiple managed attributes at once */
  
  burrow_test("burrow_create");
  if ((burrow = burrow_create(NULL, "dummy")) == NULL)
    burrow_test_error("returned NULL")
  
  burrow_test("burrow_attributes_create managed 5");
  attr = burrow_attributes_create(NULL, burrow);
  attr2 = burrow_attributes_create(NULL, burrow);
  attr3 = burrow_attributes_create(NULL, burrow);
  attr4 = burrow_attributes_create(NULL, burrow);
  attr5 = burrow_attributes_create(NULL, burrow);
  if (!attr || !attr2 || !attr3 || !attr4 || !attr5)
    burrow_test_error("returned NULL");
  
  burrow_attributes_free(attr); /* head */
  burrow_attributes_free(attr3); /* middle */
  burrow_attributes_free(attr5); /* end */
  
  burrow_test("burrow_free managed");
  burrow_free(burrow);
  
  /* Test non-managed attributes */
  
  burrow_test("burrow_create");
  if ((burrow = burrow_create(NULL, "dummy")) == NULL)
    burrow_test_error("returned NULL")
  
  burrow_test("burrow_attributes_create multiple unmanaged");
  attr = malloc(size * COUNT);
  for (i = 0; i < COUNT; i++) {
    if (burrow_attributes_create( (burrow_attributes_st *)((uint8_t*)attr + size * i), burrow ) == NULL)
      burrow_test_error("returned NULL")
  }
  
  burrow_free(burrow);
  
  free(attr);
}
