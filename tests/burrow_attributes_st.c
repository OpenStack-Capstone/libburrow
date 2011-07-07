#include "tests/common.h"

int main(void)
{
  burrow_st *burrow;
  burrow_attributes_st *attr;
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
    
  
}