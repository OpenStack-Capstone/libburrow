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
 * @brief Burrow user/frontend functions
 */


#include "common.h"
#include <stdio.h>

/* Functions visible to the backend: */

static void burrow_default_watch_fd_fn(burrow_st *burrow, int fd, burrow_ioevent_t events)
{
  struct pollfd *pfd;
  uint32_t count;

  count = burrow->watch_size + 1;

  if (burrow->pfds_size < count) {
    pfd = realloc(burrow->pfds, count * sizeof(struct pollfd));
    if (!pfd) {
      /* raise error */
      return;
    }
    burrow->pfds = pfd;
    burrow->pfds_size = count;
  }

  burrow->watch_size = count;

  pfd = &burrow->pfds[count-1];
  pfd->fd = fd;
  pfd->events = 0;
  if (events & BURROW_IOEVENT_READ)
    pfd->events |= POLLIN;
  if (events & BURROW_IOEVENT_WRITE)
    pfd->events |= POLLOUT;
}

static burrow_result_t burrow_start_command(burrow_st *burrow)
{
  void *context = burrow->backend_context;
  burrow_command_st *cmd = &burrow->cmd;
  
  switch(cmd->command) {
    case BURROW_CMD_GET_ACCOUNTS:
      return burrow->backend->get_accounts(context, cmd->filters);
      
    case BURROW_CMD_DELETE_ACCOUNTS:
      return burrow->backend->delete_accounts(context, cmd->filters);

    case BURROW_CMD_GET_QUEUES:
      return burrow->backend->get_queues(context, cmd->account, cmd->filters);

    case BURROW_CMD_DELETE_QUEUES:
      return burrow->backend->delete_queues(context, cmd->account, cmd->filters);

    case BURROW_CMD_GET_MESSAGES:
      return burrow->backend->get_messages(context, cmd->account, cmd->queue, cmd->filters);
    
    case BURROW_CMD_UPDATE_MESSAGES:
      return burrow->backend->update_messages(context, cmd->account, cmd->queue, cmd->attributes, cmd->filters);
    
    case BURROW_CMD_DELETE_MESSAGES:
      return burrow->backend->delete_messages(context, cmd->account, cmd->queue, cmd->filters);

    case BURROW_CMD_GET_MESSAGE:
      return burrow->backend->get_message(context, cmd->account, cmd->queue, cmd->message_id, cmd->filters);
    
    case BURROW_CMD_UPDATE_MESSAGE:
      return burrow->backend->update_message(context, cmd->account, cmd->queue, cmd->message_id, cmd->attributes, cmd->filters);
    
    case BURROW_CMD_DELETE_MESSAGE:
      return burrow->backend->delete_message(context, cmd->account, cmd->queue, cmd->message_id, cmd->filters);

    case BURROW_CMD_CREATE_MESSAGE:
      return burrow->backend->create_message(context, cmd->account, cmd->queue, cmd->message_id, cmd->body, cmd->body_size, cmd->attributes);

    case BURROW_CMD_NONE:
    default:
      /* log error */
      return BURROW_ERROR_UNSUPPORTED;
  }
}

static void burrow_poll_fds(burrow_st *burrow)
{
  int count;
  uint32_t watch_size;
  struct pollfd *pfd, *last_pfd;
  
  if (burrow->watch_size == 0) /* nothing to watch */
    return;

  count = poll(burrow->pfds, burrow->watch_size, burrow->timeout);
  if (count == -1) {
    /* TODO: something sensible here, handling errno */
    return;
  }
  if (count == 0) {
    /* Timeout has occurred */
    /* TODO: something sensible here */
    /* maybe cancel the current command? */
    return;
  }
  pfd = burrow->pfds;
  
  watch_size = burrow->watch_size;
  while(count) {
    if (pfd->revents) { /* Found a live event */
      
      /* Dispatch it: */
      burrow_ioevent_t event = BURROW_IOEVENT_NONE;      
      if (pfd->revents & POLLIN)
        event |= BURROW_IOEVENT_READ;
      if (pfd->revents & POLLOUT)
        event |= BURROW_IOEVENT_WRITE;
      burrow_event_raised(burrow, pfd->fd, event);

      /* And copy the last pfd to this location */
      count--;
      watch_size--;
      last_pfd = &burrow->pfds[watch_size - 1];
      
      /* But not if we're at the last pfd entry */
      if (last_pfd > pfd) {
        pfd->fd = last_pfd->fd;
        pfd->events = last_pfd->events;
        pfd->revents = last_pfd->revents;
      }
      /* Note that we don't increment pfd here, because this location
         now has new data */
    }
    else
      pfd++;
  }
  burrow->watch_size = watch_size;
  return;
}

