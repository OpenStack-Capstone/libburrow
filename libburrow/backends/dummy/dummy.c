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

#include <libburrow/burrow.h>
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
  uint8_t *body;
  ssize_t body_size;
  time_t ttl;
  time_t hide;
};

typedef struct burrow_backend_dummy_st burrow_backend_dummy_st;

/* Cleans up everything we've allocated */
static void dummy_free_internals(burrow_backend_dummy_st *dummy)
{
  /* free_fn can be assumed to accept NULL values without error */
  dummy->burrow->free_fn(dummy->burrow, dummy->account);
  dummy->burrow->free_fn(dummy->burrow, dummy->queue);
  dummy->burrow->free_fn(dummy->burrow, dummy->message_id);
  dummy->burrow->free_fn(dummy->burrow, dummy->body);
}

/* Calls the user's message callback with the specified detail level;
   also checks if the message has expired before sending it back,
   in case filtering was otherwise ignored */
static void message_with_detail(burrow_backend_dummy_st *dummy, burrow_detail_t detail)
{
  char *id = dummy->message_id;
  uint8_t *body = dummy->body;
  ssize_t body_size = dummy->body_size;
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
  attr.ttl = (int32_t)((int64_t)dummy->ttl - (int64_t)curtime);
  if (dummy->hide != 0 && dummy->hide < curtime) {
    attr.hide = (int32_t)((int64_t)dummy->hide - (int64_t)curtime);
  } else
    attr.hide = 0;
  
  switch(detail)
  {
  case BURROW_DETAIL_ID: /* only the id is filled out */
    burrow_log_debug(dummy->burrow, "message_with_detail: detail level id selected");
    body = NULL;
    body_size = -1;
    attrptr = NULL;
    break;
  case BURROW_DETAIL_BODY: /* only the body and body size are filled out */
    burrow_log_debug(dummy->burrow, "message_with_detail: detail level body selected");
    attrptr = NULL;
    break;
  case BURROW_DETAIL_ATTRIBUTES: /* the body isn't filled out */
  burrow_log_debug(dummy->burrow, "message_with_detail: detail level attributes selected");
    body = NULL;
    body_size = -1;
    break;
  case BURROW_DETAIL_ALL:
  case BURROW_DETAIL_UNSET:
  case BURROW_DETAIL_NONE: /* suppress compiler warning */
  default:
    burrow_log_debug(dummy->burrow, "message_with_detail: detail level all selected");
    break;
  }
  dummy->burrow->message_fn(dummy->burrow, id, body, body_size, attrptr);
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
  if (dummy->hide != 0 && dummy->hide < curtime && (!filters || filters->match_hidden != BURROW_TRUE))
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

static void burrow_backend_dummy_free(void *ptr)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  
  dummy_free_internals(dummy);
  
  if (dummy->selfallocated)
    dummy->burrow->free_fn(dummy->burrow, ptr);
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

static burrow_result_t burrow_backend_dummy_get_accounts(void *ptr, const burrow_filters_st *filters)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;

  if (!search_matches(dummy, NULL, NULL, NULL, filters))
    return BURROW_OK;

  /* Note that the second parameter is an array of char *s, so we have to
     get the address of our single account value for this to work */
  dummy->burrow->accounts_fn(dummy->burrow, &dummy->account, 1);
  return BURROW_OK;
}

static burrow_result_t burrow_backend_dummy_delete_accounts(void *ptr, const burrow_filters_st *filters)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  /* filters parameter unused here */
  (void) filters;
  
  dummy_free_internals(dummy);
  return BURROW_OK;
}

static burrow_result_t burrow_backend_dummy_get_queues(void *ptr, const char *account, const burrow_filters_st *filters)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  
  if (!search_matches(dummy, account, NULL, NULL, filters))
    return BURROW_OK;

  /* Note that the second parameter is an array of char *s, so we have to
     get the address of our single queue value for this to work */
  dummy->burrow->queues_fn(dummy->burrow, &dummy->queue, 1);
  return BURROW_OK;
}

static burrow_result_t burrow_backend_dummy_delete_queues(void *ptr, const char *account, const burrow_filters_st *filters)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;

  if (!search_matches(dummy, account, NULL, NULL, filters))
    return BURROW_OK;
  
  dummy_free_internals(dummy);
  return BURROW_OK;
}

static burrow_result_t burrow_backend_dummy_get_messages(void *ptr, const char *account, const char *queues, const burrow_filters_st *filters)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  /* get_messages default detail is DETAIL_ALL */
  burrow_detail_t detail = (filters && filters->detail != BURROW_DETAIL_UNSET ? filters->detail : BURROW_DETAIL_ALL);
  
  if (!search_matches(dummy, account, queues, NULL, filters))
    return BURROW_OK;
    
  message_with_detail(dummy, detail);
  return BURROW_OK;
}

