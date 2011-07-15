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
 * @brief Dummy backend implementation
 */

#include <libburrow/common.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Backend state struct */
struct burrow_backend_dummy_st
{
  int selfallocated;
  burrow_st *burrow;

  char *account;
  char *queue;
  char *message_id;
  void *body;
  size_t body_size;
  time_t ttl;
  time_t hide;
};

typedef struct burrow_backend_dummy_st burrow_backend_dummy_st;

/* Cleans up everything we've allocated */
static void dummy_free_internals(burrow_backend_dummy_st *dummy)
{
  /* free_fn can be assumed to accept NULL values without error */
  burrow_free(dummy->burrow, dummy->account);
  burrow_free(dummy->burrow, dummy->queue);
  burrow_free(dummy->burrow, dummy->message_id);
  burrow_free(dummy->burrow, dummy->body);
  dummy->account = NULL;
  dummy->queue = NULL;
  dummy->message_id = NULL;
  dummy->body = NULL;
  dummy->body_size = 0;
}

/* Calls the user's message callback with the specified detail level;
   also checks if the message has expired before sending it back,
   in case filtering was otherwise ignored */
static void message_with_detail(burrow_backend_dummy_st *dummy, burrow_detail_t detail)
{
  char *id = dummy->message_id;
  uint8_t *body = dummy->body;
  size_t body_size = dummy->body_size;
  burrow_attributes_st attr;
  burrow_attributes_st *attrptr = &attr;
  time_t curtime;

  if (detail == BURROW_DETAIL_NONE)
    return; /* don't send the message! */

  curtime = time(NULL);

  if (dummy->ttl < curtime)
    return; /* message has expired */
  
  /* Note that .ttl and .hide fields are represented to the user as
     relative time deltas, not absolute values when the message will
     expire/go visible. Internally, we use absolute time values, so
     we have to correct for this. */

  /* The differences should be within 32 bits, but time_t may not be signed, so we do this: */
  attr.ttl = (uint32_t)(dummy->ttl - curtime);
  if (dummy->hide > 0 && dummy->hide > curtime) {
    attr.hide = (uint32_t)(dummy->hide - curtime);
  } else {
    attr.hide = 0;
  }
  attr.set |= BURROW_ATTRIBUTES_TTL | BURROW_ATTRIBUTES_HIDE;
  
  switch(detail)
  {
  case BURROW_DETAIL_ID: /* only the id is filled out */
    burrow_log_debug(dummy->burrow, "message_with_detail: detail level id selected");
    body = NULL;
    body_size = 0;
    attrptr = NULL;
    break;
  case BURROW_DETAIL_BODY: /* only the body and body size are filled out */
    burrow_log_debug(dummy->burrow, "message_with_detail: detail level body selected");
    attrptr = NULL;
    break;
  case BURROW_DETAIL_ATTRIBUTES: /* the body isn't filled out */
  burrow_log_debug(dummy->burrow, "message_with_detail: detail level attributes selected");
    body = NULL;
    body_size = 0;
    break;
  case BURROW_DETAIL_ALL:
  case BURROW_DETAIL_NONE: /* suppress compiler warning */
  default:
    burrow_log_debug(dummy->burrow, "message_with_detail: detail level all selected");
    break;
  }
  burrow_callback_message(dummy->burrow, id, body, body_size, attrptr);
}

/* General-purpose function to see if the user's query matches the
   account, queue and message_id, taking into account ttl, hide and filters */
/* Any null parameters will be considered to be wildcards, except
   for filters, which will be assumed to be limit=infinite, match_hidden = false */
/* Returns 1 if user query matches, 0 otherwise */
static int search_matches(burrow_backend_dummy_st *dummy, const char *account, const char *queue, const char *message, const burrow_filters_st *filters)
{
  time_t curtime;
  
  /* See if there's even a message still there... */
  if (!dummy->account)
    return 0;
  
  curtime = time(NULL);
  
  if (dummy->ttl < curtime)
    return 0;
  if (!message && dummy->hide > 0 && dummy->hide > curtime && (!filters || filters->match_hidden != true))
    return 0;
  
  if (!account || !queue || !message
      || (filters && filters->limit == 0))
    return 1; /* always match */
  
  if ((account && !strcmp(account, dummy->account))
      && (queue && !strcmp(queue, dummy->queue))
      && (message && !strcmp(message, dummy->message_id)))
      return 1;

  return 0;
}

static void *burrow_backend_dummy_create(void *ptr, burrow_st *burrow)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st*)ptr;
  
  if (dummy == NULL) {
    dummy = malloc(sizeof(burrow_backend_dummy_st));
    if (!dummy)
      return NULL;
    dummy->selfallocated = 1;
  } else
    dummy->selfallocated = 0;
  
  dummy->burrow = burrow;

  dummy->account = NULL;
  dummy->queue = NULL;
  dummy->message_id = NULL;
  dummy->body = NULL;
  dummy->body_size = 0;
  dummy->ttl = 0;
  dummy->hide = 0;
  
  return dummy;
}

