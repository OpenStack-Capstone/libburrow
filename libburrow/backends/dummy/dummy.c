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
 * @brief Template for implementing a backend
 */

#include <libburrow/common.h>

typedef struct burrow_backend_dummy_st
{
  int selfallocated;
  burrow_st *burrow;
} burrow_backend_dummy_st;


/** 
 * Implements burrow_backend_functions_st#create
 */
static void *burrow_backend_dummy_create(void *ptr, burrow_st *burrow)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st*)ptr;
  
  if (dummy == NULL)
  {
    dummy = malloc(sizeof(burrow_backend_dummy_st));
    if (!dummy)
      return NULL;
    dummy->selfallocated = 1;
  }
  else
    dummy->selfallocated = 0;
  
  dummy->burrow = burrow;
  
  return dummy;
}

/** 
 * Implements burrow_backend_functions_st#destroy
 */
static void burrow_backend_dummy_destroy(void *ptr)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
    
  if (dummy->selfallocated)
  {
    burrow_log_debug(dummy->burrow,
                     "dummy_destroy: deallocating self-allocated struct");
    burrow_free(dummy->burrow, dummy);
  }
  else
    burrow_log_debug(dummy->burrow,
                     "dummy_destroy: deallocating self-allocated struct");
}

/** 
 * Implements burrow_backend_functions_st#size
 */
static size_t burrow_backend_dummy_size(void)
{
  return sizeof(burrow_backend_dummy_st);
}

/** 
 * Implements burrow_backend_functions_st#set_option
 */
static int burrow_backend_dummy_set_option(void *ptr,
                                           const char *key,
                                           const char *value)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  (void) dummy;
  (void) key;
  (void) value;

  /*
  if (strcmp(key, "key") == 0)
  {
    dummy->key = value;
    return 0;
  }
  */
  
  return EINVAL;
}

/** 
 * Implements burrow_backend_functions_st#set_option_int
 */
static int burrow_backend_dummy_set_option_int(void *ptr,
                                               const char *key,
                                               int32_t value)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  (void) dummy;
  (void) key;
  (void) value;

  /*
  if (strcmp(key, "key") == 0)
  {
    dummy->key = value;
    return 0;
  }
  */
  return EINVAL;
}

/** 
 * Implements burrow_backend_functions_st#cancel
 */
static void burrow_backend_dummy_cancel(void *ptr)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  (void) dummy;
}

/** 
 * Implements burrow_backend_functions_st#process
 */
static int burrow_backend_dummy_process(void *ptr)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  (void) dummy;
  return 0;
}

/** 
 * Implements burrow_backend_functions_st#event_raised
 */
static int burrow_backend_dummy_event_raised(void *ptr,
                                                int fd,
                                                burrow_ioevent_t events)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  (void) dummy;
  (void) fd;
  (void) events;
  
  return 0;
}

/** 
 * Implements burrow_backend_functions_st#get_accounts
 */
static int burrow_backend_dummy_get_accounts(void *ptr,
                                             const burrow_command_st *cmd)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  (void) dummy;
  (void) cmd;

  /*
  account = account_match(cmd->filters);
  
  while(account != null)
  {
    burrow_callback_account(dummy->burrow, account->name);
    account = account->next;
  }  
  */
  return 0;
}

/** 
 * Implements burrow_backend_functions_st#delete_accounts
 */
static int burrow_backend_dummy_delete_accounts(void *ptr,
                                                const burrow_command_st *cmd)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  (void) dummy;
  (void) cmd;
  
  /*
  account = account_match(cmd->filters);
  
  while(account != null)
  {
    account_delete(account->name);
    burrow_callback_account(dummy->burrow, account->name);
    account = account->next;
  }  
  */
  return 0;
}

/** 
 * Implements burrow_backend_functions_st#get_queues
 */
static int burrow_backend_dummy_get_queues(void *ptr,
                                           const burrow_command_st *cmd)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  (void) dummy;
  (void) cmd;
  
  /*
  queue = queue_match(cmd->account, cmd->filters);
  
  while(queue != null)
  {
    burrow_callback_queue(dummy->burrow, queue->name);
    queue = queue->next;
  }  
  */
  return 0;
}

/** 
 * Implements burrow_backend_functions_st#delete_queues
 */
static int burrow_backend_dummy_delete_queues(void *ptr,
                                              const burrow_command_st *cmd)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  (void) dummy;
  (void) cmd;

  /*
  queue = queue_match(cmd->account, cmd->filters);
  
  while(queue != null)
  {
    queue_delete(cmd->account, queue->name);
    burrow_callback_queue(dummy->burrow, queue->name);
    queue = queue->next;
  }  
  */
  return 0;
}

/** 
 * Implements burrow_backend_functions_st#get_messages
 */
static int burrow_backend_dummy_get_messages(void *ptr,
                                             const burrow_command_st *cmd)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  (void) dummy;
  (void) cmd;
  
  /*
  msg = message_match(cmd->account, cmd->queue, cmd->filters);
  
  while(msg != null)
  {
    burrow_callback_message(dummy->burrow, msg->id, msg->body,
                            msg->body_size, msg->attributes);
    msg = msg->next;
  }
  */  
  return 0;
}

