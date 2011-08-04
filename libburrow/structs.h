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
 * @brief Public structures declarations
 */


#ifndef __BURROW_STRUCTS_H
#define __BURROW_STRUCTS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct burrow_attributes_st
 * Filters structure. Definition internal; use burrow_attributes_xyz functions
 * to modify this structure.
 */
struct burrow_attributes_st;

/**
 * @struct burrow_filters_st
 * Filters structure. Definition internal; use burrow_filters_xyz functions to
 * modify this structure.
 */
struct burrow_filters_st;

/**
 * @struct burrow_st
 * Burrow structure. Definition internal; use burrow_xyz functions to
 * modify this structure.
 */
struct burrow_st;


#ifdef __cplusplus
}
#endif

#endif /* __BURROW_STRUCTS_H */