/*
 * libburrow/tests -- Burrow Client Library Unit Tests
 *
 * Copyright 2011 Tony Wooster 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief Burrow attributes tests
 */
#include "tests/common.h"

int main(void)
{
  const size_t COUNT = 7;
  const uint32_t TTL = 100;
  const uint32_t HIDE = 10;
  
  burrow_st *burrow;
  burrow_attributes_st *attr, *attr2, *attr3, *attr4, *attr5;
  uint32_t v;
  size_t size;
  uint i;
  
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
  burrow_test("burrow_attributes check unset");
  if (burrow_attributes_isset_ttl(attr) ||
      burrow_attributes_isset_hide(attr))
    burrow_test_error("badly initialized, some attributes set")
  
  burrow_test("burrow_attributes ttl set check get")
  burrow_attributes_set_ttl(attr, TTL);
  if (burrow_attributes_isset_ttl(attr) != true)
    burrow_test_error("check failed");
  if ((v = burrow_attributes_get_ttl(attr)) != TTL)
    burrow_test_error("expected 100, got: %d", v);
  
  burrow_test("burrow_attributes hide set check get")
  burrow_attributes_set_hide(attr, HIDE);
  if (burrow_attributes_isset_hide(attr) != true)
    burrow_test_error("check failed");
  if ((v = burrow_attributes_get_hide(attr)) != HIDE)
    burrow_test_error("hide: expected 10, got: %d", v);
    

  burrow_test("burrow_attributes_clone")
  if ((attr2 = burrow_attributes_clone(NULL, attr)) == NULL)
    burrow_test_error("returned NULL");
  if (!burrow_attributes_isset_ttl(attr))
    burrow_test_error("check ttl failed");
  if (!burrow_attributes_isset_hide(attr))
    burrow_test_error("check hide failed");
  if (burrow_attributes_get_ttl(attr2) != TTL)
    burrow_test_error("ttl not copied");
  if (burrow_attributes_get_hide(attr2) != HIDE)
    burrow_test_error("hide not copied");


  burrow_test("burrow_attributes_destroy");
  burrow_attributes_destroy(attr);
  
  burrow_test("burrow_destroy");
  burrow_destroy(burrow);
 
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
  
  burrow_attributes_destroy(attr); /* head */
  burrow_attributes_destroy(attr3); /* middle */
  burrow_attributes_destroy(attr5); /* end */
  
  burrow_test("burrow_destroy managed");
  burrow_destroy(burrow);
  
  /* Test non-managed attributes */
  
  burrow_test("burrow_create");
  if ((burrow = burrow_create(NULL, "dummy")) == NULL)
    burrow_test_error("returned NULL")
  
  burrow_test("burrow_attributes_create multiple unmanaged");
  attr = malloc(size * (size_t)COUNT);
  for (i = 0; i < COUNT; i++) {
    if (burrow_attributes_create( (burrow_attributes_st *)((uintptr_t)attr + size * i), burrow ) == NULL)
      burrow_test_error("returned NULL")
  }
  
  burrow_destroy(burrow);
  free(attr);
}
