#include "common.h"
#include "backends/dummy/dummy.h"

burrow_backend_functions_st *burrow_backend_load_functions(const char *backend)
{
  if (!backend)
    return NULL;
    
  if (!strcmp(backend, "dummy")) {
    return &burrow_backend_dummy_functions;
  }
  
  return NULL;
}