burrow_result_t burrow_process(burrow_st *burrow)
{
  burrow_result_t result = BURROW_OK_WAITING;

  if (burrow->flags & BURROW_FLAG_PROCESSING) /* prevent recursion */
    return BURROW_OK_WAITING; /* parent process loop will pick it up */

  burrow->flags |= BURROW_FLAG_PROCESSING;

  while (burrow->state != BURROW_STATE_IDLE) {
    switch(burrow->state) {
    
    case BURROW_STATE_START: /* command is initialized, but hasn't kicked off */
      result = burrow_start_command(burrow);
      if (result == BURROW_OK_WAITING)
        burrow->state = BURROW_STATE_WAITING;
      else /* could be error or OK */
        burrow->state = BURROW_STATE_FINISH;
      break;

    case BURROW_STATE_READY: /* io events have made the backend ready */
      result = burrow->backend->process(burrow->backend_context);
      if (result == BURROW_OK_WAITING)
        burrow->state = BURROW_STATE_WAITING;
      else /* could be error or OK */
        burrow->state = BURROW_STATE_FINISH;
      break;

    case BURROW_STATE_WAITING: /* backend is blocking on io */
      if (burrow->watch_size == 0)
        return BURROW_OK_WAITING; /* waiting is performed by the client */
      
      /* TODO: what if this returns for timeout or error? */
      burrow_poll_fds(burrow); /* this should unblock the io */
      break;

    case BURROW_STATE_FINISH: /* backend is done */
      burrow->state = BURROW_STATE_IDLE; /* we now accept new commands */
      if (burrow->complete_fn)
        burrow->complete_fn(burrow); /* could update burrow state by calling a command again */
      break;
    
    case BURROW_STATE_IDLE: /* suppress compiler warning */
    default:
      /* log error */
      break;
    }
  }
  
  burrow->flags ^= BURROW_FLAG_PROCESSING;
  return result;
}

burrow_result_t burrow_event_raised(burrow_st *burrow, int fd, burrow_ioevent_t event)
{
  burrow_result_t result;
  
  if (!burrow->backend->event_raised) {
    /* log error */; 
    return BURROW_ERROR_UNSUPPORTED;
  }

  if (burrow->state != BURROW_STATE_WAITING)
    (void)0/* log warning */;
        
   result = burrow->backend->event_raised(burrow->backend, fd, event);
  
  if (result == BURROW_OK) {
    burrow->state = BURROW_STATE_READY;
    if (burrow->options & BURROW_OPT_AUTOPROCESS)
      return burrow_process(burrow);
  }
  return result;
}

void burrow_cancel(burrow_st *burrow)
{
  if (burrow->state == BURROW_STATE_IDLE)
    return;

  burrow->watch_size = 0;
  if (burrow->backend->cancel)
    burrow->backend->cancel(burrow->backend_context);
  
  burrow->cmd.command = BURROW_CMD_NONE;
}

/* Burrow Object Functions */
static void burrow_default_message_fn(burrow_st *burrow,
                                      const char *message_id,
                                      const uint8_t *body,
                                      ssize_t body_size,
                                      const burrow_attributes_st *attributes)
{
  (void) burrow;
  (void) message_id;
  (void) body;
  (void) body_size;
  (void) attributes;
  burrow_log_info(burrow, "burrow_default_message_fn: called, msgid: '%s', body size %d", message_id, body_size);
}

