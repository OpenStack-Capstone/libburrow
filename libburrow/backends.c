/*
 * libburrow -- Burrow Client Library
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
 * @brief Backend loader implementation
 */

#include "common.h"
#include "backends/dummy/dummy.h"
#include "backends/http/curl_backend.h"
#include "backends/memory/memory.h"

burrow_backend_functions_st *burrow_backend_load_functions(const char *backend)
{
  if (!backend)
    return NULL;
    
  if (!strcmp(backend, "dummy"))
    return &burrow_backend_dummy_functions;
  else if (!strcmp(backend, "http"))
    return &burrow_backend_http_functions;
  else if (!strcmp(backend, "memory"))
    return &burrow_backend_memory_functions;
  
  return NULL;
}