static burrow_result_t burrow_backend_dummy_delete_messages(void *ptr, const char *account, const char *queues, const burrow_filters_st *filters)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;

  if (!search_matches(dummy, account, queues, NULL, filters))
    return BURROW_OK;

  dummy_free_internals(dummy);
  return BURROW_OK;
}

static burrow_result_t burrow_backend_dummy_update_messages(void *ptr, const char *account, const char *queues, const burrow_attributes_st *attributes, const burrow_filters_st *filters)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  /* update_messages default detail is DETAIL_ATTRIBUTES */
  burrow_detail_t detail = (filters && filters->detail != BURROW_DETAIL_UNSET ? filters->detail : BURROW_DETAIL_ATTRIBUTES);
  time_t curtime = time(NULL);

  if (!attributes)
    return BURROW_OK;
  
  if (!search_matches(dummy, account, queues, NULL, filters))
    return BURROW_OK;
    
  if (attributes->ttl > -1)
    dummy->ttl = curtime + attributes->ttl;

  if (attributes->hide > -1) {
    if (attributes->hide == 0)
      dummy->hide = 0;
    else
      dummy->hide = curtime + attributes->hide;
  }
    
  message_with_detail(dummy, detail);
  return BURROW_OK;
}

static burrow_result_t burrow_backend_dummy_get_message(void *ptr, const char *account, const char *queue, const char *id, const burrow_filters_st *filters)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  /* get_message default detail is DETAIL_ALL */
  burrow_detail_t detail = (filters && filters->detail != BURROW_DETAIL_UNSET ? filters->detail : BURROW_DETAIL_ALL);
  
  if (!search_matches(dummy, account, queue, id, NULL))
    return BURROW_OK;
    
  message_with_detail(dummy, detail);
  return BURROW_OK;
}

static burrow_result_t burrow_backend_dummy_delete_message(void *ptr, const char *account, const char *queue, const char *id, const burrow_filters_st *filters)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  /* filters parameter unused here */
  (void) filters;
  
  if (!search_matches(dummy, account, queue, id, NULL))
    return BURROW_OK;

  dummy_free_internals(dummy);
  return BURROW_OK;
}

static burrow_result_t burrow_backend_dummy_update_message(void *ptr, const char *account, const char *queue, const char *id, const burrow_attributes_st *attributes, const burrow_filters_st *filters)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  /* update_message default detail is DETAIL_ATTRIBUTES */
  burrow_detail_t detail = (filters && filters->detail != BURROW_DETAIL_UNSET ? filters->detail : BURROW_DETAIL_ATTRIBUTES);
  time_t curtime = time(NULL);

  if (!attributes)
    return BURROW_OK;
  
  if (!search_matches(dummy, account, queue, id, filters))
    return BURROW_OK;
    
  if (attributes->ttl > -1)
    dummy->ttl = curtime + attributes->ttl;

  if (attributes->hide > -1) {
    if (attributes->hide == 0)
      dummy->hide = 0;
    else
      dummy->hide = curtime + attributes->hide;
  }
    
  message_with_detail(dummy, detail);
  return BURROW_OK;
}

static burrow_result_t burrow_backend_dummy_create_message(void *ptr, const char *account, const char *queue, const char *id, const uint8_t *body, size_t body_size, const burrow_attributes_st *attributes)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;

  uint8_t *body_copy = NULL;
  char *account_copy = NULL;
  char *queue_copy = NULL;
  char *id_copy = NULL;

  time_t curtime = time(NULL);
  
  /* alloc body first: most likely malloc to fail */
  body_copy = dummy->burrow->malloc_fn(dummy->burrow, body_size);
  id_copy = dummy->burrow->malloc_fn(dummy->burrow, strlen(id) + 1);
  account_copy = dummy->burrow->malloc_fn(dummy->burrow, strlen(account) + 1);
  queue_copy = dummy->burrow->malloc_fn(dummy->burrow, strlen(queue) + 1);

  if (!body_copy || !id_copy || !account_copy || !queue_copy)
  {
    dummy->burrow->free_fn(dummy->burrow, body_copy);
    dummy->burrow->free_fn(dummy->burrow, id_copy);
    dummy->burrow->free_fn(dummy->burrow, account_copy);
    dummy->burrow->free_fn(dummy->burrow, queue_copy);
    burrow_error(dummy->burrow, BURROW_ERROR_MEMORY, "burrow_backend_dummy_create_message: malloc failed");
    return BURROW_ERROR_MEMORY;
  }

  dummy_free_internals(dummy);

  strcpy(account_copy, account);
  strcpy(queue_copy, queue);
  strcpy(id_copy, id);
  memcpy(body_copy, body, body_size * sizeof(uint8_t));

  dummy->account = account_copy;
  dummy->queue = queue_copy;
  dummy->message_id = id_copy;
  dummy->body = body_copy;
  dummy->body_size = body_size;

  if (attributes) {
    if (attributes->ttl > -1)
      dummy->ttl = curtime + attributes->ttl;

    if (attributes->hide > -1) {
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
  .free = &burrow_backend_dummy_free,
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
