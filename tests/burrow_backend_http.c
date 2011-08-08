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
 * @brief Burrow_st tests
 */

#include "common.h"
#include "burrow_generic_tests.h"

int main(void)
{
  client_st *client;
  client = test_setup("http");
  
  burrow_set_backend_option(client->burrow, "server", "localhost");
  burrow_set_backend_option(client->burrow, "port", "8080");
  
  test_run_functional(client);
  
  test_teardown(client);
  return 0;
}
