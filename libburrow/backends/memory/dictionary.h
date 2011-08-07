/*
 * libburrow -- Memory Backend: internal dictionary structure.
 *
 * Copyright (C) 2011 Federico G. Saldarini.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

/**
 * @file
 * @brief Memory backend dictionary declarations
 */
#include <libburrow/common.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef __DICTIONARY_H
#define __DICTIONARY_H

#ifdef __cplusplus
extern "C" 
{
#endif
  
#define DICTIONARY_LENGTH 0

typedef enum 
{
  SEARCH, 
  CREATE
} dictionary_get_action_t;


  typedef struct dictionary_node_st 
{
  char* key;
  struct dictionary_node_st* previous;
  struct dictionary_node_st* next;
  void* data;
  
} dictionary_node_st;


typedef struct
{
  dictionary_node_st* first;
  dictionary_node_st* last;
  int length;
  burrow_st* burrow;
  
} dictionary_st;


/**
 * Some stuff.
 */
dictionary_st* dictionary_init(dictionary_st* self, burrow_st* burrow);

/**
 * Some stuff.
 */
dictionary_node_st* dictionary_add(dictionary_st* self, 
                                   const char* key, 
                                   void* data_pointer);

/**
 * Some stuff.
 */
dictionary_node_st* dictionary_get(dictionary_st* self, 
                                   const char* key, 
                                   dictionary_get_action_t default_action);

/**
 * Some stuff.
 */
void dictionary_delete_node(dictionary_st* self, const char* key);

/**
 * Some stuff.
 */
void dictionary_delete(dictionary_st* self, 
                       const char* l_bound_key, 
                       int32_t u_bound);

/**
 * Some stuff.
 */
dictionary_st* dictionary_iter(dictionary_st* self, 
                               const char* l_bound_key, 
                               uint32_t u_bound);

#ifdef __cplusplus
}
#endif
#endif