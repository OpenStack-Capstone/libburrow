/*
 * libburrow -- Memory Backend: public interface implementation.
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
 * @brief Memory backend public interface implementation
 */

#include <time.h>
#include "dictionary.h"

/* These are the possible actions when scanning a queue:*/
typedef enum 
{
  UPDATE, 
  GET, 
  DELETE
} scan_action_t;

/* And these set whether we go about reporting (or not) messages, 
 as we delete them from a queue:*/ 
typedef enum 
{
  REPORT, 
  IGNORE
} delete_action_t;

/* Some redefinitions, just to make the code more sensible.*/
typedef dictionary_st accounts_st;
typedef dictionary_st queues_st;
typedef dictionary_node_st account_st;
typedef dictionary_node_st queue_st;
typedef dictionary_node_st message_node_st;

/* A burrow message as stored in the memory backend.*/
typedef struct
{
  char* message_id;
  char* body;
  size_t body_size;
  uint32_t ttl;
  uint32_t hide;
} burrow_message_st;

/* The memory backend internal structure.*/
typedef struct 
{
  int selfallocated;
  burrow_st* burrow;
  accounts_st* accounts;
  
} burrow_backend_memory_st;

/******************************************************************************/
static burrow_filters_st* _process_filter(burrow_backend_memory_st* self, 
                                          const burrow_filters_st* in_filters)
{
  /* Allocate a default filter to scan a dictionary from beggining to end 
   and, in the case of queues, ignore hidden messages.*/
  burrow_filters_st* out_filters;
  out_filters = burrow_malloc(self->burrow, sizeof(burrow_filters_st));
  if(!out_filters)
  {
    burrow_log_error(self->burrow, "_process_filter(): malloc failed");
    return NULL;
  }
  
  out_filters->set |= BURROW_FILTERS_LIMIT | BURROW_FILTERS_MATCH_HIDDEN;
  out_filters->marker = NULL;
  out_filters->limit = DICTIONARY_LENGTH;
  out_filters->match_hidden = false;
  
  /* But if the supplied filter has sensible values, use those instead...*/
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
/******************************************************************************/
static int _scan_queue(burrow_backend_memory_st* self, 
                       const burrow_command_st* cmd, 
                       scan_action_t scan_type, 
                       delete_action_t delete_action)
{
  /* Get current time: 
   messages'ttl/hide are relative to "now" = the current time when 
   this function is called.
      
   Also, get the appropriate account and queue.*/
  uint32_t current_time = (uint32_t)time(NULL);
  
  account_st* account = dictionary_get(self->accounts, cmd->account, SEARCH);
  if(!account)
    return 0;
  
  queue_st* queue = dictionary_get(account->data, cmd->queue, SEARCH);
  if(!queue)
    return 0;
  
  /* validate incoming attributes only relevant if updating messages' ttl/hide*/
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
    burrow_log_error(self->burrow, "_scan_queue(): malloc failed");
    return 0;
  }
  
  dictionary_st* iterator = dictionary_iter(queue->data, 
                                            ref_filters->marker, 
                                            ref_filters->limit);
  if(!iterator)
  {
    burrow_free(self->burrow, ref_filters);
    return 0;
  }
  
  dictionary_node_st* item = iterator->first;
  burrow_message_st* message;
  
  /* Iterate through the selected range of messages in a specific queue, 
   performing one of the following, on each message in the range:
   
   UPDATE: update ttl / hide attributes.
   GET:    return the message unmodified.
   DELETE: delete the message.
   
   DELETE additionaly can either IGNORE: just delete the message 
   or REPORT: still return the deleted message.*/
  int i;
  for(i=0; i < iterator->length; i++)
  {
    /* Check if messge ttl expired and if so, delete*/
    message = item->data;
    if(message->ttl <= current_time)
    {
      dictionary_delete_node(queue->data, item->key);
      burrow_free(self->burrow, message->message_id);
      burrow_free(self->burrow, message->body);
      burrow_free(self->burrow, message);
      item = item->next;
      continue;
    }
    /* Check if message hidden, if so skip unless the range includes 
     hidden messages*/
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
        /* FALLTHROUGH*/
        
      case GET:
        reference_attributes.ttl = message->ttl - current_time;
        
        if(message->hide > current_time)
          reference_attributes.hide = message->hide - current_time;
        else
          reference_attributes.hide = 0;
        
        reference_attributes.set |= BURROW_ATTRIBUTES_TTL;
        reference_attributes.set |= BURROW_ATTRIBUTES_HIDE;
        
        burrow_callback_message(self->burrow, 
                                message->message_id, 
                                message->body, 
                                message->body_size, 
                                &reference_attributes);
        
        break;
        
      case DELETE:
        dictionary_delete_node(queue->data, item->key);
        
        if(delete_action == REPORT)
        {
          reference_attributes.ttl = message->ttl - current_time;
          
          if(message->hide > current_time)
            reference_attributes.hide = message->hide - current_time;
          else
            reference_attributes.hide = 0;

          reference_attributes.set |= BURROW_ATTRIBUTES_TTL;
          reference_attributes.set |= BURROW_ATTRIBUTES_HIDE;
          
          burrow_callback_message(self->burrow, 
                                  message->message_id, 
                                  message->body, 
                                  message->body_size, 
                                  &reference_attributes);
         
        }

        burrow_free(self->burrow, message->message_id);
        burrow_free(self->burrow, message->body);
        burrow_free(self->burrow, message);
        
        break;
        
      default:
        break;
    }
    
    item = item->next;
  }
  
  burrow_free(self->burrow, ref_filters);
  dictionary_delete(iterator, NULL, iterator->length);
  burrow_free(self->burrow, iterator);
  
  /* If all messages in a queue were deleted, delete the queue itself.*/
  if(!((dictionary_st*)(queue->data))->length)
  {
    burrow_free(self->burrow, queue->data);
    dictionary_delete_node(dictionary_get(self->accounts, cmd->account, SEARCH)->data, queue->key);
  }
  
  /* And if the recently deleted queue was the only queue in an account, 
   delete that account.*/
  if(!((dictionary_st*)(account->data))->length)
  {
    burrow_free(self->burrow, account->data);
    dictionary_delete_node(self->accounts, cmd->account);    
  }
  
  return 0;
}
/******************************************************************************/
static int burrow_backend_memory_get_queues(void* ptr, 
                                            const burrow_command_st* cmd)
{
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr;
  
  account_st* account = (dictionary_get(self->accounts, cmd->account, SEARCH));
  if(!account)
    return 0;
  
  queues_st* queues = account->data;
  if(!queues)
    return 0;
  
  burrow_filters_st* ref_filters = _process_filter(self, cmd->filters);
  if(!ref_filters)
    return 0;
  
  dictionary_st* iterator = dictionary_iter(queues, 
                                            ref_filters->marker, 
                                            ref_filters->limit);
  if(!iterator)
  {
    burrow_free(self->burrow, ref_filters);
    return 0;
  }
  
  dictionary_node_st* item = iterator->first;
  
  int i;
  for(i=0; i < iterator->length; i++)
  {
    burrow_callback_queue(self->burrow, item->key);
    item = item->next;
  }
  
  burrow_free(self->burrow, ref_filters);
  dictionary_delete(iterator, NULL, iterator->length);
  burrow_free(self->burrow, iterator);
  
  return 0;
}
/******************************************************************************/
static int burrow_backend_memory_delete_queues(void* ptr, 
                                               const burrow_command_st* cmd)
{
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr;
  
  account_st* account = dictionary_get(self->accounts, cmd->account, SEARCH);
  if(!account)
    return 0;
  
  queues_st* queues = account->data;
  if(!queues)
    return 0;
  
  burrow_filters_st* ref_filters = _process_filter(self, cmd->filters);
  if(!ref_filters)
    return 0;
  
  dictionary_st* iterator = dictionary_iter(queues, 
                                            ref_filters->marker, 
                                            ref_filters->limit);
  if(!iterator)
  {
    burrow_free(self->burrow, ref_filters);
    return 0;
  }
  
  burrow_command_st* erase_cmd;
  erase_cmd = burrow_malloc(self->burrow, sizeof(burrow_command_st)); 
  if(!erase_cmd)
  {
    burrow_log_error(self->burrow, "delete_queues(): malloc failed: erase_cmd");
    burrow_free(self->burrow, ref_filters);
    dictionary_delete(iterator, NULL, iterator->length);
    burrow_free(self->burrow, iterator);
    return 0;
  }
  
  erase_cmd->filters = _process_filter(self, NULL);
  if(!erase_cmd->filters)
  {
    burrow_free(self->burrow, ref_filters);
    dictionary_delete(iterator, NULL, iterator->length);
    burrow_free(self->burrow, iterator);
    burrow_free(self->burrow, erase_cmd);
    return 0;
  }
  erase_cmd->attributes = NULL;
  
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
  dictionary_delete(iterator, NULL, iterator->length);
  burrow_free(self->burrow, iterator);
  burrow_free(self->burrow, (burrow_filters_st*)erase_cmd->filters);
  burrow_free(self->burrow, erase_cmd);
  
  return 0;
}
/******************************************************************************/
static int burrow_backend_memory_get_accounts(void* ptr, 
                                              const burrow_command_st* cmd)
{
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr;
  
  burrow_filters_st* ref_filters = _process_filter(self, cmd->filters);
  if(!ref_filters)
    return 0;
  
  dictionary_st* iterator = dictionary_iter(self->accounts, 
                                            ref_filters->marker, 
                                            ref_filters->limit);
  if(!iterator)
  {
    burrow_free(self->burrow, ref_filters);
    return 0;
  }
  
  dictionary_node_st* item = iterator->first;
  
  int i;
  for(i=0; i < iterator->length; i++)
  {
    burrow_callback_account(self->burrow, item->key);
    item = item->next;
  }
  
  burrow_free(self->burrow, ref_filters); 
  dictionary_delete(iterator, NULL, iterator->length);
  burrow_free(self->burrow, iterator);
  
  return 0;
}
/******************************************************************************/
static int burrow_backend_memory_delete_accounts(void* ptr, 
                                                 const burrow_command_st* cmd)
{
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr;
  
  burrow_filters_st* ref_filters = _process_filter(self, cmd->filters);
  if(!ref_filters)
    return 0;
  
  dictionary_st* iterator = dictionary_iter(self->accounts, 
                                            ref_filters->marker, 
                                            ref_filters->limit);
  if(!iterator)
  {
    burrow_free(self->burrow, ref_filters);
    return 0;
  }
  
  burrow_command_st* erase_cmd;
  erase_cmd = burrow_malloc(self->burrow, sizeof(burrow_command_st)); 
  if(!erase_cmd)
  {
    burrow_log_error(self->burrow, 
                     "delete_accounts(): malloc failed: erase_cmd");
    
    burrow_free(self->burrow, ref_filters);
    dictionary_delete(iterator, NULL, iterator->length);
    burrow_free(self->burrow, iterator);
    return 0;
  }
  
  erase_cmd->filters = _process_filter(self, NULL);
  if(!erase_cmd->filters)
  {
    burrow_free(self->burrow, ref_filters);
    dictionary_delete(iterator, NULL, iterator->length);
    burrow_free(self->burrow, iterator);
    burrow_free(self->burrow, erase_cmd);
    return 0;
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
  dictionary_delete(iterator, NULL, iterator->length);
  burrow_free(self->burrow, iterator);
  burrow_free(self->burrow, (burrow_filters_st*)erase_cmd->filters);
  burrow_free(self->burrow, erase_cmd);
  
  return 0;
}
/******************************************************************************/
static int burrow_backend_memory_get_messages(void *ptr, 
                                              const burrow_command_st *cmd)
{
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr;  
  
  _scan_queue(self, cmd, GET, REPORT);
  return 0;
}
/******************************************************************************/
static int burrow_backend_memory_update_messages(void *ptr, 
                                                 const burrow_command_st *cmd)
{
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr;  
  
  _scan_queue(self, cmd, UPDATE, REPORT);
  return 0;
}
/******************************************************************************/
static int burrow_backend_memory_delete_messages(void *ptr, 
                                                 const burrow_command_st *cmd)
{
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr;  
  
  _scan_queue(self, cmd, DELETE, REPORT);
  return 0;
}
/******************************************************************************/
static int burrow_backend_memory_create_message(void* ptr, 
                                                const burrow_command_st* cmd)
{
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr;
  uint32_t creation_time = (uint32_t)time(NULL);
  
  burrow_message_st* new_message;
  new_message = burrow_malloc(self->burrow, sizeof(burrow_message_st));
  
  char* body = burrow_malloc(self->burrow, cmd->body_size);
  
  char* message_id = burrow_malloc(self->burrow, strlen(cmd->message_id)+1);
  
  
  if(!new_message || !body || !message_id)  
  {
    burrow_log_error(self->burrow, "create_message(): malloc failed.");
    burrow_free(self->burrow, new_message);
    burrow_free(self->burrow, body);
    burrow_free(self->burrow, message_id);
    return 0;
  }
  
  memcpy(body, cmd->body, cmd->body_size);
  new_message->body = body;
  strcpy(message_id, cmd->message_id);
  new_message->message_id = message_id;
  new_message->body_size = cmd->body_size;
  
  if(cmd->attributes && (cmd->attributes->set & BURROW_ATTRIBUTES_TTL))  
    new_message->ttl = creation_time + cmd->attributes->ttl;
  else
    new_message->ttl = creation_time + 300; /* five minutes by default.*/
  
  new_message->hide = 0;
  if(cmd->attributes && (cmd->attributes->set & BURROW_ATTRIBUTES_HIDE)) 
    if(cmd->attributes->hide)
      new_message->hide = creation_time + cmd->attributes->hide;
  
  
  account_st* account = dictionary_get(self->accounts, cmd->account, CREATE);
  if(!account)
  {
    burrow_free(self->burrow, new_message);
    burrow_free(self->burrow, body);
    burrow_free(self->burrow, message_id);
    return 0;
  }
  
  queue_st* queue = dictionary_get(account->data, cmd->queue, CREATE);
  if(!queue)
  {
    if(!((dictionary_st*)(account->data))->length)
      dictionary_delete_node(self->accounts, cmd->account);
    
    burrow_free(self->burrow, new_message);
    burrow_free(self->burrow, body);
    burrow_free(self->burrow, message_id);
    return 0;
  }
  
  message_node_st* message_node;
  message_node = dictionary_get(queue->data, new_message->message_id, SEARCH);
  if(message_node)
  {
    burrow_free(self->burrow, message_node->data);
    message_node->data = new_message;
  }
  else
    if(!dictionary_add(queue->data, new_message->message_id, new_message))
      if(!((dictionary_st*)(queue->data))->length)
        dictionary_delete_node(account->data, cmd->queue);
  
  return 0;
}
/******************************************************************************/
static int burrow_backend_memory_update_message(void *ptr, 
                                                const burrow_command_st *cmd)
{
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr; 
  uint32_t current_time = (uint32_t)time(NULL);
  
  account_st* account;
  queue_st* queue;
  message_node_st* message_node;
  burrow_message_st* message; 
  
  if((account = dictionary_get(self->accounts, cmd->account, SEARCH)))
    if((queue = dictionary_get(account->data, cmd->queue, SEARCH)))
      if((message_node = dictionary_get(queue->data, cmd->message_id, SEARCH)))
        if((message = message_node->data))
        {
          if(message->ttl <= current_time)
          {
            burrow_free(self->burrow, message->message_id);
            burrow_free(self->burrow, message->body);
            burrow_free(self->burrow, message);
            dictionary_delete_node(queue->data, cmd->message_id);
            
            if(!((dictionary_st*)(queue->data))->length)
            {
              burrow_free(self->burrow, queue->data);
              dictionary_delete_node(account->data, cmd->queue);
            }
            
            if(!((dictionary_st*)(account->data))->length)
            {
              burrow_free(self->burrow, account->data);
              dictionary_delete_node(self->accounts, cmd->account);
            }
            
            return 0;
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
          if(message->hide > current_time)
            attributes.hide = message->hide - current_time;
          else
            attributes.hide = 0;
          
          burrow_callback_message(self->burrow, 
                                  message->message_id, 
                                  message->body, 
                                  message->body_size, 
                                  &attributes);
          return 0;
        }
  
  return EINVAL;
}
/******************************************************************************/
static int burrow_backend_memory_get_message(void *ptr, 
                                             const burrow_command_st *cmd)
{
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr; 
  uint32_t current_time = (uint32_t)time(NULL);
  
  account_st* account;
  queue_st* queue;
  message_node_st* message_node;
  burrow_message_st* message;
  
  if((account = dictionary_get(self->accounts, cmd->account, SEARCH)))
    if((queue = dictionary_get(account->data, cmd->queue, SEARCH)))
      if((message_node = dictionary_get(queue->data, cmd->message_id, SEARCH)))
        if((message = message_node->data))
        {
          if(message->ttl <= current_time)
          {
            burrow_free(self->burrow, message);
            burrow_free(self->burrow, message->message_id);
            burrow_free(self->burrow, message->body);
            dictionary_delete_node(queue->data, cmd->message_id);
            
            if(!((dictionary_st*)(queue->data))->length)
            {
              burrow_free(self->burrow, queue->data);
              dictionary_delete_node(account->data, cmd->queue);
            }
            
            if(!((dictionary_st*)(account->data))->length)
            {
              burrow_free(self->burrow, account->data);
              dictionary_delete_node(self->accounts, cmd->account);
            }
            
            return 0;
          }
          
          burrow_attributes_st attributes;
          attributes.ttl = message->ttl - current_time;
          if(message->hide > current_time)
            attributes.hide = message->hide - current_time;
          else
            attributes.hide = 0;
          
          burrow_callback_message(self->burrow, 
                                  message->message_id, 
                                  message->body, 
                                  message->body_size, 
                                  &attributes);
          return 0;
        }
  
  return EINVAL;
}
/******************************************************************************/
static int burrow_backend_memory_delete_message(void *ptr, 
                                                const burrow_command_st *cmd)
{
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr;
  uint32_t current_time = (uint32_t)time(NULL);
  
  account_st* account;
  queue_st* queue;
  message_node_st* message_node;
  burrow_message_st* message;
  
  if((account = dictionary_get(self->accounts, cmd->account, SEARCH)))
    if((queue = dictionary_get(account->data, cmd->queue, SEARCH)))
      if((message_node = dictionary_get(queue->data, cmd->message_id, SEARCH)))
        if((message = message_node->data))
        {
          if(message->ttl > current_time)
          {
            burrow_attributes_st attributes;
            attributes.ttl = message->ttl - current_time;
            if(message->hide > current_time)
              attributes.hide = message->hide - current_time;
            else
              attributes.hide = 0;

            
            burrow_callback_message(self->burrow, 
                                    message->message_id, 
                                    message->body, 
                                    message->body_size, 
                                    &attributes);
          }

          burrow_free(self->burrow, message->message_id);
          burrow_free(self->burrow, message->body);
          burrow_free(self->burrow, message);
          dictionary_delete_node(queue->data, cmd->message_id);
          
          if(!((dictionary_st*)(queue->data))->length)
          {
            burrow_free(self->burrow, queue->data);
            dictionary_delete_node(account->data, cmd->queue);
          }
          
          if(!((dictionary_st*)(account->data))->length)
          {
            burrow_free(self->burrow, account->data);
            dictionary_delete_node(self->accounts, cmd->account);
          }
        }
  
  return 0;
}
/******************************************************************************/
static size_t burrow_backend_memory_size(void)
{
  return sizeof(burrow_backend_memory_st);
}
/******************************************************************************/
static void* burrow_backend_memory_create(void* ptr, burrow_st* burrow)
{
  burrow_backend_memory_st* self = (burrow_backend_memory_st*)ptr;
  
  if(self)
    self->selfallocated = 0;
  else
  {
    if(!(self = burrow_malloc(burrow, sizeof(burrow_backend_memory_st))))
    {
      burrow_log_error(burrow, "create(): malloc failed.");
      return NULL;
    }
    
    self->selfallocated = 1;
  }
  
  self->burrow = burrow;
  self->accounts = dictionary_init(NULL, burrow);
  
  return self;
}
/******************************************************************************/
static void burrow_backend_memory_free(void* ptr)
{
  burrow_backend_memory_st *self = (burrow_backend_memory_st *)ptr;
  
  
  burrow_command_st* erase_cmd;
  erase_cmd = burrow_malloc(self->burrow, sizeof(burrow_command_st)); 
  if(!erase_cmd)
  {
    burrow_log_error(self->burrow, 
                     "free(): malloc failed: erase_cmd");
    
    return;
  }
  
  erase_cmd->filters = _process_filter(self, NULL);
  if(!erase_cmd->filters)
    return;
  
  burrow_backend_memory_delete_accounts(self, erase_cmd);
  burrow_free(self->burrow, (burrow_filters_st*)erase_cmd->filters);
  burrow_free(self->burrow, erase_cmd);
  burrow_free(self->burrow, self->accounts);
  
  if (self->selfallocated)
    burrow_free(self->burrow, self);
}
/********FOR-EXPORT STRUCT*****************************************************/
burrow_backend_functions_st burrow_backend_memory_functions = 
{
  .create           = &burrow_backend_memory_create,
  .destroy          = &burrow_backend_memory_free,
  .size             = &burrow_backend_memory_size,
  
  .cancel           = NULL,
  .set_option       = NULL,
  .set_option_int   = NULL,
  .event_raised     = NULL,
  .process          = NULL,

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
};
/******************************************************************************/