static void burrow_default_queues_fn(burrow_st *burrow, char **queues, size_t size)
{
  (void) burrow;
  (void) queues;
  (void) size;
  burrow_log_info(burrow, "burrow_default_queues_fn: called, %u queues", size);
}

static void burrow_default_accounts_fn(burrow_st *burrow, char **accounts, size_t size)
{
  (void) burrow;
  (void) accounts;
  (void) size;
  burrow_log_info(burrow, "burrow_default_accounts_fn: called, %u accounts", size);
}

static void burrow_default_complete_fn(burrow_st *burrow)
{
  (void) burrow;
  burrow_log_info(burrow, "burrow_default_complete_fn: called");
}

static void *burrow_default_malloc_fn(burrow_st *burrow, size_t size)
{
  (void) burrow;
  return malloc(size);
}

static void burrow_default_free_fn(burrow_st *burrow, void *ptr)
{
  (void) burrow;
  free(ptr);
}

const char *_error_strings[] = {
  "***NONE***", /* BURROW_VERBOSE_NONE, */
  "FATAL", /* BURROW_VERBOSE_FATAL, */
  "ERROR", /* BURROW_VERBOSE_ERROR, */
  "WARN", /* BURROW_VERBOSE_WARN, */
  "INFO", /* BURROW_VERBOSE_INFO, */
  "DEBUG", /* BURROW_VERBOSE_DEBUG, */
  "***MAX***", /* BURROW_VERBOSE_MAX */  
};

static void burrow_default_log_fn(burrow_st *burrow, burrow_verbose_t verbose, const char *msg)
{
  (void) burrow;
  printf("libburrow(%s): %s\n", _error_strings[verbose], msg);
}

burrow_st *burrow_create(burrow_st *burrow, const char *backend)
{
  burrow_backend_functions_st *backend_fns;
  
  backend_fns = burrow_backend_load_functions(backend);
  if (!backend_fns)
    return NULL;
  
  if (!burrow) {
    /* We allocate to include the backend just after the base
       burrow struct */
    
    burrow = malloc(sizeof(burrow_st) + backend_fns->size());
    if (!burrow)
      return NULL;
    burrow->flags = BURROW_FLAG_SELFALLOCATED;
  } else {
    burrow->flags = 0;
  }
  
  burrow->options = 0;
  burrow->verbose = BURROW_VERBOSE_DEFAULT;
  burrow->state = BURROW_STATE_IDLE;
  burrow->cmd.command = BURROW_CMD_NONE;
  burrow->context = NULL;
  burrow->backend_context = backend_fns->create((void *)(burrow + 1), burrow);
  burrow->backend = backend_fns;

  burrow->malloc_fn   = &burrow_default_malloc_fn;
  burrow->free_fn     = &burrow_default_free_fn;
  
  burrow->message_fn  = &burrow_default_message_fn;
  burrow->queues_fn   = &burrow_default_queues_fn;
  burrow->accounts_fn = &burrow_default_accounts_fn;
  burrow->log_fn      = &burrow_default_log_fn;
  burrow->complete_fn = &burrow_default_complete_fn;
  burrow->watch_fd_fn = &burrow_default_watch_fd_fn;
  
  burrow->pfds = NULL;
  burrow->pfds_size = 0;
  burrow->watch_size = 0;
  burrow->timeout = 60;
  
  burrow->attributes_list = NULL;
  burrow->filters_list = NULL;

  return burrow;
}

void burrow_free(burrow_st *burrow)
{
  /* TODO: more */
  burrow->backend->free((void*)(burrow+1));
  if (burrow->pfds != NULL) {
    burrow->free_fn(burrow, burrow->pfds);
    burrow->pfds = NULL;
  }
 
  burrow_log_info(burrow, "burrow_free: attributes list %c= NULL", (burrow->attributes_list == NULL ? '=' : '!')); 
  while (burrow->attributes_list != NULL)
    burrow_attributes_free(burrow->attributes_list);

  while (burrow->filters_list != NULL)
    burrow_filters_free(burrow->filters_list);

  if (burrow->flags & BURROW_FLAG_SELFALLOCATED) {
    free(burrow);
  }
}

