#include "common.h"
#include "backends/dummy/dummy.h"
#include "backends/http/curl_backend.h"
#include "backends/memory/memory.h"

burrow_backend_functions_st *burrow_backend_load_functions(const char *backend)
{
  if (!backend)
    return NULL;
    
  if (!strcmp(backend, "dummy")) {
    return &burrow_backend_dummy_functions;
  } else if (!strcmp(backend, "http")) {
    return &burrow_backend_http_functions;
  } else if (!strcmp(backend, "memory")) {
    return &burrow_backend_memory_functions;
  }
  
  return NULL;
}
