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
 * @brief Internal / backend structures
 */


#ifndef __BURROW_STRUCTS_LOCAL_H
#define __BURROW_STRUCTS_LOCAL_H

#ifdef __cplusplus
extern "C" {
#endif

struct burrow_command_st
{
  burrow_command_t command;
  burrow_backend_command_fn *command_fn;
  const char *account;
  const char *queue;
  const char *message_id;
  const void *body;
  size_t body_size;
  const burrow_filters_st *filters;
  const burrow_attributes_st *attributes;
};

/**
 * Backend interface.
 */
struct burrow_backend_functions_st
{
  /**
   * Creates a backend.
   *
   * @param ptr NULL, meaning "please allocate for me", or pointer to
   *            memory allocated after calling size function.
   * @param burrow Burrow object to be associated with this backend.
   * @return pointer to initialized memory
   */
  burrow_backend_create_fn *create;
  
  /**
   * Deallocates/destroys a backend.
   *
   * @param ptr pointer to a backend struct
   * @return pointer to initialized memory
   */
  burrow_backend_destroy_fn *destroy;

  /**
   * Returns the size of a base backend structure so that the user
   * or frontend can allocate space appropriately.
   *
   * @return size of a base backend structure
   */
  burrow_backend_size_fn *size;

  /**
   * Generic set_option string, string function.
   *
   * @param ptr Pointer to backend struct
   * @param key Key name, guaranteed to be non-NULL
   * @param value Value, may be NULL
   * @return 0 on success, EINVAL if key or value is invalid
   */
  burrow_backend_set_option_fn *set_option;

  /**
   * Generic set_option_int -> string, int function.
   *
   * @param ptr Pointer to backend struct
   * @param key Key name, guaranteed to be non-NULL
   * @param value Value
   * @return 0 on success, EINVAL if key or value is invalid
   */
  burrow_backend_set_option_int_fn *set_option_int;

  /**
   * Called when the user requests all pending activity to be canceled.
   * The backend should be in a good but idle state after this call.
   * All previously requested watch-fds will no longer be watched after
   * this call.
   *
   * @param ptr pointer to backend struct
   */
  burrow_backend_cancel_fn *cancel;
  
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
  burrow_backend_process_fn *process;
  
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
  burrow_backend_event_raised_fn *event_raised;

  /**
   * Called when the user wishes to retrieve a list of accounts.
   * See burrow_callback_account().
   *
   * Incoming, the following is guaranteed:
   *   cmd->filters MAY be NULL, check individual filters
   *
   * @param ptr Pointer to backend context
   * @param cmd Command structure
   * @return 0 on success, EAGAIN if would block, any other errors otherwise
   */
  burrow_backend_command_fn *get_accounts;
  
  /**
   * Called when the user wishes to delete a list of accounts.
   * See burrow_callback_account().
   *
   * Incoming, the following is guaranteed:
   *   cmd->filters MAY be NULL, check individual filters
   *
   * @param ptr Pointer to backend context
   * @param cmd Command structure
   * @return 0 on success, EAGAIN if would block, any other errors otherwise
   */
  burrow_backend_command_fn *delete_accounts;

  /**
   * Called when the user wishes to retrieve a list of queues associated
   * with a given account. See burrow_callback_queue().
   *
   * Incoming, the following is guaranteed:
   *   cmd->account WILL be non-NULL
   *   cmd->filters MAY be NULL, check individual filters
   *
   * @param ptr Pointer to backend context
   * @param cmd Command structure
   * @return 0 on success, EAGAIN if would block, any other errors otherwise
   */
  burrow_backend_command_fn *get_queues;

  /**
   * Called when the user wishes to delete a list of queues associated
   * with a given account. See burrow_callback_queue().
   *
   * Incoming, the following is guaranteed:
   *   cmd->account WILL be non-NULL
   *   cmd->filters MAY be NULL, check individual filters
   *
   * @param ptr Pointer to backend context
   * @param cmd Command structure
   * @return 0 on success, EAGAIN if would block, any other errors otherwise
   */
  burrow_backend_command_fn *delete_queues;

  /**
   * Called when the user wishes to retrieve a list of messages associated
   * with a given account and queue. See burrow_callback_message().
   *
   * Incoming, the following is guaranteed:
   *   cmd->account WILL be non-NULL
   *   cmd->queue WILL be non-NULL
   *   cmd->filters MAY be NULL, check individual filters
   *
   * @param ptr Pointer to backend context
   * @param cmd Command structure
   * @return 0 on success, EAGAIN if would block, any other errors otherwise
   */
  burrow_backend_command_fn *get_messages;
  
  /**
   * Called when the user wishes to delete a list of messages associated
   * with a given account and queue. See burrow_callback_message().
   *
   * Incoming, the following is guaranteed:
   *   cmd->account WILL be non-NULL
   *   cmd->queue WILL be non-NULL
   *   cmd->filters MAY be NULL, check individual filters
   *
   * @param ptr Pointer to backend context
   * @param cmd Command structure
   * @return 0 on success, EAGAIN if would block, any other errors otherwise
   */
  burrow_backend_command_fn *delete_messages;


  /**
   * Called when the user wishes to update the attributes of a list of
   * messages associated with a given account and queue.
   * See burrow_callback_message().
   *
   * Incoming, the following is guaranteed:
   *   cmd->account WILL be non-NULL
   *   cmd->queue WILL be non-NULL
   *   cmd->attributes WILL be non-NULL, check individual attributes
   *   cmd->filters MAY be NULL, check individual filters
   *
   * @param ptr Pointer to backend context
   * @param cmd Command structure
   * @return 0 on success, EAGAIN if would block, any other errors otherwise
   */
  burrow_backend_command_fn *update_messages;

  /**
   * Called when the user wishes to update the attributes of a message
   * associated with a given account and queue. See burrow_callback_message().
   *
   * Incoming, the following is guaranteed:
   *   cmd->account WILL be non-NULL
   *   cmd->queue WILL be non-NULL
   *   cmd->message_id WILL be non-NULL
   *   cmd->filters MAY be NULL, check individual filters
   *
   * @param ptr Pointer to backend context
   * @param cmd Command structure
   * @return 0 on success, EAGAIN if would block, any other errors otherwise
   */
  burrow_backend_command_fn *get_message;

  /**
   * Called when the user wishes to delete a specific message in a
   * given account and queue. See burrow_callback_message().
   *
   * Incoming, the following is guaranteed:
   *   cmd->account WILL be non-NULL
   *   cmd->queue WILL be non-NULL
   *   cmd->message_id WILL be non-NULL
   *   cmd->filters MAY be NULL, check individual filters
   *
   * @param ptr Pointer to backend context
   * @param cmd Command structure
   * @return 0 on success, EAGAIN if would block, any other errors otherwise
   */
  burrow_backend_command_fn *delete_message;

  /**
   * Called when the user wishes to update the attributes of a specific message
   * in a given account and queue. See burrow_callback_message().
   *
   * Incoming, the following is guaranteed:
   *   cmd->account WILL be non-NULL
   *   cmd->queue WILL be non-NULL
   *   cmd->message_id WILL be non-NULL
   *   cmd->filters MAY be NULL, check individual filters
   *
   * @param ptr Pointer to backend context
   * @param cmd Command structure
   * @return 0 on success, EAGAIN if would block, any other errors otherwise
   */
  burrow_backend_command_fn *update_message;

  /**
   * Called when the user wishes to create/overwrite a specific message
   * in a given account and queue, optionally with the given attributes.
   * See burrow_callback_message().
   *
   * Incoming, the following is guaranteed:
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
  burrow_backend_command_fn *create_message;
};

/* Public */
struct burrow_attributes_st
{
  burrow_attributes_set_t set;

  uint32_t ttl;
  uint32_t hide;
  
  /* These are only a front-end concern. */
  burrow_st *burrow;
  burrow_attributes_st *next;
  burrow_attributes_st *prev;
};

struct burrow_filters_st
{ 
  burrow_filters_set_t set;
  
  bool match_hidden;
  uint32_t limit;
  const char *marker;
  burrow_detail_t detail;
  uint32_t wait;

  /* These are only a front-end concern. */
  burrow_st *burrow;
  burrow_filters_st *next;
  burrow_filters_st *prev;
};

struct burrow_st
{
  /* Frontend state */
  burrow_options_t options;
  burrow_flags_t flags;
  burrow_verbose_t verbose;
  burrow_state_t state;

  /* Current command structure */
  burrow_command_st cmd;
  
  /* User callbacks & context */
  void *context;

  burrow_message_fn *message_fn;
  burrow_queue_fn *queue_fn;
  burrow_account_fn *account_fn;
  burrow_log_fn *log_fn;
  burrow_complete_fn *complete_fn;
  burrow_watch_fd_fn *watch_fd_fn;
  
  burrow_malloc_fn *malloc_fn;
  burrow_free_fn *free_fn;
  
  /* Backend structure and context */
  burrow_backend_functions_st *backend;
  void *backend_context;
  
  /* Built-in FD polling */
  uint32_t watch_size;
  int32_t timeout;
  uint32_t pfds_size;
  struct pollfd *pfds;
  
  /* Managed objects */
  burrow_attributes_st *attributes_list;
  burrow_filters_st *filters_list;
};

#ifdef __cplusplus
}
#endif

#endif /* __BURROW_STRUCTS_LOCAL_H */