ssize_t burrow_size(const char *backend)
{
  burrow_backend_functions_st *backend_fns;
  
  backend_fns = burrow_backend_load_functions(backend);
  if (!backend_fns)
    return -1;
    
  return (ssize_t)(sizeof(burrow_st) + backend_fns->size());
}

/**
 * Clones a burrow object in its entirety. Any ongoing commands/state will not
 * be present in the cloned object. If no destination is specified, one will
 * be allocated using the source's malloc function.
 *
 * If dest is not NULL, it must be of sufficient size to support the incoming
 * source structure. Otherwise, behavior is undefined.
 *
 * @param dest Destination burrow objec; can be NULL
 * @param src Source burrow object; must not be NULL
 * @return The cloned burrow object, or NULL on error
 */
burrow_st *burrow_clone(burrow_st *dest, burrow_st *src)
{
  /* this is tricky.... TODO: think this through w/r/t appended backends! */
  (void) dest;
  (void) src;
  return NULL;
}
 
void burrow_set_context(burrow_st *burrow, void *context)
{
  burrow->context = context;
}

void *burrow_get_context(burrow_st *burrow)
{
  return burrow->context;
}

void burrow_set_options(burrow_st *burrow, burrow_options_t options)
{
  burrow->options = options;
}

void burrow_add_options(burrow_st *burrow, burrow_options_t options)
{
  burrow->options |= options;
}

void burrow_remove_options(burrow_st *burrow, burrow_options_t options_to_remove)
{
  burrow->options &= ~options_to_remove;
}

burrow_options_t burrow_get_options(burrow_st *burrow)
{
  return burrow->options;
}

burrow_result_t burrow_backend_set_option(burrow_st *burrow, const char *option, const char *value)
{
  return burrow->backend->set_option(burrow->backend, option, value);
}

burrow_result_t burrow_backend_set_option_int(burrow_st *burrow, const char *option, int32_t value)
{
  (void) burrow;
  (void) option;
  (void) value;
  return BURROW_ERROR_UNSUPPORTED;
  /* TODO: not implemented? Maybe never? */
}

void burrow_set_message_fn(burrow_st *burrow, burrow_message_fn *callback)
{
  burrow->message_fn = callback;
}

void burrow_set_accounts_fn(burrow_st *burrow, burrow_accounts_fn *callback)
{
  burrow->accounts_fn = callback;
}

void burrow_set_queues_fn(burrow_st *burrow, burrow_queues_fn *callback)
{
  burrow->queues_fn = callback;
}

void burrow_set_log_fn(burrow_st *burrow, burrow_log_fn *callback)
{
  burrow->log_fn = callback;
}

void burrow_set_complete_fn(burrow_st *burrow, burrow_complete_fn *callback)
{
  burrow->complete_fn = callback;
}

void burrow_set_watch_fd_fn(burrow_st *burrow, burrow_watch_fd_fn *callback)
{
  burrow->watch_fd_fn = callback;
}

void burrow_set_malloc_fn(burrow_st *burrow, burrow_malloc_fn *func)
{
  burrow->malloc_fn = func;
}

void burrow_set_free_fn(burrow_st *burrow, burrow_free_fn *func)
{
  burrow->free_fn = func;
}

burrow_result_t burrow_get_message(burrow_st *burrow,
                                   const char *account,
                                   const char *queue,
                                   const char *message_id,
                                   const burrow_filters_st *filters)
{
  if (burrow->state != BURROW_STATE_IDLE) {
    burrow_log_error(burrow, "burrow_get_message: burrow not in an idle state");
    return BURROW_ERROR_NOT_READY;
  }

  if (!account || !queue || !message_id) {
    burrow_log_error(burrow, "burrow_get_message: invalid parameters");
    return BURROW_ERROR_BAD_ARGS;
  }
  
  burrow->cmd.command = BURROW_CMD_GET_MESSAGE;
  burrow->cmd.account = account;
  burrow->cmd.queue = queue;
  burrow->cmd.message_id = message_id;
  burrow->cmd.filters = filters;
  
  burrow->state = BURROW_STATE_START;

  if (burrow->options & BURROW_OPT_AUTOPROCESS)
    return burrow_process(burrow);

  return BURROW_OK;
}

