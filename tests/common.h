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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libburrow/burrow.h>

#define burrow_test(...) do \
{ \
  printf(__VA_ARGS__); \
  printf("\n"); \
} while (0);

#define burrow_test_error(...) do \
{ \
  printf("*** %s:%d *** ", __FILE__, __LINE__); \
  printf(__VA_ARGS__); \
  printf("\n"); \
  exit(1); \
} while (0);
