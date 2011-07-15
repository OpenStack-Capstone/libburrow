/*
 * libburrow -- Burrow Client Library
 *
 * Copyright (C) 2011 Tony Wooster (twooster@gmail.com)
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in this directory for full text.
 */

/**
 * @file
 * @brief Public structures declarations
 */


#ifndef __BURROW_STRUCTS_LOCAL_H
#define __BURROW_STRUCTS_LOCAL_H

/* Private */

/* Private */
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

struct burrow_backend_functions_st {
  burrow_backend_create_fn *create;
  burrow_backend_destroy_fn *destroy;
  burrow_backend_size_fn *size;
  burrow_backend_clone_fn *clone;

  burrow_backend_set_option_fn *set_option;

  burrow_backend_cancel_fn *cancel;
  burrow_backend_process_fn *process;
  burrow_backend_event_raised_fn *event_raised;

  burrow_backend_command_fn *get_accounts;
  burrow_backend_command_fn *delete_accounts;

  burrow_backend_command_fn *get_queues;
  burrow_backend_command_fn *delete_queues;

  burrow_backend_command_fn *get_messages;
  burrow_backend_command_fn *update_messages;
  burrow_backend_command_fn *delete_messages;

  burrow_backend_command_fn *get_message;
  burrow_backend_command_fn *update_message;
  burrow_backend_command_fn *delete_message;
  burrow_backend_command_fn *create_message;
};

/* Public */
struct burrow_attributes_st
{
  burrow_attributes_set_t set;

  uint32_t ttl;
  uint32_t hide;
  
  /* These may or may not be required for memory management, and
     are only a front-end concern. */
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

  /* These may or may not be required for memory management, and
     are only a front-end concern. */
  burrow_st *burrow;
  burrow_filters_st *next;
  burrow_filters_st *prev;
};

struct burrow_st {
  burrow_options_t options;
  burrow_flags_t flags;
  void *context;
  burrow_verbose_t verbose;
  burrow_state_t state;

  burrow_command_st cmd;
  
  burrow_message_fn *message_fn;
  burrow_queue_fn *queue_fn;
  burrow_account_fn *account_fn;
  burrow_log_fn *log_fn;
  burrow_complete_fn *complete_fn;
  burrow_watch_fd_fn *watch_fd_fn;
  
  burrow_malloc_fn *malloc_fn;
  burrow_free_fn *free_fn;
  
  burrow_backend_functions_st *backend;
  void *backend_context;
  
  uint32_t watch_size;
  int32_t timeout;
  uint32_t pfds_size;
  struct pollfd *pfds;
  
  burrow_attributes_st *attributes_list;
  burrow_filters_st *filters_list;
};

#endif /* __BURROW_STRUCTS_LOCAL_H */