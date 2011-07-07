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
 * @brief Structures used internally
 */


#ifndef __BURROW_STRUCTS_H
#define __BURROW_STRUCTS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Existence public, internals not */
struct burrow_attributes_st
{
  int32_t ttl; /* -1: not set */
  int32_t hide; /* -1: not set */
  
  /* These may or may not be required for memory management, and
     are only a front-end concern. */
  burrow_st *burrow;
  burrow_attributes_st *next;
  burrow_attributes_st *prev;
};

/* Existence public, internals not */
struct burrow_filters_st
{
  burrow_tribool_t match_hidden; /* MAYBE -- not set */
  int32_t limit; /* -1, not set */
  char *marker;  /* NULL, not set */
  burrow_detail_t detail; /* BURROW_DETAIL_UNSET -- not set */
  int32_t wait; /* -1, not set */

  /* These may or may not be required for memory management, and
     are only a front-end concern. */
  burrow_st *burrow;
  burrow_attributes_st *next;
  burrow_attributes_st *prev;
};

/* Private */
struct burrow_command_st
{
  burrow_command_t command;
  const char *account;
  const char *queue;
  const char *message_id;
  const uint8_t *body;
  size_t body_size;
  const burrow_filters_st *filters;
  const burrow_attributes_st *attributes;
};

struct burrow_st {
  burrow_options_t options;
  burrow_flags_t flags;
  void *context;
  burrow_verbose_t verbose;
  burrow_state_t state;

  burrow_command_st cmd;
  
  burrow_message_fn *message_fn;
  burrow_queues_fn *queues_fn;
  burrow_accounts_fn *accounts_fn;
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

struct burrow_backend_functions_st {
  burrow_backend_create_fn *create;
  burrow_backend_free_fn *free;
  burrow_backend_size_fn *size;
  burrow_backend_clone_fn *clone;

  burrow_backend_set_option_fn *set_option;

  burrow_backend_cancel_fn *cancel;
  burrow_backend_process_fn *process;
  burrow_backend_event_raised_fn *event_raised;

  burrow_get_accounts_fn    *get_accounts;
  burrow_delete_accounts_fn *delete_accounts;

  burrow_get_queues_fn      *get_queues;
  burrow_delete_queues_fn   *delete_queues;

  burrow_get_messages_fn    *get_messages;
  burrow_update_messages_fn *update_messages;
  burrow_delete_messages_fn *delete_messages;

  burrow_get_message_fn     *get_message;
  burrow_update_message_fn  *update_message;
  burrow_delete_message_fn  *delete_message;
  burrow_create_message_fn  *create_message;
}; 

#ifdef __cplusplus
}
#endif

#endif /* __BURROW_STRUCTS_H */