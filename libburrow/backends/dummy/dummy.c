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
 * Creates a backend.
 *
 * @param ptr NULL, meaning "please allocate for me", or pointer to
 *            memory allocated after calling size function.
 * @param burrow Burrow object to be associated with this backend.
 * @return pointer to initialized memory
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
 * Deallocates/destroys a backend.
 *
 * @param ptr pointer to a backend struct
 * @return pointer to initialized memory
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
 * Returns the size of a base backend structure so that the user
 * or frontend can allocate space appropriately.
 *
 * @return size of a base backend structure
 */
static size_t burrow_backend_dummy_size(void)
{
  return sizeof(burrow_backend_dummy_st);
}

/**
 * Generic set_option string, string function.
 *
 * @param ptr Pointer to backend struct
 * @param key Key name, guaranteed to be non-NULL
 * @param value Value, may be NULL
 * @return 0 on success, EINVAL if key or value is invalid
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
 * Generic set_option_int -> string, int function.
 *
 * @param ptr Pointer to backend struct
 * @param key Key name, guaranteed to be non-NULL
 * @param value Value
 * @return 0 on success, EINVAL if key or value is invalid
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
 * Called when the user requests all pending activity to be canceled.
 * The backend should be in a good but idle state after this call.
 * All previously requested watch-fds will no longer be watched after
 * this call.
 *
 * @param ptr pointer to backend struct
 */
static void burrow_backend_dummy_cancel(void *ptr)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  (void) dummy;
}

/**
 * Called when the user wants the backend to continue processing whichever
 * command last returned EAGAIN.
 *
 * Backends SHOULD NOT block; if a backend would block, it should instead
 * call burrow_watch_fd() one or more times and return EAGAIN.
 *
 * @param ptr pointer to backend struct
 * @return 0 on command completion, EAGAIN on wouldblock, or any other
 *         errno constant on error
 */
static int burrow_backend_dummy_process(void *ptr)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  (void) dummy;
  return 0;
}


/**
 * Called when an event previously watched by calling burrow_watch_fd
 * comes live. Processing should not occur at this stage, only within
 * the backend's process function.
 *
 * @param ptr pointer to backend struct
 * @param fd file descriptor that came live
 * @param event bit-array of events that came live
 * @return 0 if the backend is ready to have process called on it, EAGAIN
 *         if the backend is still waiting for more events to come live,
 *         or any other errno on error, which will result in burrow_cancel
 *         being invoked against this backend after return.
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
 * Invocation of a get_accounts command. The following is guaranteed:
 *   cmd->filters MAY be NULL, check individual filters
 *
 * @param ptr Pointer to backend context
 * @param cmd Command structure
 * @return 0 on success, EAGAIN if would block, any other errors otherwise
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
 * Invocation of a delete_accounts command. The following is guaranteed:
 *   cmd->filters MAY be NULL, check individual filters
 *
 * @param ptr Pointer to backend context
 * @param cmd Command structure
 * @return 0 on success, EAGAIN if would block, any other errors otherwise
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
 * Invocation of a get_queues command. The following is guaranteed:
 *   cmd->account WILL be non-NULL
 *   cmd->filters MAY be NULL, check individual filters
 *
 * @param ptr Pointer to backend context
 * @param cmd Command structure
 * @return 0 on success, EAGAIN if would block, any other errors otherwise
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
 * Invocation of a delete_queues command. The following is guaranteed:
 *   cmd->account WILL be non-NULL
 *   cmd->filters MAY be NULL, check individual filters
 *
 * @param ptr Pointer to backend context
 * @param cmd Command structure
 * @return 0 on success, EAGAIN if would block, any other errors otherwise
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
 * Invocation of a get_messages command. The following is guaranteed:
 *   cmd->account WILL be non-NULL
 *   cmd->queue WILL be non-NULL
 *   cmd->filters MAY be NULL, check individual filters
 *
 * @param ptr Pointer to backend context
 * @param cmd Command structure
 * @return 0 on success, EAGAIN if would block, any other errors otherwise
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
 * Invocation of a delete_messages command. The following is guaranteed:
 *   cmd->account WILL be non-NULL
 *   cmd->queue WILL be non-NULL
 *   cmd->filters MAY be NULL, check individual filters
 *
 * @param ptr Pointer to backend context
 * @param cmd Command structure
 * @return 0 on success, EAGAIN if would block, any other errors otherwise
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
 * Invocation of a update_messages command. The following is guaranteed:
 *   cmd->account WILL be non-NULL
 *   cmd->queue WILL be non-NULL
 *   cmd->attributes WILL be non-NULL, check individual attributes
 *   cmd->filters MAY be NULL, check individual filters
 *
 * @param ptr Pointer to backend context
 * @param cmd Command structure
 * @return 0 on success, EAGAIN if would block, any other errors otherwise
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
 * Invocation of a get_message command. The following is guaranteed:
 *   cmd->account WILL be non-NULL
 *   cmd->queue WILL be non-NULL
 *   cmd->message_id WILL be non-NULL
 *   cmd->filters MAY be NULL, check individual filters
 *
 * @param ptr Pointer to backend context
 * @param cmd Command structure
 * @return 0 on success, EAGAIN if would block, any other errors otherwise
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
 * Invocation of a delete_message command. The following is guaranteed:
 *   cmd->account WILL be non-NULL
 *   cmd->queue WILL be non-NULL
 *   cmd->message_id WILL be non-NULL
 *   cmd->filters MAY be NULL, check individual filters
 *
 * @param ptr Pointer to backend context
 * @param cmd Command structure
 * @return 0 on success, EAGAIN if would block, any other errors otherwise
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
 * Invocation of a update_message command. The following is guaranteed:
 *   cmd->account WILL be non-NULL
 *   cmd->queue WILL be non-NULL
 *   cmd->message_id WILL be non-NULL
 *   cmd->filters MAY be NULL, check individual filters
 *
 * @param ptr Pointer to backend context
 * @param cmd Command structure
 * @return 0 on success, EAGAIN if would block, any other errors otherwise
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
 * Invocation of a create command. The following is guaranteed:
 *   cmd->account WILL be non-NULL
 *   cmd->queue WILL be non-NULL
 *   cmd->message_id WILL be non-NULL
 *   cmd->body WILL be non-NULL
 *   cmd->body_size WILL be set to appropriate size
 *   cmd->attributes MAY be NULL, check individual attributes
 *
 * @param ptr Pointer to backend context
 * @param cmd Command structure
 * @return 0 on success, EAGAIN if would block, any other errors otherwise
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
 * This is the structure that's required to be exposed to the frontend
 * so that appropriate backend functions can be called.
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
