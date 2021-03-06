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
 * @brief Backend loader declarations
 */

#ifndef __BURROW_BACKENDS_H
#define __BURROW_BACKENDS_H

#ifdef  __cplusplus
extern "C" {
#endif

/**
 * The backend loader/linker function. Used to load a function structure
 * that defines the interface to a backend.
 *
 * @param backend Which backend to load
 * @return ptr to a burrow_backend_functions_st fully populated or NULL if
 *         backend doesn't exist.
 */
burrow_backend_functions_st *burrow_backend_load_functions(const char *backend);

#ifdef  __cplusplus
}
#endif

#endif /* __BURROW_BACKENDS_H */