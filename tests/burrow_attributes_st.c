#include "tests/common.h"

int main(void)
{
  burrow_st *burrow;
  burrow_attributes_st *attr, *attr2, *attr3, *attr4, *attr5;
  int32_t v;
  
  burrow_test("burrow_create");
  if ((burrow = burrow_create(NULL, "dummy")) == NULL)
    burrow_test_error("returned NULL")

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

  burrow_test("burrow_attributes ttl get set")
  burrow_attributes_set_ttl(attr, 100);
  if ((v = burrow_attributes_get_ttl(attr)) != 100)
    burrow_test_error("expected 100, got: %d", v);
  
  burrow_test("burrow_attributes hide get set")
  burrow_attributes_set_hide(attr, 10);
  if ((v = burrow_attributes_get_hide(attr)) != 10)
    burrow_test_error("hide: expected 10, got: %d", v);

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
}