static void burrow_backend_dummy_destroy(void *ptr)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  
  dummy_free_internals(dummy);
  
  if (dummy->selfallocated)
    burrow_free(dummy->burrow, dummy);
}

static size_t burrow_backend_dummy_size(void)
{
  return sizeof(burrow_backend_dummy_st);
}

static void *burrow_backend_dummy_clone(void *dst, void *src)
{
  (void) dst;
  (void) src;
  
  /* TODO: unimplemented. semantics yet unclear */
  return NULL;
}

static burrow_result_t burrow_backend_dummy_set_option(void *ptr, const char *key, const char *value)
{
  (void) ptr;
  (void) key;
  (void) value;
  return BURROW_ERROR_UNSUPPORTED;
}

static void burrow_backend_dummy_cancel(void *ptr)
{
  (void) ptr;
}

static burrow_result_t burrow_backend_dummy_process(void *ptr)
{
  (void) ptr;
  return BURROW_OK;
}


static burrow_result_t burrow_backend_dummy_event_raised(void *ptr, int fd, burrow_ioevent_t event)
{
  (void) ptr;
  (void) fd;
  (void) event;
  
  return BURROW_OK;
}

static burrow_result_t burrow_backend_dummy_get_accounts(void *ptr, const burrow_command_st *cmd)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;

  if (!search_matches(dummy, NULL, NULL, NULL, cmd->filters))
    return BURROW_OK;

  burrow_callback_account(dummy->burrow, dummy->account);
  return BURROW_OK;
}

static burrow_result_t burrow_backend_dummy_delete_accounts(void *ptr, const burrow_command_st *cmd)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  (void) cmd;
  
  dummy_free_internals(dummy);
  return BURROW_OK;
}

static burrow_result_t burrow_backend_dummy_get_queues(void *ptr, const burrow_command_st *cmd)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  
  if (!search_matches(dummy, cmd->account, NULL, NULL, cmd->filters))
    return BURROW_OK;

  burrow_callback_queue(dummy->burrow, dummy->queue);
  return BURROW_OK;
}

static burrow_result_t burrow_backend_dummy_delete_queues(void *ptr, const burrow_command_st *cmd)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;

  if (!search_matches(dummy, cmd->account, NULL, NULL, cmd->filters))
    return BURROW_OK;
  
  dummy_free_internals(dummy);
  return BURROW_OK;
}

static burrow_result_t burrow_backend_dummy_get_messages(void *ptr, const burrow_command_st *cmd)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  /* get_messages default detail is DETAIL_ALL */
  burrow_detail_t detail = (cmd->filters && (cmd->filters->set & BURROW_FILTERS_DETAIL) ? cmd->filters->detail : BURROW_DETAIL_ALL);
  
  if (!search_matches(dummy, cmd->account, cmd->queue, NULL, cmd->filters))
    return BURROW_OK;
    
  message_with_detail(dummy, detail);
  return BURROW_OK;
}

static burrow_result_t burrow_backend_dummy_delete_messages(void *ptr, const burrow_command_st *cmd)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;

  if (!search_matches(dummy, cmd->account, cmd->queue, NULL, cmd->filters))
    return BURROW_OK;

  dummy_free_internals(dummy);
  return BURROW_OK;
}

static burrow_result_t burrow_backend_dummy_update_messages(void *ptr, const burrow_command_st *cmd)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  /* update_messages default detail is DETAIL_ATTRIBUTES */
  burrow_detail_t detail = (cmd->filters && (cmd->filters->set & BURROW_FILTERS_DETAIL) ? cmd->filters->detail : BURROW_DETAIL_ATTRIBUTES);
  time_t curtime = time(NULL);

  if (!cmd->attributes)
    return BURROW_OK;
  
  if (!search_matches(dummy, cmd->account, cmd->queue, NULL, cmd->filters))
    return BURROW_OK;
    
  if (cmd->attributes->set & BURROW_ATTRIBUTES_TTL)
    dummy->ttl = curtime + cmd->attributes->ttl;

  if (cmd->attributes->set & BURROW_ATTRIBUTES_HIDE) {
    if (cmd->attributes->hide == 0)
      dummy->hide = 0;
    else
      dummy->hide = curtime + cmd->attributes->hide;
  }
    
  message_with_detail(dummy, detail);
  return BURROW_OK;
}