burrow_result_t burrow_create_message(burrow_st *burrow,
                                      const char *account, 
                                      const char *queue, 
                                      const char *message_id,
                                      const uint8_t *body,
                                      size_t body_size,
                                      const burrow_attributes_st *attributes)
{
  if (burrow->state != BURROW_STATE_IDLE) {
    burrow_log_error(burrow, "burrow_create_message: burrow not in an idle state");
    return BURROW_ERROR_NOT_READY;
  }
  
  if (!account || !queue || !message_id || !body) {
    burrow_log_error(burrow, "burrow_create_message: invalid parameters");
    return BURROW_ERROR_BAD_ARGS;
  }
  
  burrow->cmd.command = BURROW_CMD_CREATE_MESSAGE;
  burrow->cmd.account = account;
  burrow->cmd.queue = queue;
  burrow->cmd.message_id = message_id;
  burrow->cmd.body = body;
  burrow->cmd.body_size = body_size;
  burrow->cmd.attributes = attributes;
  
  burrow->state = BURROW_STATE_START;

  if (burrow->options & BURROW_OPT_AUTOPROCESS)
    return burrow_process(burrow);

  return BURROW_OK;
}

burrow_result_t burrow_update_message(burrow_st *burrow,
                                      const char *account,
                                      const char *queue,
                                      const char *message_id,
                                      const burrow_attributes_st *attributes,
                                      const burrow_filters_st *filters)
{
  if (burrow->state != BURROW_STATE_IDLE) {
    burrow_log_error(burrow, "burrow_update_message: burrow not in an idle state");
    return BURROW_ERROR_NOT_READY;
  }
  
  if (!account || !queue || !message_id || !attributes) {
    burrow_log_error(burrow, "burrow_update_message: invalid parameters");
    return BURROW_ERROR_BAD_ARGS;
  }
  
  burrow->cmd.command = BURROW_CMD_UPDATE_MESSAGE;
  burrow->cmd.account = account;
  burrow->cmd.queue = queue;
  burrow->cmd.message_id = message_id;
  burrow->cmd.attributes = attributes;
  burrow->cmd.filters = filters;
  
  burrow->state = BURROW_STATE_START;

  if (burrow->options & BURROW_OPT_AUTOPROCESS)
    return burrow_process(burrow);

  return BURROW_OK;
}

burrow_result_t burrow_delete_message(burrow_st *burrow,
                                      const char *account,
                                      const char *queue,
                                      const char *message_id,
                                      const burrow_filters_st *filters)
{
  if (burrow->state != BURROW_STATE_IDLE) {
    burrow_log_error(burrow, "burrow_delete_message: burrow not in an idle state");
    return BURROW_ERROR_NOT_READY;
  }
  
  if (!account || !queue || !message_id) {
    burrow_log_error(burrow, "burrow_delete_message: invalid parameters");
    return BURROW_ERROR_BAD_ARGS;
  }
  
  burrow->cmd.command = BURROW_CMD_DELETE_MESSAGE;
  burrow->cmd.account = account;
  burrow->cmd.queue = queue;
  burrow->cmd.message_id = message_id;
  burrow->cmd.filters = filters;
  
  burrow->state = BURROW_STATE_START;

  if (burrow->options & BURROW_OPT_AUTOPROCESS)
    return burrow_process(burrow);

  return BURROW_OK;
}

