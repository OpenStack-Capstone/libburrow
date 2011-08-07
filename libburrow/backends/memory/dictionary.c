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
 * @brief Memory backend dictionary implementation
 */

#include "dictionary.h"

/******************************************************************************/
dictionary_st* dictionary_init(dictionary_st* self, burrow_st* burrow)
{
  if(!self)
    if(!(self = burrow_malloc(burrow, sizeof(dictionary_st))))
    {
      burrow_log_error(burrow, "init(): malloc failed.");
      return NULL;
    }
  
  self->first = self->last = NULL;
  self->length = 0;
  self->burrow = burrow;
  return self;
}
/******************************************************************************/
dictionary_node_st* dictionary_add(dictionary_st* self, 
                                   const char* key, 
                                   void* data_pointer)
{
  if(!self)
    return NULL;
  
  dictionary_node_st* new_node;
  new_node = burrow_malloc(self->burrow, sizeof(dictionary_node_st));
  if(!new_node)
  {
    burrow_log_error(self->burrow, "add(): malloc failed: new_node");
    return NULL;
  }
  
  new_node->key = burrow_malloc(self->burrow, strlen(key)+1);
  if(!new_node->key)
  {
    burrow_log_error(self->burrow, "add(): malloc failed: new_node->key");
    burrow_free(self->burrow, new_node);
    return NULL;
  }
  
  strcpy(new_node->key, key);
  new_node->previous = self->last;
    
  if(self->last != NULL) 
    self->last->next = new_node;
    
  self->last = new_node;
    
  if(self->first == NULL)
    self->first = new_node;
    
  new_node->next = NULL;
  new_node->data = data_pointer;
  
  self->length++;
  
  return new_node;
}
/******************************************************************************/
dictionary_node_st* dictionary_get(dictionary_st* self, 
                                   const char* key, 
                                   dictionary_get_action_t default_action)
{
  if(!key || !self)
    return NULL;
  
  dictionary_node_st* current_node = self->first;
  
  int i = 0;
  while((i < self->length) && ++i)
    if(!strcmp(current_node->key, key))
      break;
    else
      current_node = current_node->next;
  
  if(!current_node && default_action == CREATE)
  {
    dictionary_node_st* temp = dictionary_add(self, key, NULL);
    if(!temp)
      return NULL;
    
    dictionary_st* messages = dictionary_init(NULL, self->burrow);
    if(!messages)
      return NULL;
    
    temp->data = messages;
    current_node = temp;
  }
     
  return current_node;
}

/******************************************************************************/
void dictionary_delete_node(dictionary_st* self, const char* key)
{
  if(!self)
    return;
  
  dictionary_node_st* current_node;
  
  if(!(current_node = dictionary_get(self, key, SEARCH)))
    return;
  
  if(current_node->previous != NULL)
    current_node->previous->next = current_node->next;
  else
    self->first = current_node->next;
  
  if(current_node->next != NULL)
    current_node->next->previous = current_node->previous;
  else
    self->last = current_node->previous;
  
  burrow_free(self->burrow, current_node);
  current_node = NULL;
  self->length--;
}
/******************************************************************************/
void dictionary_delete(dictionary_st* self, 
                       const char* l_bound_key, 
                       int32_t u_bound)
{
  if(!self || !self->length)
    return;
  
  dictionary_node_st* current_node;
  dictionary_node_st* next_node;
  
  if(!(current_node = dictionary_get(self, l_bound_key, SEARCH)))
    current_node = self->first;

  while(current_node && u_bound)
  {
    next_node = current_node->next;
    dictionary_delete_node(self, current_node->key);
    current_node = next_node;
    u_bound--;
  }
}
/******************************************************************************/
dictionary_st* dictionary_iter(dictionary_st* self, 
                               const char* l_bound_key, 
                               uint32_t u_bound)
{
  if(!self)
    return NULL;
  
  dictionary_st* dictionary = dictionary_init(NULL, self->burrow);
  if(!dictionary)
    return NULL;
  
  dictionary_node_st* current_node;
  
  if(!(current_node = dictionary_get(self, l_bound_key, SEARCH)))
    current_node = self->first;
  
  if(u_bound == DICTIONARY_LENGTH)
    u_bound = (uint32_t)self->length;
  
  while(current_node && u_bound)
  {
    if(!dictionary_add(dictionary, current_node->key, current_node->data))
    {
      dictionary_delete(dictionary, NULL, dictionary->length);
      break;
    }
    
    current_node = current_node->next;
    u_bound--;
  }
  
  return dictionary;
}
/******************************************************************************/