/** 
 * Implements burrow_backend_functions_st#delete_messages
 */
static int burrow_backend_dummy_delete_messages(void *ptr,
                                                const burrow_command_st *cmd)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  (void) dummy;
  (void) cmd;

  /*
  msg = message_match(cmd->account, cmd->queue, cmd->filters);
  
  while(msg != null)
  {
    message_delete(cmd->account, cmd->queue, msg->id);
    burrow_callback_message(dummy->burrow, msg->id, msg->body,
                            msg->body_size, msg->attributes);
    msg = msg->next;
  }
  */  
  return 0;
}

/** 
 * Implements burrow_backend_functions_st#update_messages
 */
static int burrow_backend_dummy_update_messages(void *ptr,
                                                const burrow_command_st *cmd)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  (void) dummy;
  (void) cmd;

  /*
  msg = message_match(cmd->account, cmd->queue, cmd->filters);
  
  while(msg != null)
  {
    message_update(cmd->account, cmd->queue, msg->id, cmd->attributes);
    burrow_callback_message(dummy->burrow, msg->id,
                            msg->body, msg->body_size, msg->attributes);
    msg = msg->next;
  }
  */
  return 0;
}

/** 
 * Implements burrow_backend_functions_st#get_message
 */
static int burrow_backend_dummy_get_message(void *ptr,
                                            const burrow_command_st *cmd)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  (void) dummy;
  (void) cmd;

  /*
  msg = message_match_id(cmd->account, cmd->queue,
                         cmd->message_id, cmd->filters);

  if (msg != NULL)
    burrow_callback_message(dummy->burrow, msg->id, msg->body,
                            msg->body_size, msg->attributes);
  */
  return 0;
}

/** 
 * Implements burrow_backend_functions_st#delete_message
 */
static int burrow_backend_dummy_delete_message(void *ptr,
                                               const burrow_command_st *cmd)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  (void) dummy;
  (void) cmd;

  /*
  msg = message_match_id(cmd->account, cmd->queue,
                         cmd->message_id, cmd->filters);

  if (msg != NULL)
  {
    delete_message(cmd->account, cmd->queue, cmd->message_id);
    burrow_callback_message(dummy->burrow, msg->id, msg->body,
                            msg->body_size, msg->attributes);  
  }
  */
  return 0;
}

/** 
 * Implements burrow_backend_functions_st#update_message
 */
static int burrow_backend_dummy_update_message(void *ptr,
                                               const burrow_command_st *cmd)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  (void) dummy;
  (void) cmd;
  
  /*
  msg = message_match_id(cmd->account, cmd->queue,
                         cmd->message_id, cmd->filters);

  if (msg != NULL)
  {
    message_update(cmd->account, cmd->queue, msg->id, cmd->attributes);
    burrow_callback_message(dummy->burrow, msg->id, msg->body,
                            msg->body_size, msg->attributes);  
  }
  */
  return 0;
}

/** 
 * Implements burrow_backend_functions_st#create_message
 */
static int burrow_backend_dummy_create_message(void *ptr,
                                               const burrow_command_st *cmd)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  (void) dummy;
  (void) cmd;

  /*
  msg = message_create(cmd->account, cmd->queue, cmd->message_id,
                       cmd->body, cmd->body_size, cmd->attributes);
                 
  burrow_callback_message(dummy->burrow, msg->id, msg->body,
                          msg->body_size, msg->attributes);
  */
  return 0;
}

/**
 * The public face of burrow_backend_dummy
 *
 * An implementation of @extends burrow_backend_functions_st
 */
burrow_backend_functions_st burrow_backend_dummy_functions =
{
  /* May not be NULL: */
  .create = &burrow_backend_dummy_create,
  .destroy = &burrow_backend_dummy_destroy,
  .size = &burrow_backend_dummy_size,

   /* All of these must be set */
  .get_accounts = &burrow_backend_dummy_get_accounts,
  .delete_accounts = &burrow_backend_dummy_delete_accounts,

  .get_queues = &burrow_backend_dummy_get_queues,
  .delete_queues = &burrow_backend_dummy_delete_queues,

  .get_messages = &burrow_backend_dummy_get_messages,
  .update_messages = &burrow_backend_dummy_update_messages,
  .delete_messages = &burrow_backend_dummy_delete_messages,

  .get_message = &burrow_backend_dummy_get_message,
  .update_message = &burrow_backend_dummy_update_message,
  .delete_message = &burrow_backend_dummy_delete_message,
  .create_message = &burrow_backend_dummy_create_message,

  /* May be NULL: */
  .cancel = &burrow_backend_dummy_cancel,
  .set_option = &burrow_backend_dummy_set_option,
  .set_option_int = &burrow_backend_dummy_set_option_int,
  
  /* These may NOT be NULL if watch_fd is ever called */
  .event_raised = &burrow_backend_dummy_event_raised,
  .process = &burrow_backend_dummy_process,
};