burrow_result_t burrow_get_messages(burrow_st *burrow,
                                    const char *account,
                                    const char *queue,
                                    const burrow_filters_st *filters)
{
  if (burrow->state != BURROW_STATE_IDLE) {
    burrow_log_error(burrow, "burrow_get_messages: burrow not in an idle state");
    return BURROW_ERROR_NOT_READY;
  }
  
  if (!account || !queue) {
    burrow_log_error(burrow, "burrow_get_messages: invalid parameters");
    return BURROW_ERROR_BAD_ARGS;
  }
  
  burrow->cmd.command = BURROW_CMD_GET_MESSAGES;
  burrow->cmd.account = account;
  burrow->cmd.queue = queue;
  burrow->cmd.filters = filters;
  
  burrow->state = BURROW_STATE_START;

  if (burrow->options & BURROW_OPT_AUTOPROCESS)
    return burrow_process(burrow);

  return BURROW_OK;
}


burrow_result_t burrow_delete_messages(burrow_st *burrow,
                                       const char *account,
                                       const char *queue,
                                       const burrow_filters_st *filters)
{
  if (burrow->state != BURROW_STATE_IDLE) {
    burrow_log_error(burrow, "burrow_delete_messages: burrow not in an idle state");
    return BURROW_ERROR_NOT_READY;
  }
  
  if (!account || !queue) {
    burrow_log_error(burrow, "burrow_delete_messages: invalid parameters");
    return BURROW_ERROR_BAD_ARGS;
  }
  
  burrow->cmd.command = BURROW_CMD_DELETE_MESSAGES;
  burrow->cmd.account = account;
  burrow->cmd.queue = queue;
  burrow->cmd.filters = filters;
  
  burrow->state = BURROW_STATE_START;

  if (burrow->options & BURROW_OPT_AUTOPROCESS)
    return burrow_process(burrow);

  return BURROW_OK;
}


burrow_result_t burrow_update_messages(burrow_st *burrow,
                                       const char *account,
                                       const char *queue,
                                       const burrow_attributes_st *attributes,
                                       const burrow_filters_st *filters)
{
  if (burrow->state != BURROW_STATE_IDLE) {
    burrow_log_error(burrow, "burrow_update_messages: burrow not in an idle state");
    return BURROW_ERROR_NOT_READY;
  }
  
  if (!account || !queue || !attributes) {
    burrow_log_error(burrow, "burrow_update_messages: invalid parameters");
    return BURROW_ERROR_BAD_ARGS;
  }
  
  burrow->cmd.command = BURROW_CMD_UPDATE_MESSAGES;
  burrow->cmd.account = account;
  burrow->cmd.queue = queue;
  burrow->cmd.filters = filters;
  burrow->cmd.attributes = attributes;
  
  burrow->state = BURROW_STATE_START;

  if (burrow->options & BURROW_OPT_AUTOPROCESS)
    return burrow_process(burrow);

  return BURROW_OK;
}


burrow_result_t burrow_get_queues(burrow_st *burrow,
                                  const char *account,
                                  const burrow_filters_st *filters)
{
  if (burrow->state != BURROW_STATE_IDLE) {
    burrow_log_error(burrow, "burrow_get_queues: burrow not in an idle state");
    return BURROW_ERROR_NOT_READY;
  }
  
  if (!account) {
    burrow_log_error(burrow, "burrow_get_queues: invalid parameters");
    return BURROW_ERROR_BAD_ARGS;
  }
  
  burrow->cmd.command = BURROW_CMD_GET_QUEUES;
  burrow->cmd.account = account;
  burrow->cmd.filters = filters;
  
  burrow->state = BURROW_STATE_START;

  if (burrow->options & BURROW_OPT_AUTOPROCESS)
    return burrow_process(burrow);

  return BURROW_OK;
}

burrow_result_t burrow_delete_queues(burrow_st *burrow,
                                     const char *account,
                                     const burrow_filters_st *filters)
{
  if (burrow->state != BURROW_STATE_IDLE) {
    burrow_log_error(burrow, "burrow_delete_queues: burrow not in an idle state");
    return BURROW_ERROR_NOT_READY;
  }
  
  if (!account) {
    burrow_log_error(burrow, "burrow_delete_queues: invalid parameters");
    return BURROW_ERROR_BAD_ARGS;
  }
  
  burrow->cmd.command = BURROW_CMD_DELETE_QUEUES;
  burrow->cmd.account = account;
  burrow->cmd.filters = filters;
  
  burrow->state = BURROW_STATE_START;

  if (burrow->options & BURROW_OPT_AUTOPROCESS)
    return burrow_process(burrow);

  return BURROW_OK;
}

