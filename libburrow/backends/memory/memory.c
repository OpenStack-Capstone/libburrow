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

/*  These are the possible actions when scanning a queue:*/
typedef enum {UPDATE, GET, DELETE} scan_action_t;
/*  And these set whether we go about reporting (or not) messages, as we delete them from a queue:*/ 
typedef enum {REPORT, IGNORE} delete_action_t;


typedef dictionary_st accounts_st;
typedef dictionary_st queues_st;
typedef dictionary_node_st account_st;
typedef dictionary_node_st queue_st;
typedef dictionary_node_st message_node_st;


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


/*********INTERNAL HELPER FUNCTIONS************************************************************************************/
/*********************************************************************************************************************/
static burrow_filters_st* _process_filter(burrow_backend_memory_st* self, const burrow_filters_st* in_filters)
{
  /*  Allocate a default filter to scan a dictionary from beggining to end and, in the case of
   queues, ignore hidden messages.*/
  burrow_filters_st* out_filters = burrow_malloc(self->burrow, sizeof(burrow_filters_st));
  if(!out_filters)
  {
    burrow_log_error(self->burrow, "custom malloc failed in memory.c: _process_filter()");
    return NULL;
  }
  
  out_filters->set |= BURROW_FILTERS_LIMIT | BURROW_FILTERS_MATCH_HIDDEN;
  out_filters->marker = NULL;
  out_filters->limit = DICTIONARY_LENGTH;
  out_filters->match_hidden = false;
  
  /*  But if the supplied filter has sensible values, use those instead...*/
  if(in_filters)
  {
    if(in_filters->marker)
      out_filters->marker = in_filters->marker;
    
    if(in_filters->set & BURROW_FILTERS_LIMIT)
      out_filters->limit = in_filters->limit;
    
    if(in_filters->set & BURROW_FILTERS_MATCH_HIDDEN)
      out_filters->match_hidden = in_filters->match_hidden;
  }
  
  return out_filters;
}
/*********************************************************************************************************************/
static burrow_result_t _scan_queue(burrow_backend_memory_st* self, const burrow_command_st* cmd, scan_action_t scan_type, delete_action_t delete_action)
{
  /*  Get current time (messages'ttl/hide are relative to "now" = the current time when this function is called)
   Get the appropriate account and queue.*/
  uint32_t current_time = (uint32_t)time(NULL);
  
  account_st* account = get(self->accounts, cmd->account, SEARCH);
  if(!account)
    return BURROW_OK;
  
  queue_st* queue = get(account->data, cmd->queue, SEARCH);
  if(!queue)
    return BURROW_OK;
  
  /*  validate incoming attributes (only relevant if updating messages' ttl/hide)*/
  burrow_attributes_st reference_attributes;
  uint32_t attributes_ttl = 0; 
  uint32_t attributes_hide = 0;
  if(cmd->attributes)
  {
    if(cmd->attributes->set & BURROW_ATTRIBUTES_TTL)
      attributes_ttl = cmd->attributes->ttl + current_time;
    
    if(cmd->attributes->set & BURROW_ATTRIBUTES_HIDE)
      attributes_hide = cmd->attributes->hide + current_time;
  }
  
  burrow_filters_st* ref_filters = _process_filter(self, cmd->filters);
  if(!ref_filters)
  {
    burrow_log_error(self->burrow, "custom malloc failed in memory.c: _scan_queue()");
    return BURROW_OK;
  }
  
  dictionary_st* iterator = iter(queue->data, ref_filters->marker, ref_filters->limit);
  if(!iterator)
  {
    burrow_free(self->burrow, ref_filters);
    return BURROW_OK;
  }
  
  dictionary_node_st* item = iterator->first;
  burrow_message_st* message;
  
  /*  Iterate through the selected range of messages in a specific queue, performing one of the following, 
   on each message in the range:
   
   UPDATE: update ttl / hide attributes.
   GET:    return the message unmodified.
   DELETE: delete the message.
   
   DELETE additionaly can either IGNORE: just delete the message or REPORT: still return the deleted message.*/
  int i;
  for(i=0; i < iterator->length; i++)
  {
    /*  Check if messge ttl expired and if so, delete*/
    message = item->data;
    if(message->ttl <= current_time)
    {
      delete_node(queue->data, item->key);
      burrow_free(self->burrow, message);
      item = item->next;
      continue;
    }
    /*  Check if message hidden, if so skip unless the range includes hidden messages*/
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
  
  burrow_free(self->burrow, ref_filters);
  delete(iterator, NULL, iterator->length);
  burrow_free(self->burrow, iterator);
  
  /*  If all messages in a queue were deleted, delete the queue itself.*/
  if(!((dictionary_st*)(queue->data))->length)
    delete_node(get(self->accounts, cmd->account, SEARCH)->data, queue->key);
  
  /*  And if the recently deleted queue was the only queue in an account, delete that account.*/
  if(!((dictionary_st*)(account->data))->length)
    delete_node(self->accounts, cmd->account);
  
  return BURROW_OK;
}
/*********************************************************************************************************************/
/*********************************************************************************************************************/

/*********INTERFACE****************************************************************************************************/
/*********************************************************************************************************************/
static burrow_result_t burrow_backend_memory_get_queues(void* ptr, const burrow_command_st* cmd)
{
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr;
  
  account_st* account = (get(self->accounts, cmd->account, SEARCH));
  if(!account)
    return BURROW_OK;
  
  queues_st* queues = account->data;
  if(!queues)
    return BURROW_OK;
  
  burrow_filters_st* ref_filters = _process_filter(self, cmd->filters);
  if(!ref_filters)
  {
    burrow_log_error(self->burrow, "custom malloc failed in memory.c: _scan_queue()");
    return BURROW_OK;
  }
  
  dictionary_st* iterator = iter(queues, ref_filters->marker, ref_filters->limit);
  if(!iterator)
  {
    burrow_free(self->burrow, ref_filters);
    return BURROW_OK;
  }
  
  dictionary_node_st* item = iterator->first;
  
  int i;
  for(i=0; i < iterator->length; i++)
  {
    burrow_callback_queue(self->burrow, item->key);
    item = item->next;
  }
  
  burrow_free(self->burrow, ref_filters);
  delete(iterator, NULL, iterator->length);
  burrow_free(self->burrow, iterator);
  
  return BURROW_OK;
}
/*********************************************************************************************************************/
static burrow_result_t burrow_backend_memory_delete_queues(void* ptr, const burrow_command_st* cmd)
{
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr;
  
  account_st* account = get(self->accounts, cmd->account, SEARCH);
  if(!account)
    return BURROW_OK;
  
  queues_st* queues = account->data;
  if(!queues)
    return BURROW_OK;
  
  burrow_filters_st* ref_filters = _process_filter(self, cmd->filters);
  if(!ref_filters)
  {
    burrow_log_error(self->burrow, "custom malloc failed in memory.c: _scan_queue()");
    return BURROW_OK;
  }
  
  dictionary_st* iterator = iter(queues, ref_filters->marker, ref_filters->limit);
  if(!iterator)
  {
    burrow_free(self->burrow, ref_filters);
    return BURROW_OK;
  }
  
  burrow_command_st* erase_cmd = burrow_malloc(self->burrow, sizeof(burrow_command_st)); 
  if(!erase_cmd)
  {
    burrow_log_error(self->burrow, "custom malloc failed in memory.c: _scan_queue()");
    burrow_free(self->burrow, ref_filters);
    delete(iterator, NULL, iterator->length);
    burrow_free(self->burrow, iterator);
    return BURROW_OK;
  }
  
  erase_cmd->filters = _process_filter(self, NULL);
  if(!erase_cmd->filters)
  {
    burrow_log_error(self->burrow, "custom malloc failed in memory.c: _scan_queue()");
    burrow_free(self->burrow, ref_filters);
    delete(iterator, NULL, iterator->length);
    burrow_free(self->burrow, iterator);
    burrow_free(self->burrow, erase_cmd);
    return BURROW_OK;
  }
  
  dictionary_node_st* item = iterator->first;
  erase_cmd->account = cmd->account;
  
  int i;
  for(i = 0; i < iterator->length; i++)
  {
    erase_cmd->queue = item->key;
    _scan_queue(self, erase_cmd, DELETE, IGNORE);
    item = item->next;
  }
  
  burrow_free(self->burrow, ref_filters);
  delete(iterator, NULL, iterator->length);
  burrow_free(self->burrow, iterator);
  burrow_free(self->burrow, (burrow_filters_st*)erase_cmd->filters);
  burrow_free(self->burrow, erase_cmd);
  
  return BURROW_OK;
}
/*********************************************************************************************************************/
static burrow_result_t burrow_backend_memory_get_accounts(void* ptr, const burrow_command_st* cmd)
{
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr;
  
  burrow_filters_st* ref_filters = _process_filter(self, cmd->filters);
  if(!ref_filters)
  {
    burrow_log_error(self->burrow, "custom malloc failed in memory.c: _scan_queue()");
    return BURROW_OK;
  }
  
  dictionary_st* iterator = iter(self->accounts, ref_filters->marker, ref_filters->limit);
  if(!iterator)
  {
    burrow_free(self->burrow, ref_filters);
    return BURROW_OK;
  }
  
  dictionary_node_st* item = iterator->first;
  
  int i;
  for(i=0; i < iterator->length; i++)
  {
    burrow_callback_account(self->burrow, item->key);
    item = item->next;
  }
  
  burrow_free(self->burrow, ref_filters); 
  delete(iterator, NULL, iterator->length);
  burrow_free(self->burrow, iterator);
  
  return BURROW_OK;
}
/*********************************************************************************************************************/
static burrow_result_t burrow_backend_memory_delete_accounts(void* ptr, const burrow_command_st* cmd)
{
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr;
  
  burrow_filters_st* ref_filters = _process_filter(self, cmd->filters);
  if(!ref_filters)
  {
    burrow_log_error(self->burrow, "custom malloc failed in memory.c: _scan_queue()");
    return BURROW_OK;
  }
  
  dictionary_st* iterator = iter(self->accounts, ref_filters->marker, ref_filters->limit);
  if(!iterator)
  {
    burrow_free(self->burrow, ref_filters);
    return BURROW_OK;
  }
  
  burrow_command_st* erase_cmd = burrow_malloc(self->burrow, sizeof(burrow_command_st)); 
  if(!erase_cmd)
  {
    burrow_log_error(self->burrow, "custom malloc failed in memory.c: _scan_queue()");
    burrow_free(self->burrow, ref_filters);
    delete(iterator, NULL, iterator->length);
    burrow_free(self->burrow, iterator);
    return BURROW_OK;
  }
  
  erase_cmd->filters = _process_filter(self, NULL);
  if(!erase_cmd->filters)
  {
    burrow_log_error(self->burrow, "custom malloc failed in memory.c: _scan_queue()");
    burrow_free(self->burrow, ref_filters);
    delete(iterator, NULL, iterator->length);
    burrow_free(self->burrow, iterator);
    burrow_free(self->burrow, erase_cmd);
    return BURROW_OK;
  }
  
  dictionary_node_st* item = iterator->first;
  
  int i;
  for(i = 0; i < iterator->length; i++)
  {
    erase_cmd->account = item->key;
    burrow_backend_memory_delete_queues(self, erase_cmd);
    item = item->next;
  }
  
  burrow_free(self->burrow, ref_filters);
  delete(iterator, NULL, iterator->length);
  burrow_free(self->burrow, iterator);
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
  uint32_t creation_time = (uint32_t)time(NULL);
  
  burrow_message_st* new_message = burrow_malloc(self->burrow, sizeof(burrow_message_st));
  char* body = burrow_malloc(self->burrow, cmd->body_size);
  char* message_id = burrow_malloc(self->burrow, strlen(cmd->message_id)+1);
  
  if(!new_message || !body || !message_id)  
  {
    burrow_log_error(self->burrow, "error allocating memory for new message");
    burrow_free(self->burrow, new_message);
    burrow_free(self->burrow, body);
    burrow_free(self->burrow, message_id);
    return BURROW_OK;
  }
  
  memcpy(body, cmd->body, cmd->body_size);
  new_message->body = body;
  strcpy(message_id, cmd->message_id);
  new_message->message_id = message_id;
  new_message->body_size = cmd->body_size;
  
  if(cmd->attributes && (cmd->attributes->set & BURROW_ATTRIBUTES_TTL))  
    new_message->ttl = creation_time + cmd->attributes->ttl;
  else
    new_message->ttl = creation_time + 300; /* five minutes */
  
  new_message->hide = 0;
  if(cmd->attributes && (cmd->attributes->set & BURROW_ATTRIBUTES_HIDE)) 
    if(cmd->attributes->hide)
      new_message->hide = creation_time + cmd->attributes->hide;
  
  
  account_st* account = get(self->accounts, cmd->account, CREATE);
  if(!account)
  {
    burrow_free(self->burrow, new_message);
    burrow_free(self->burrow, body);
    burrow_free(self->burrow, message_id);
    return BURROW_OK;
  }
  
  queue_st* queue = get(account->data, cmd->queue, CREATE);
  if(!queue)
  {
    if(!((dictionary_st*)(account->data))->length)
      delete_node(self->accounts, cmd->account);
    
    burrow_free(self->burrow, new_message);
    burrow_free(self->burrow, body);
    burrow_free(self->burrow, message_id);
    return BURROW_OK;
  }
  
  message_node_st* message_node = get(queue->data, new_message->message_id, SEARCH);
  if(message_node)
  {
    burrow_free(self->burrow, message_node->data);
    message_node->data = new_message;
  }
  else
    if(!add(queue->data, new_message->message_id, new_message))
      if(!((dictionary_st*)(queue->data))->length)
        delete_node(account->data, cmd->queue);
  
  return BURROW_OK;
}
/*********************************************************************************************************************/
static burrow_result_t burrow_backend_memory_update_message(void *ptr, const burrow_command_st *cmd)
{
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr; 
  uint32_t current_time = (uint32_t)time(NULL);
  
  account_st* account;
  queue_st* queue;
  message_node_st* message_node;
  burrow_message_st* message; 
  
  if((account = get(self->accounts, cmd->account, SEARCH)))
    if((queue = get(account->data, cmd->queue, SEARCH)))
      if((message_node = get(queue->data, cmd->message_id, SEARCH)))
        if((message = message_node->data))
        {
          if(message->ttl <= current_time)
          {
            burrow_free(self->burrow, message);
            delete_node(queue->data, cmd->message_id);
            
            if(!((dictionary_st*)(queue->data))->length)
              delete_node(account->data, cmd->queue);
            
            if(!((dictionary_st*)(account->data))->length)
              delete_node(self->accounts, cmd->account);
            
            return BURROW_OK;
          }
          
          uint32_t attributes_ttl = 0;
          uint32_t attributes_hide = 0;
          if(cmd->attributes)
          {
            if(cmd->attributes->set & BURROW_ATTRIBUTES_TTL)
              if(cmd->attributes->ttl > 0)
                attributes_ttl = cmd->attributes->ttl + current_time;
            
            if(cmd->attributes->set & BURROW_ATTRIBUTES_HIDE)
              attributes_hide = cmd->attributes->hide + current_time;
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
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr; 
  uint32_t current_time = (uint32_t)time(NULL);
  
  account_st* account;
  queue_st* queue;
  message_node_st* message_node;
  burrow_message_st* message;
  
  if((account = get(self->accounts, cmd->account, SEARCH)))
    if((queue = get(account->data, cmd->queue, SEARCH)))
      if((message_node = get(queue->data, cmd->message_id, SEARCH)))
        if((message = message_node->data))
        {
          if(message->ttl <= current_time)
          {
            burrow_free(self->burrow, message);
            delete_node(queue->data, cmd->message_id);
            
            if(!((dictionary_st*)(queue->data))->length)
              delete_node(account->data, cmd->queue);
            
            if(!((dictionary_st*)(account->data))->length)
              delete_node(self->accounts, cmd->account);
            
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
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr;
  uint32_t current_time = (uint32_t)time(NULL);
  
  account_st* account;
  queue_st* queue;
  message_node_st* message_node;
  burrow_message_st* message;
  
  if((account = get(self->accounts, cmd->account, SEARCH)))
    if((queue = get(account->data, cmd->queue, SEARCH)))
      if((message_node = get(queue->data, cmd->message_id, SEARCH)))
        if((message = message_node->data))
        {
          if(message->ttl > current_time)
          {
            burrow_attributes_st attributes;
            attributes.ttl = message->ttl - current_time;
            attributes.hide = (message->hide > current_time ? message->hide - current_time : 0);
            
            burrow_callback_message(self->burrow, message->message_id, message->body, message->body_size, &attributes);
          }
          
          burrow_free(self->burrow, message);
          delete_node(queue->data, cmd->message_id);
          
          if(!((dictionary_st*)(queue->data))->length)
            delete_node(account->data, cmd->queue);
          
          if(!((dictionary_st*)(account->data))->length)
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
    if(!(self = burrow_malloc(burrow, sizeof(burrow_backend_memory_st))))
    {
      burrow_log_error(burrow, "custom malloc failed in memory.c: memory_create()");
      return NULL;
    }
    
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


/********FOR-EXPORT STRUCT*********************************************************************************************/
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