static burrow_result_t burrow_backend_dummy_get_message(void *ptr, const burrow_command_st *cmd)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  /* get_message default detail is DETAIL_ALL */
  burrow_detail_t detail = (cmd->filters && (cmd->filters->set & BURROW_FILTERS_DETAIL) ? cmd->filters->detail : BURROW_DETAIL_ALL);
  
  if (!search_matches(dummy, cmd->account, cmd->queue, cmd->message_id, NULL))
    return BURROW_OK;
    
  message_with_detail(dummy, detail);
  return BURROW_OK;
}

static burrow_result_t burrow_backend_dummy_delete_message(void *ptr, const burrow_command_st *cmd)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  
  if (!search_matches(dummy, cmd->account, cmd->queue, cmd->message_id, NULL))
    return BURROW_OK;

  dummy_free_internals(dummy);
  return BURROW_OK;
}

static burrow_result_t burrow_backend_dummy_update_message(void *ptr, const burrow_command_st *cmd)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  /* update_message default detail is DETAIL_ATTRIBUTES */
  burrow_detail_t detail = (cmd->filters && (cmd->filters->set & BURROW_FILTERS_DETAIL) ? cmd->filters->detail : BURROW_DETAIL_ATTRIBUTES);
  time_t curtime = time(NULL);

  if (!cmd->attributes)
    return BURROW_OK;
  
  if (!search_matches(dummy, cmd->account, cmd->queue, cmd->message_id, cmd->filters))
    return BURROW_OK;
    
  if (cmd->attributes->set & BURROW_ATTRIBUTES_TTL)
    dummy->ttl = curtime + cmd->attributes->ttl;

  if (cmd->attributes->set & BURROW_ATTRIBUTES_HIDE) {
    if (cmd->attributes->hide == 0)
      dummy->hide = 0;
    else
      dummy->hide = curtime + cmd->attributes->hide;
  }
    
  message_with_detail(dummy, detail);
  return BURROW_OK;
}

static burrow_result_t burrow_backend_dummy_create_message(void *ptr, const burrow_command_st *cmd)
{
  const char *account = cmd->account;
  const char *queue = cmd->queue;
  const char *id = cmd->message_id;
  const void *body = cmd->body;
  size_t body_size = cmd->body_size;
  const burrow_attributes_st *attributes = cmd->attributes;

  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;

  void *body_copy = NULL;
  char *account_copy = NULL;
  char *queue_copy = NULL;
  char *id_copy = NULL;

  time_t curtime = time(NULL);
  
  /* alloc body first: most likely malloc to fail */
  body_copy = burrow_malloc(dummy->burrow, body_size);
  id_copy = burrow_malloc(dummy->burrow, strlen(id) + 1);
  account_copy = burrow_malloc(dummy->burrow, strlen(account) + 1);
  queue_copy = burrow_malloc(dummy->burrow, strlen(queue) + 1);

  if (!body_copy || !id_copy || !account_copy || !queue_copy)
  {
    burrow_free(dummy->burrow, body_copy);
    burrow_free(dummy->burrow, id_copy);
    burrow_free(dummy->burrow, account_copy);
    burrow_free(dummy->burrow, queue_copy);
    burrow_error(dummy->burrow, BURROW_ERROR_MEMORY, "burrow_backend_dummy_create_message: malloc failed");
    return BURROW_ERROR_MEMORY;
  }

  dummy_free_internals(dummy);

  strcpy(account_copy, account);
  strcpy(queue_copy, queue);
  strcpy(id_copy, id);
  memcpy(body_copy, body, body_size);

  dummy->account = account_copy;
  dummy->queue = queue_copy;
  dummy->message_id = id_copy;
  dummy->body = body_copy;
  dummy->body_size = body_size;

  if (attributes) {
    if (attributes->set & BURROW_ATTRIBUTES_TTL)
      dummy->ttl = curtime + attributes->ttl;

    if (attributes->set & BURROW_ATTRIBUTES_HIDE) {
      if (attributes->hide == 0)
        dummy->hide = 0;
      else
        dummy->hide = curtime + attributes->hide;
    }
  } else {
    dummy->hide = 0;
    dummy->ttl = curtime + 60 * 5; /* five minutes */
  }
    
  return BURROW_OK;
}

/* This is the structure that the frontend will use to access our
   internal functions. This is filled out in C99 style, which I think
   is acceptable. We can, however, default to C90 style if req'd. */
burrow_backend_functions_st burrow_backend_dummy_functions = {
  .create = &burrow_backend_dummy_create,
  .destroy = &burrow_backend_dummy_destroy,
  .size = &burrow_backend_dummy_size,
  .clone = &burrow_backend_dummy_clone,

  .cancel = &burrow_backend_dummy_cancel,
  .set_option = &burrow_backend_dummy_set_option,

  .event_raised = &burrow_backend_dummy_event_raised,

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

  .process = &burrow_backend_dummy_process,
};
