/***********************************************************************************************************************
 * libburrow -- Memory Backend
 *
 * Copyright (C) 2011 Federico G. Saldarini (saldavonschwartz@gmail.com)
 * All rights reserved. 
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in this directory for full text.
 **********************************************************************************************************************/

/**
 * @file
 * @brief memory backend implementation
 */

#include <libburrow/common.h>
//#include "onefile.h"
#include "dictionary.c"
#include <time.h>

typedef enum {UPDATE, GET, DELETE} scan_action_t;
typedef enum {REPORT, IGNORE} delete_action_t;

typedef dictionary_st queues_st;
typedef dictionary_node_st queue_st;
typedef dictionary_st accounts_st;
typedef dictionary_node_st account_st;


typedef struct
{
  const char* message_id;
  char* body;
  size_t body_size;
  uint32_t ttl;
  uint32_t hide;
} burrow_message_st;

typedef struct 
{
  int selfallocated;
  burrow_st* burrow;
  accounts_st* accounts;
  
} burrow_backend_memory_st;



/*********************************************************************************************************************/
/*********************************************************************************************************************/
static burrow_filters_st* _process_filter(burrow_backend_memory_st* self, const burrow_filters_st* in_filters)
{
  burrow_filters_st* out_filters = burrow_malloc(self->burrow, sizeof(burrow_filters_st));
  out_filters->set |= BURROW_FILTERS_MARKER | BURROW_FILTERS_LIMIT | BURROW_FILTERS_MATCH_HIDDEN;
  
  out_filters->marker = NULL;
  out_filters->limit = DICTIONARY_LENGTH;
  out_filters->match_hidden = false;
  
  if(in_filters)
  {
    if(in_filters->set & BURROW_FILTERS_MARKER)
      out_filters->marker = in_filters->marker;
    
    if(in_filters->set & BURROW_FILTERS_LIMIT)
      out_filters->limit = in_filters->limit;
    
    if(in_filters->set & BURROW_FILTERS_MATCH_HIDDEN)
      out_filters->match_hidden = in_filters->match_hidden;
  }
  
  return out_filters;
}
/*********************************************************************************************************************/
static void _scan_queue(burrow_backend_memory_st* self, const burrow_command_st* cmd, scan_action_t scan_type, delete_action_t delete_action)
{
  dictionary_st* iterator;
  dictionary_node_st* item;
  burrow_message_st* message;
  burrow_attributes_st reference_attributes;
  burrow_filters_st* ref_filters;
  
  uint32_t attributes_ttl;
  uint32_t attributes_hide;
  
  uint32_t current_time = (uint32_t)time(NULL);
  const char* account_key = cmd->account;
  const char* queue_key = cmd->queue;
  
  account_st* account = (get(self->accounts, account_key, SEARCH));
  if(!account)
    return;
  
  queue_st* queue = get(account->data, queue_key, SEARCH);
  if(!queue)
    return;
  
  ref_filters = _process_filter(self, cmd->filters);
  
  attributes_ttl = attributes_hide = 0;
  if(cmd->attributes)
  {
    if(cmd->attributes->set & BURROW_ATTRIBUTES_TTL)
      attributes_ttl = cmd->attributes->ttl + current_time;
    
    if(cmd->attributes->set & BURROW_ATTRIBUTES_HIDE)
      attributes_hide = cmd->attributes->hide + current_time;
  }
  
  iterator = iter(queue->data, ref_filters->marker, ref_filters->limit);
  item = iterator->first;
  
  int i;
  for(i=0; i < iterator->length; i++)
  {
    message = item->data;
    if(message->ttl <= current_time)
    {
      delete_node(queue->data, item->key);
      burrow_free(self->burrow, message);
      item = item->next;
      continue;
    }
    
    if(!ref_filters->match_hidden && (message->hide > current_time))
    {
      item = item->next;
      continue;
    }
    
    switch(scan_type)
    {
      case UPDATE:
        if(attributes_ttl)
          message->ttl = attributes_ttl;
        if(attributes_hide)
          message->hide = attributes_hide;
        
      case GET:
        reference_attributes.ttl = message->ttl - current_time;
        reference_attributes.hide = (message->hide > current_time) ? (message->hide - current_time) : 0;
        reference_attributes.set |= BURROW_ATTRIBUTES_TTL | BURROW_ATTRIBUTES_HIDE;
        
        burrow_callback_message
        (
          self->burrow, 
          message->message_id, 
          message->body, 
          message->body_size, 
          &reference_attributes
        );
        
        break;
        
      case DELETE:
        delete_node(queue->data, item->key);
        
        if(delete_action == REPORT)
        {
          reference_attributes.ttl = message->ttl - current_time;
          reference_attributes.hide = (message->hide > current_time) ? (message->hide - current_time) : 0;
          reference_attributes.set |= BURROW_ATTRIBUTES_TTL | BURROW_ATTRIBUTES_HIDE;
        
          burrow_callback_message
          (
            self->burrow, 
            message->message_id, 
            message->body, 
            message->body_size, 
            &reference_attributes
          );
        }
        
        burrow_free(self->burrow, message);
        
        break;
        
      default:
        break;
    }
    
    item = item->next;
  }
  
  delete(iterator, NULL, iterator->length);
  burrow_free(self->burrow, iterator);
  burrow_free(self->burrow, ref_filters);
  
  if(!((dictionary_st*)(queue->data))->length)
    delete_node(get(self->accounts, account_key, SEARCH)->data, queue->key);
  
  if(!((dictionary_st*)(account->data))->length)
    delete_node(self->accounts, account_key);
}
/*********************************************************************************************************************/
/*********************************************************************************************************************/
/*********************************************************************************************************************/
static burrow_result_t burrow_backend_memory_get_queues(void* ptr, const burrow_command_st* cmd)
{
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr;
  burrow_filters_st* ref_filters = _process_filter(self, cmd->filters);
  
  account_st* account = (get(self->accounts, cmd->account, SEARCH));
  if(!account)
  {
    burrow_free(self->burrow, ref_filters);
    return BURROW_OK;
  }
  
  
  dictionary_st* iterator = iter(account->data, ref_filters->marker, ref_filters->limit);
  burrow_free(self->burrow, ref_filters);
  dictionary_node_st* item = iterator->first;
  
  int i;
  for(i=0; i < iterator->length; i++)
  {
    burrow_callback_queue(self->burrow, item->key);
    item = item->next;
  }
  
  delete(iterator, NULL, iterator->length);
  burrow_free(self->burrow, iterator);
  return BURROW_OK;
}
/*********************************************************************************************************************/
static burrow_result_t burrow_backend_memory_delete_queues(void* ptr, const burrow_command_st* cmd)
{
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr;
  burrow_filters_st* ref_filters = _process_filter(self, cmd->filters);
  dictionary_st* iterator = iter(get(self->accounts, cmd->account, SEARCH)->data, ref_filters->marker, ref_filters->limit);
  dictionary_node_st* item = iterator->first;
  burrow_command_st* erase_cmd = burrow_malloc(self->burrow, sizeof(burrow_command_st)); 
  erase_cmd->account = cmd->account;
  erase_cmd->filters = _process_filter(self, NULL);
  
  int i;
  for(i = 0; i < iterator->length; i++)
  {
    erase_cmd->queue = item->key;
    _scan_queue(self, erase_cmd, DELETE, IGNORE);
    item = item->next;
  }
  
  delete(iterator, NULL, iterator->length);
  burrow_free(self->burrow, iterator);
  burrow_free(self->burrow, ref_filters);
  burrow_free(self->burrow, (burrow_filters_st*)erase_cmd->filters);
  burrow_free(self->burrow, erase_cmd);
  
  return BURROW_OK;
}
/*********************************************************************************************************************/
static burrow_result_t burrow_backend_memory_get_accounts(void* ptr, const burrow_command_st* cmd)
{
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr;
  burrow_filters_st* ref_filters = _process_filter(self, cmd->filters);
  dictionary_st* iterator = iter(self->accounts, ref_filters->marker, ref_filters->limit);
  burrow_free(self->burrow, ref_filters);
  dictionary_node_st* item = iterator->first;
  
  int i;
  for(i=0; i < iterator->length; i++)
  {
    burrow_callback_account(self->burrow, item->key);
    item = item->next;
  }
  
  delete(iterator, NULL, iterator->length);
  burrow_free(self->burrow, iterator);
  return BURROW_OK;
}
/*********************************************************************************************************************/
static burrow_result_t burrow_backend_memory_delete_accounts(void* ptr, const burrow_command_st* cmd)
{
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr;
  burrow_filters_st* ref_filters = _process_filter(self, cmd->filters);
  dictionary_st* iterator = iter(self->accounts, ref_filters->marker, ref_filters->limit);
  dictionary_node_st* item = iterator->first;
  burrow_command_st* erase_cmd = burrow_malloc(self->burrow, sizeof(burrow_command_st)); 
  erase_cmd->filters = _process_filter(self, NULL);
  
  int i;
  for(i = 0; i < iterator->length; i++)
  {
    erase_cmd->account = item->key;
    burrow_backend_memory_delete_queues(self, erase_cmd);
    item = item->next;
  }
  
  delete(iterator, NULL, iterator->length);
  burrow_free(self->burrow, iterator);
  burrow_free(self->burrow, ref_filters);
  burrow_free(self->burrow, (burrow_filters_st*)erase_cmd->filters);
  burrow_free(self->burrow, erase_cmd);
  
  return BURROW_OK;
}
/*********************************************************************************************************************/
static burrow_result_t burrow_backend_memory_get_messages(void *ptr, const burrow_command_st *cmd)
{
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr;  
  _scan_queue(self, cmd, GET, REPORT);
  return BURROW_OK;
}
/*********************************************************************************************************************/
static burrow_result_t burrow_backend_memory_update_messages(void *ptr, const burrow_command_st *cmd)
{
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr;  
  _scan_queue(self, cmd, UPDATE, REPORT);
  return BURROW_OK;
}
/*********************************************************************************************************************/
static burrow_result_t burrow_backend_memory_delete_messages(void *ptr, const burrow_command_st *cmd)
{
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr;  
  _scan_queue(self, cmd, DELETE, REPORT);
  return BURROW_OK;
}
/*********************************************************************************************************************/
static burrow_result_t burrow_backend_memory_create_message(void* ptr, const burrow_command_st* cmd)
{
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr;
  burrow_message_st* new_message = burrow_malloc(self->burrow, sizeof(burrow_message_st));
  char* body = burrow_malloc(self->burrow, cmd->body_size);
  char* message_id = burrow_malloc(self->burrow, strlen(cmd->message_id)+1);
  
  if(!new_message || !body || !message_id)  
  {
    burrow_log_error(self->burrow, "error allocating memory for new message");
    burrow_free(self->burrow, body);
    burrow_free(self->burrow, message_id);
    burrow_free(self->burrow, new_message);
    return BURROW_ERROR_MEMORY;
  }
  
  memcpy(body, cmd->body, cmd->body_size);
  new_message->body = body;
  strcpy(message_id, cmd->message_id);
  new_message->message_id = message_id;
  new_message->body_size = cmd->body_size;
  
  
  uint32_t creation_time = (uint32_t)time(NULL);
  
  if(cmd->attributes && (cmd->attributes->set & BURROW_ATTRIBUTES_TTL))  
    new_message->ttl = creation_time + cmd->attributes->ttl;
  else
    new_message->ttl = creation_time + 300; /* five minutes */
  
  new_message->hide = 0;
  if(cmd->attributes && (cmd->attributes->set & BURROW_ATTRIBUTES_HIDE)) 
    if(cmd->attributes->hide)
      new_message->hide = creation_time + cmd->attributes->hide;
  
  
  queue_st* queue = get(get(self->accounts, cmd->account, CREATE)->data, cmd->queue, CREATE);
  dictionary_node_st* message_node = get(queue->data, new_message->message_id, SEARCH);
  if(message_node)
  {
    burrow_free(self->burrow, message_node->data);
    message_node->data = new_message;
  }
  else
    add(queue->data, new_message->message_id, new_message);
  
  return BURROW_OK;
}
/*********************************************************************************************************************/
static burrow_result_t burrow_backend_memory_update_message(void *ptr, const burrow_command_st *cmd)
{
  account_st* account;
  queue_st* queue;
  dictionary_node_st* message_node;
  burrow_message_st* message;
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr;  
  uint32_t attributes_ttl;
  uint32_t attributes_hide;
  
  uint32_t current_time = (uint32_t)time(NULL);
  
  attributes_ttl = attributes_hide = 0;
  if(cmd->attributes)
  {
    if(cmd->attributes->set & BURROW_ATTRIBUTES_TTL)
      if(cmd->attributes->ttl > 0)
        attributes_ttl = cmd->attributes->ttl + current_time;
    
    if(cmd->attributes->set & BURROW_ATTRIBUTES_HIDE)
        attributes_hide = cmd->attributes->hide + current_time;
  }
  
  if((account = get(self->accounts, cmd->account, SEARCH)))
    if((queue = get(account->data, cmd->queue, SEARCH)))
      if((message_node = get(queue->data, cmd->message_id, SEARCH)))
        if((message = message_node->data))
        {
          if(message->ttl <= current_time)
          {
            burrow_free(self->burrow, message);
            delete_node(queue->data, cmd->message_id);
            dictionary_st* q = queue->data;
            
            if(!q->length)
              delete_node(q, cmd->queue);
            
            return BURROW_OK;
          }
          
          if(attributes_ttl)
            message->ttl = attributes_ttl;
          
          if(attributes_hide)
            message->hide = attributes_hide;
          
          burrow_attributes_st attributes;
          attributes.ttl = message->ttl - current_time;
          attributes.hide = (message->hide > current_time ? message->hide - current_time : 0);
          
          burrow_callback_message(self->burrow, message->message_id, message->body, message->body_size, &attributes);
          return BURROW_OK;
        }
  
  return BURROW_ERROR_INTERNAL;
}
/*********************************************************************************************************************/
static burrow_result_t burrow_backend_memory_get_message(void *ptr, const burrow_command_st *cmd)
{
  account_st* account;
  queue_st* queue;
  dictionary_node_st* message_node;
  burrow_message_st* message;
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr;  
  
  if((account = get(self->accounts, cmd->account, SEARCH)))
    if((queue = get(account->data, cmd->queue, SEARCH)))
      if((message_node = get(queue->data, cmd->message_id, SEARCH)))
        if((message = message_node->data))
        {
          uint32_t current_time = (uint32_t)time(NULL);
          if(message->ttl <= current_time)
          {
            burrow_free(self->burrow, message);
            delete_node(queue->data, cmd->message_id);
            dictionary_st* q = queue->data;
            
            if(!q->length)
              delete_node(q, cmd->queue);
            
            return BURROW_OK;
          }
          
          burrow_attributes_st attributes;
          attributes.ttl = message->ttl - current_time;
          attributes.hide = (message->hide > current_time ? message->hide - current_time : 0);
          
          burrow_callback_message(self->burrow, message->message_id, message->body, message->body_size, &attributes);
          return BURROW_OK;
        }
  
  return BURROW_ERROR_INTERNAL;
}
/*********************************************************************************************************************/
static burrow_result_t burrow_backend_memory_delete_message(void *ptr, const burrow_command_st *cmd)
{
  dictionary_st* queues;
  dictionary_st* queue;
  burrow_message_st* message;
  
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr;  
  if((queues = get(self->accounts, cmd->account, SEARCH)->data))
    if((queue = get(queues, cmd->queue, SEARCH)->data))
      if((message = get(queue, cmd->message_id, SEARCH)->data))
        if(message)
        {
          uint32_t current_time = (uint32_t)time(NULL);
          if(message->ttl > current_time)
          {
            burrow_attributes_st attributes;
            attributes.ttl = message->ttl - current_time;
            attributes.hide = (message->hide > current_time ? message->hide - current_time : 0);
              
            burrow_callback_message(self->burrow, message->message_id, message->body, message->body_size, &attributes);
          }
          
          burrow_free(self->burrow, message);
          delete_node(queue, cmd->message_id);
          
          if(!queue->length)
            delete_node(queues, cmd->queue);
          
          if(!queue->length)
            delete_node(queues, cmd->queue);
          
          if(!queues->length)
            delete_node(self->accounts, cmd->account);
        }
        
  return BURROW_OK;
}
/*********************************************************************************************************************/
static size_t burrow_backend_memory_size(void)
{
  return sizeof(burrow_backend_memory_st);
}
/*********************************************************************************************************************/
static void* burrow_backend_memory_create(void* ptr, burrow_st* burrow)
{
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr;
  
  if(self)
    self->selfallocated = 0;
  else
  {
    if((self = burrow_malloc(burrow, sizeof(burrow_backend_memory_st))) == NULL)
      return NULL;
    
    self->selfallocated = 1;
  }
  
  self->burrow = burrow;
  self->accounts = init(NULL, burrow);
  
  return self;
}
/*********************************************************************************************************************/
static void burrow_backend_memory_free(void* ptr)
{
  burrow_backend_memory_st *self = (burrow_backend_memory_st *)ptr;
  
  burrow_command_st* erase_cmd = burrow_malloc(self->burrow, sizeof(burrow_command_st)); 
  erase_cmd->filters = _process_filter(self, NULL);
  burrow_backend_memory_delete_accounts(self, erase_cmd);
  burrow_free(self->burrow, (burrow_filters_st*)erase_cmd->filters);
  burrow_free(self->burrow, erase_cmd);
  
  if (self->selfallocated)
    burrow_free(self->burrow, self);
}
/*********************************************************************************************************************/
/*********************************************************************************************************************/
/*********************************************************************************************************************/
burrow_backend_functions_st burrow_backend_memory_functions = 
{
  .create           = &burrow_backend_memory_create,
  .destroy          = &burrow_backend_memory_free,
  .size             = &burrow_backend_memory_size,
  /*.clone = &burrow_backend_memory_clone,
  
  .cancel = &burrow_backend_memory_cancel,
  .set_option = &burrow_backend_memory_set_option,
  
  .event_raised = &burrow_backend_memory_event_raised,
  */
  .get_accounts     = &burrow_backend_memory_get_accounts,
  .delete_accounts  = &burrow_backend_memory_delete_accounts,
  
  .get_queues       = &burrow_backend_memory_get_queues,
  .delete_queues    = &burrow_backend_memory_delete_queues,
  
  .get_messages     = &burrow_backend_memory_get_messages,
  .update_messages  = &burrow_backend_memory_update_messages,
  .delete_messages  = &burrow_backend_memory_delete_messages,
  
  .create_message   = &burrow_backend_memory_create_message,
  .get_message      = &burrow_backend_memory_get_message,
  .update_message   = &burrow_backend_memory_update_message,
  .delete_message   = &burrow_backend_memory_delete_message,
  
  
/*  .process = &burrow_backend_memory_process,*/
};
/*********************************************************************************************************************/
/*********************************************************************************************************************/
