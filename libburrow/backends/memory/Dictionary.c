/***********************************************************************************************************************
 * libburrow -- Memory Backend internal dictionary / associative array structure.
 *              
 *
 * Copyright (C) 2011 Federico G. Saldarini (saldavonschwartz@gmail.com)
 * All rights reserved. 
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in this directory for full text.
 **********************************************************************************************************************/


#include <stdlib.h>
#include <stdio.h>

#ifndef DICTIONARY_H
#define DICTIONARY_H


/*********************************************************************************************************************/
/*********************************************************************************************************************/

#define DICTIONARY_LENGTH 0

typedef enum {SEARCH, CREATE} dictionary_get_action_t;

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

/*********************************************************************************************************************/
/*********************************************************************************************************************/
static dictionary_st* init(dictionary_st* self, burrow_st* burrow)
{
  if(!self)
    self = burrow_malloc(burrow, sizeof(dictionary_st));
  
  self->first = self->last = NULL;
  self->length = 0;
  self->burrow = burrow;
  return self;
}
/*********************************************************************************************************************/
static dictionary_node_st* add(dictionary_st* self, const char* key, void* data_pointer)
{
  if(!self)
    return NULL;
  
  dictionary_node_st* new_node = burrow_malloc(self->burrow, sizeof(dictionary_node_st));
  new_node->key = burrow_malloc(self->burrow, strlen(key)+1);
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
/*********************************************************************************************************************/
static dictionary_node_st* get(dictionary_st* self, const char* key, dictionary_get_action_t default_action)
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
    dictionary_node_st* temp = add(self, key, NULL);
    dictionary_st* messages = init(NULL, self->burrow);
    temp->data = messages;
    current_node = temp;
  }
     
  return current_node;
}
/*********************************************************************************************************************/
static dictionary_st* iter(dictionary_st* self, const char* l_bound_key, uint32_t u_bound)
{
  if(!self)
    return NULL;
  
  dictionary_st* dictionary = init(NULL, self->burrow);
  dictionary_node_st* current_node;
    
  if(!(current_node = get(self, l_bound_key, SEARCH)))
    current_node = self->first;
  
  if(u_bound == DICTIONARY_LENGTH)
    u_bound = (uint32_t)self->length;
  
  while(current_node && u_bound)
  {
    add(dictionary, current_node->key, current_node->data);
    current_node = current_node->next;
    u_bound--;
  }
  
  return dictionary;
}
/*********************************************************************************************************************/
static void delete_node(dictionary_st* self, const char* key)
{
  if(!self)
    return;
  
  dictionary_node_st* current_node;
  
  if(!(current_node = get(self, key, SEARCH)))
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
/*********************************************************************************************************************/
static void delete(dictionary_st* self, const char* l_bound_key, int32_t u_bound)
{
  if(!self || !self->length)
    return;
  
  dictionary_node_st* current_node;
  dictionary_node_st* next_node;
  
  if(!(current_node = get(self, l_bound_key, SEARCH)))
    current_node = self->first;

  while(current_node && u_bound)
  {
    next_node = current_node->next;
    delete_node(self, current_node->key);
    current_node = next_node;
    u_bound--;
  }
}
/*********************************************************************************************************************/
#endif