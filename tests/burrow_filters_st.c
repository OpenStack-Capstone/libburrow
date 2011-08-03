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
  const int COUNT = 7;
  const char *MARKER = "default marker";
  
  burrow_st *burrow;
  burrow_filters_st *filter, *filters[COUNT];
  size_t size;
  uint32_t i;
  const char *cchar;
  burrow_detail_t detail;
  
  (void)filters; /* currently not used */
  
  burrow_test("burrow_filters_size");
  if ((size = burrow_filters_size()) < 1)
    burrow_test_error("returned size < 1");
  
  burrow_test("burrow_create");
  if ((burrow = burrow_create(NULL, "dummy")) == NULL)
    burrow_test_error("returned NULL");


  /* Test managed structures */
  burrow_test("burrow_filters_create managed");
  if ((filter = burrow_filters_create(NULL, burrow)) == NULL)
    burrow_test_error("returned NULL");

  /* Test default values */
  burrow_test("burrow_filters_check");
  if (burrow_filters_isset_wait(filter) ||
      burrow_filters_isset_limit(filter) ||
      burrow_filters_isset_match_hidden(filter) ||
      burrow_filters_isset_detail(filter) ||
      burrow_filters_get_marker(filter) != NULL)
    burrow_test_error("badly initialized, some filters set")
  
  /* Test get/sets */
  burrow_test("burrow_filters marker set get");
  burrow_filters_set_marker(filter, MARKER);
  if (burrow_filters_get_marker(filter) == NULL)
    burrow_test_error("check failed");
  if (strcmp( (cchar = burrow_filters_get_marker(filter)), MARKER) != 0)
    burrow_test_error("expected '%s', got '%s'", MARKER, cchar);

  burrow_test("burrow_filters wait set get");
  burrow_filters_set_wait(filter, 100);
  if (burrow_filters_isset_wait(filter) != true)
    burrow_test_error("check failed");
  if ((i = burrow_filters_get_wait(filter)) != 100)
    burrow_test_error("expected 100, got %d", i);

  burrow_test("burrow_filters limit set get");
  burrow_filters_set_limit(filter, 220);
  if (burrow_filters_isset_limit(filter) != true)
    burrow_test_error("check failed");
  if ((i = burrow_filters_get_limit(filter)) != 220)
    burrow_test_error("expected 220, got %d", i);

  burrow_test("burrow_filters match_hidden set get");
  burrow_filters_set_match_hidden(filter, true);
  if (burrow_filters_isset_match_hidden(filter) != true)
    burrow_test_error("check failed");
  if (burrow_filters_get_match_hidden(filter) != true)
    burrow_test_error("expected true, got false");

  burrow_test("burrow_filters detail set get");
  burrow_filters_set_detail(filter, BURROW_DETAIL_ATTRIBUTES);
  if (burrow_filters_isset_detail(filter) != true)
    burrow_test_error("check failed");
  if ((detail = burrow_filters_get_detail(filter)) != BURROW_DETAIL_ATTRIBUTES)
    burrow_test_error("expeted %d, got %d", (int)BURROW_DETAIL_ATTRIBUTES, (int)detail);

  burrow_test("burrow_filters_destroy");
  burrow_filters_destroy(filter);
  
  burrow_test("burrow_destroy");
  burrow_destroy(burrow);

#if 0 
  burrow_test("burrow_filters_clone")
  if ((attr2 = burrow_filters_clone(NULL, attr)) == NULL)
    burrow_test_error("returned NULL");
  if (burrow_filters_get_ttl(attr2) != TTL)
    burrow_test_error("ttl not copied");
  if (burrow_filters_get_hide(attr2) != HIDE)
    burrow_test_error("hide not copied");


 
  /* Test multiple managed attributes at once */
  
  burrow_test("burrow_create");
  if ((burrow = burrow_create(NULL, "dummy")) == NULL)
    burrow_test_error("returned NULL")
  
  burrow_test("burrow_filters_create managed 5");
  attr = burrow_filters_create(NULL, burrow);
  attr2 = burrow_filters_create(NULL, burrow);
  attr3 = burrow_filters_create(NULL, burrow);
  attr4 = burrow_filters_create(NULL, burrow);
  attr5 = burrow_filters_create(NULL, burrow);
  if (!attr || !attr2 || !attr3 || !attr4 || !attr5)
    burrow_test_error("returned NULL");
  
  burrow_filters_destroy(attr); /* head */
  burrow_filters_destroy(attr3); /* middle */
  burrow_filters_destroy(attr5); /* end */
  
  burrow_test("burrow_destroy managed");
  burrow_destroy(burrow);
  
  /* Test non-managed attributes */
  
  burrow_test("burrow_create");
  if ((burrow = burrow_create(NULL, "dummy")) == NULL)
    burrow_test_error("returned NULL")
  
  burrow_test("burrow_filters_create multiple unmanaged");
  attr = malloc(size * (size_t)COUNT);
  for (i = 0; i < COUNT; i++) {
    if (burrow_filters_create( (burrow_filters_st *)((uint8_t*)attr + (int)size * i), burrow ) == NULL)
      burrow_test_error("returned NULL")
  }
  
  burrow_destroy(burrow);
  free(attr);
#endif
}