burrow_result_t burrow_get_accounts(burrow_st *burrow, const burrow_filters_st *filters)
{
  if (burrow->state != BURROW_STATE_IDLE) {
    burrow_log_error(burrow, "burrow_get_accounts: burrow not in an idle state");
    return BURROW_ERROR_NOT_READY;
  }
  
  burrow->cmd.command = BURROW_CMD_GET_ACCOUNTS;
  burrow->cmd.filters = filters;
  
  burrow->state = BURROW_STATE_START;

  if (burrow->options & BURROW_OPT_AUTOPROCESS)
    return burrow_process(burrow);

  return BURROW_OK;
}


burrow_result_t burrow_delete_accounts(burrow_st *burrow, const burrow_filters_st *filters)
{
  if (burrow->state != BURROW_STATE_IDLE) {
    burrow_log_error(burrow, "burrow_delete_accounts: burrow not in an idle state");
    return BURROW_ERROR_NOT_READY;
  }
  
  burrow->cmd.command = BURROW_CMD_DELETE_ACCOUNTS;
  burrow->cmd.filters = filters;
  
  burrow->state = BURROW_STATE_START;

  if (burrow->options & BURROW_OPT_AUTOPROCESS)
    return burrow_process(burrow);

  return BURROW_OK;
}




















void burrow_log(burrow_st *burrow, burrow_verbose_t verbose, const char *msg, va_list args);

void burrow_error(burrow_st *burrow, burrow_result_t error, const char *msg, ...)
{
  va_list args;
  (void) error;

  if (burrow->verbose <= BURROW_VERBOSE_ERROR) {
    va_start(args, msg);
    burrow_log(burrow, BURROW_VERBOSE_ERROR, msg, args);
    va_end(args);
  }
}

void burrow_log_fatal(burrow_st *burrow, const char *msg, ...)
{
  va_list args;

  if (burrow->verbose <= BURROW_VERBOSE_FATAL) {
    va_start(args, msg);
    burrow_log(burrow, BURROW_VERBOSE_FATAL, msg, args);
    va_end(args);
  }
}

void burrow_log_error(burrow_st *burrow, const char *msg, ...)
{
  va_list args;

  if (burrow->verbose <= BURROW_VERBOSE_ERROR) {
    va_start(args, msg);
    burrow_log(burrow, BURROW_VERBOSE_ERROR, msg, args);
    va_end(args);
  }
}

void burrow_log_warn(burrow_st *burrow, const char *msg, ...)
{
  va_list args;

  if (burrow->verbose <= BURROW_VERBOSE_WARN) {
    va_start(args, msg);
    burrow_log(burrow, BURROW_VERBOSE_WARN, msg, args);
    va_end(args);
  }
}

void burrow_log_info(burrow_st *burrow, const char *msg, ...)
{
  va_list args;

  if (burrow->verbose <= BURROW_VERBOSE_INFO) {
    va_start(args, msg);
    burrow_log(burrow, BURROW_VERBOSE_INFO, msg, args);
    va_end(args);
  }
}

void burrow_log_debug(burrow_st *burrow, const char *msg, ...)
{
  va_list args;

  if (burrow->verbose <= BURROW_VERBOSE_DEBUG) {
    va_start(args, msg);
    burrow_log(burrow, BURROW_VERBOSE_DEBUG, msg, args);
    va_end(args);
  }
}

void burrow_log(burrow_st *burrow, burrow_verbose_t verbose, const char *msg, va_list args)
{ 
  char buffer[BURROW_MAX_ERROR_SIZE];

  if (burrow->log_fn == NULL) {
    printf("%5s: ", _error_strings[verbose]);
    vprintf(msg, args);
    printf("\n");
  } else {
    vsnprintf(buffer, BURROW_MAX_ERROR_SIZE, msg, args);
    burrow->log_fn(burrow, verbose, buffer);
  }
}
