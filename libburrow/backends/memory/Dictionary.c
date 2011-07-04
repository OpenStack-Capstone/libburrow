////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  PROJECT:  memory backend for libburrow C client API.
//            main dictionary "class" for accounts, queues and messages internal to the backend.
//
//  AUTHOR:   Federico Saldarini
//  
//  NOTE:     This should be considered a rough sketch at this point. 
//            Super cheap O(n) implementation.
//  
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



#include <stdlib.h>
#include <stdio.h>

#ifndef DICTIONARY_H
#define DICTIONARY_H


//  EXTERNAL: only here right now for testing!! ////////////////////////////////////////////////////////////////////////

/* Public */
typedef enum {
  BURROW_DETAIL_UNSET,
  BURROW_DETAIL_NONE,
  BURROW_DETAIL_ID,
  BURROW_DETAIL_ATTRIBUTES,
  BURROW_DETAIL_BODY,
  BURROW_DETAIL_ALL
} burrow_detail_t;

typedef enum {
  BURROW_MAYBE = -1,
  BURROW_FALSE = 0,
  BURROW_TRUE = 1
} burrow_tribool_t;

typedef struct {
  int32_t ttl; /* -1: not set */
  int32_t hide; /* -1: not set */
} burrow_attributes_st;

/* Existence public, internals not */
typedef struct {
  burrow_tribool_t match_hidden; /* MAYBE -- not set */
  int32_t limit; /* -1, not set */
  char *marker;  /* NULL, not set */
  burrow_detail_t detail; /* BURROW_DETAIL_UNSET -- not set */
  int32_t wait;
} burrow_filters_st;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct dictionary_node_st 
{
  char* key;
  struct dictionary_node_st* previous;
  struct dictionary_node_st* next;
  void* value;
} dictionary_node_st;


typedef struct
{
  dictionary_node_st* first;
  dictionary_node_st* last;
  int length;
} dictionary_st;


dictionary_st* init(dictionary_st* this)
{
  this = malloc(sizeof(dictionary_st));
  this->first = this->last = NULL;
  this->length = 0;
  
  return this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int length(dictionary_st* this)
{
  return this->length;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void print(dictionary_st* this)
{
  dictionary_node_st* current_node = this->first;
  
  int i;
  for(i = 0; i < this->length; i++)
  {
    printf("\n index: %i key: %s content %s", i, current_node->key, (char*)current_node->value);
    if(current_node->next != NULL)
      current_node = current_node->next;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

dictionary_node_st* get(dictionary_st* this, char* key)
{
  if(!(this->length))
    return NULL;

  dictionary_node_st* current_node = this->first;
  
  int i = 0;
  while((i < this->length) && ++i)
    if(!strcmp(current_node->key, key))
      break;
    else
      current_node = current_node->next;

  return current_node;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void add(dictionary_st* this, char* key, void* data_pointer)
{
  dictionary_node_st* new_node = get(this, key);
  
  if(new_node == NULL)
  {
    new_node = malloc(sizeof(dictionary_node_st));
    new_node->key = key;
    new_node->previous = this->last;
    
    if(this->last != NULL) 
      this->last->next = new_node;
    
    this->last = new_node;
    
    if(this->first == NULL)
      this->first = new_node;
    
    new_node->next = NULL;
    
    this->length++;
  }
  
  new_node->value = data_pointer;  
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

dictionary_st* iter(dictionary_st* this, burrow_filters_st* filters)
{
  dictionary_st* dictionary = init(dictionary);
  dictionary_node_st* current_node;
  int32_t limit = filters->limit;
  
  if((current_node = get(this, filters->marker)) == NULL)
    current_node = this->first;
  
  while((current_node != NULL) && limit)
  {
    add(dictionary, current_node->key, current_node->value);
    current_node = current_node->next;
    limit--;
  }
  
  return dictionary;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void delete_node(dictionary_st* this, char* key)
{
  dictionary_node_st* current_node = get(this, key);
  
  if(current_node == NULL)
    return;
  
  
  if(current_node->previous != NULL)
    current_node->previous->next = current_node->next;
  else
    this->first = current_node->next;
  
  if(current_node->next != NULL)
    current_node->next->previous = current_node->previous;
  else
    this->last = current_node->previous;
  
  free(current_node);
  this->length--;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void delete(dictionary_st* this, burrow_filters_st* filters)
{
  if(!this->length)
    return;
  
  if(filters == NULL)
  {
    dictionary_node_st* next_node;
    dictionary_node_st* current_node = this->first;
    
    int i;
    for(i = 0; i < this->length; i++)
    {
      next_node = current_node->next;
      free(current_node);
      this->length--;
    }
  
    return;
  }
  
  dictionary_st* key_iterator = iter(this, filters);
  dictionary_node_st* keyed_iteration = key_iterator->first;
  int i;
  for(i = 0; i < key_iterator->length; i++)
  {
    delete_node(this, keyed_iteration->key);
    keyed_iteration = keyed_iteration->next;
  }
  
  delete(key_iterator, NULL);
  free(key_iterator);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif