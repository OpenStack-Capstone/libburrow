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

/* Functions visible to the backend: */

const char *_error_strings[] = {
  "MAX", /* this and NONE should never be printed... but just in case */
  "DEBUG",
  "INFO",
  "WARN",
  "ERROR",
  "FATAL",
  "NONE",
};

const char *burrow_verbose_name(burrow_verbose_t verbose)
{
  return _error_strings[verbose];
}

void burrow_internal_log(burrow_st *burrow, burrow_verbose_t verbose, const char *msg, va_list args)
{ 
  char buffer[BURROW_MAX_ERROR_SIZE];

  if (burrow->log_fn == NULL) {
    printf("%5s: ", burrow_verbose_name(verbose));
    vprintf(msg, args);
    printf("\n");
  } else {
    vsnprintf(buffer, BURROW_MAX_ERROR_SIZE, msg, args);
    burrow->log_fn(burrow, verbose, buffer);
  }
}


void burrow_internal_watch_fd(burrow_st *burrow, int fd, burrow_ioevent_t events)
{
  struct pollfd *pfd;
  uint32_t needed;

  needed = burrow->watch_size + 1;

  if (burrow->pfds_size < needed) {
    burrow_log_debug(burrow, "burrow_internal_watch_fd: reallocating pfd structure %u pfd structs", needed);
    if (burrow->pfds_size > 0)
      pfd = realloc(burrow->pfds, needed * sizeof(struct pollfd));
    else
      pfd = malloc(needed * sizeof(struct pollfd));
    if (!pfd) {
      burrow_log_error(burrow, "burrow_internal_watch_fd: couldn't reallocate pfds struct");
      /* TODO: something more here? how to bubble up this error? */
      return;
    }
    burrow_log_debug(burrow, "burrow_internal_watch_fd: pfds given address %x", pfd);
    burrow->pfds = pfd;
    burrow->pfds_size = needed;
  }

  burrow->watch_size = needed;

  pfd = &burrow->pfds[needed-1];
  pfd->fd = fd;
  pfd->events = 0;
  if (events & BURROW_IOEVENT_READ)
    pfd->events |= POLLIN;
  if (events & BURROW_IOEVENT_WRITE)
    pfd->events |= POLLOUT;

  burrow_log_debug(burrow, "burrow_internal_watch_fd: called fd %d, events %x", fd, pfd->events);
}

static void burrow_internal_poll_fds(burrow_st *burrow)
{
  int count;
  uint32_t watch_size;
  struct pollfd *pfd;
  struct pollfd *last_pfd;
  
  if (burrow->watch_size == 0) /* nothing to watch */
    return;
  burrow_log_debug(burrow, "burrow_internal_poll_fds: watching %u file descriptors, pfds %x, npfds %u", burrow->watch_size, burrow->pfds, burrow->pfds_size);
  
  for (count = 0; count < (int)burrow->pfds_size; count++)
    burrow->pfds[count].revents = 0;

  burrow_log_debug(burrow, "debug: fd %d, event %x, revents %x", burrow->pfds[0].fd, burrow->pfds[0].events, burrow->pfds[0].revents);
  count = poll(burrow->pfds, burrow->watch_size, burrow->timeout);
  burrow_log_debug(burrow, "debug: fd %d, event %x, revents %x", burrow->pfds[0].fd, burrow->pfds[0].events, burrow->pfds[0].revents);

  burrow_log_debug(burrow, "burrow_internal_poll_fds: !! watching %u file descriptors, pfds %x, npfds %u, count %d", burrow->watch_size, burrow->pfds, burrow->pfds_size, count);

  if (count == -1) {
    burrow_log_error(burrow, "burrow_internal_poll_fds: poll: error encountered %d", errno);
    return;
  }
  if (count == 0) {
    /* Timeout has occurred */
    burrow_log_info(burrow, "burrow_internal_poll_fds: timeout %d reached", burrow->timeout);
    burrow_cancel(burrow);
    return;
  }
  burrow_log_debug(burrow, "burrow_internal_poll_fds: %d fds with events", count);
  pfd = burrow->pfds;
  
  watch_size = burrow->watch_size;
  while(count) {
    burrow_log_debug(burrow, "burrow_internal_poll_fds: step");
    if (pfd->revents) { /* Found a live event */
      burrow_log_debug(burrow, "burrow_internal_poll_fds: fd %d, events %x, revents %x", pfd->fd, pfd->events, pfd->revents);
      
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
      last_pfd = &burrow->pfds[watch_size];
      
      /* But not if we're at the last pfd entry */
      if (last_pfd > pfd) {
        pfd->fd = last_pfd->fd;
        pfd->events = last_pfd->events;
        pfd->revents = last_pfd->revents;
        last_pfd->fd = -1;
        last_pfd->events = -1;
        last_pfd->revents = -1;
      }
      /* Note that we don't increment pfd here, because this location
         now has new data */
    }
    else /* this event not live -- skip it */
      pfd++;
  }
  burrow->watch_size = watch_size;
  return;
}

burrow_result_t burrow_process(burrow_st *burrow)
{
  burrow_result_t result = BURROW_OK_WAITING;

  if (burrow->flags & BURROW_FLAG_PROCESSING) {/* prevent recursion */
    burrow_log_debug(burrow, "burrow_process: not recurring");
    return BURROW_OK_WAITING; /* parent process loop will pick it up */
  }

  burrow->flags |= BURROW_FLAG_PROCESSING;

  while (burrow->state != BURROW_STATE_IDLE) {
    switch(burrow->state) {
    
    case BURROW_STATE_START: /* command is initialized, but hasn't kicked off */
      burrow_log_debug(burrow, "burrow_process: state start: kicking off command");
      result = burrow->cmd.command_fn(burrow->backend_context, &burrow->cmd);
      if (result == BURROW_OK_WAITING)
        burrow->state = BURROW_STATE_WAITING;
      else /* could be error or OK */
        burrow->state = BURROW_STATE_FINISH;
      break;

    case BURROW_STATE_READY: /* io events have made the backend ready */
      burrow_log_debug(burrow, "burrow_process: state ready: processing");
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
      burrow_log_debug(burrow, "burrow_process: state waiting -- internally polling");
      burrow_internal_poll_fds(burrow); /* this should unblock the io */
      break;

    case BURROW_STATE_FINISH: /* backend is done */
      burrow_log_debug(burrow, "burrow_process: state finish: idle and complete");
      if (burrow->watch_size > 0)
        burrow_log_error(burrow, "in finish state while still waiting for fds");
      burrow->state = BURROW_STATE_IDLE; /* we now accept new commands */
      burrow->cmd.command = BURROW_CMD_NONE;
      /* Note: this could update burrow state by calling a command again: */
      burrow_callback_complete(burrow); 
      break;
    
    case BURROW_STATE_IDLE: /* suppress compiler warning */
    default:
      burrow_log_error(burrow, "burrow_process: unexpected or unknown state %d", burrow->state);
      break;
    }
  }
  
  burrow->flags ^= BURROW_FLAG_PROCESSING;
  return result;
}

burrow_result_t burrow_event_raised(burrow_st *burrow, int fd, burrow_ioevent_t event)
{
  burrow_result_t result;
  
  burrow_log_debug(burrow, "burrow_event_raised: fd: %d, event %x", fd, event);
  
  if (!burrow->backend->event_raised) {
    burrow_log_warn(burrow, "burrow_event_raised: event raised but not no handler defined");
    return BURROW_ERROR_UNSUPPORTED;
  }

  if (burrow->state != BURROW_STATE_WAITING)
    burrow_log_error(burrow, "burrow_event_raised: event raised but not expected, fd %d, event %x", fd, event);
        
  result = burrow->backend->event_raised(burrow->backend_context, fd, event);
  
  if (result == BURROW_OK) {
    burrow_log_debug(burrow, "burrow_event_raised: BURROW_OK returned, state to ready");
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
  burrow->cmd.command_fn = NULL;
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
  burrow->context = NULL;

  burrow->cmd.command_fn = NULL;
  burrow->cmd.command = BURROW_CMD_NONE;

  burrow->backend_context = backend_fns->create((void *)(burrow + 1), burrow);
  burrow->backend = backend_fns;

  burrow->malloc_fn   = NULL;
  burrow->free_fn     = NULL;
  
  burrow->message_fn  = NULL;
  burrow->queue_fn    = NULL;
  burrow->account_fn  = NULL;
  burrow->complete_fn = NULL;
  burrow->watch_fd_fn = NULL;
  burrow->log_fn      = NULL;
  
  burrow->pfds = NULL;
  burrow->pfds_size = 0;
  burrow->watch_size = 0;
  burrow->timeout = 10 * 1000; /* ten seconds */
  
  burrow->attributes_list = NULL;
  burrow->filters_list = NULL;

  return burrow;
}

void burrow_destroy(burrow_st *burrow)
{
  burrow->backend->destroy((void*)(burrow+1));

  burrow_free(burrow, burrow->pfds);

  burrow_log_debug(burrow, "burrow_destroy: attributes list %c= NULL", (burrow->attributes_list == NULL ? '=' : '!')); 
  while (burrow->attributes_list != NULL)
    burrow_attributes_free(burrow->attributes_list);

  burrow_log_debug(burrow, "burrow_destroy: filters list %c= NULL", (burrow->filters_list == NULL ? '=' : '!')); 
  while (burrow->filters_list != NULL)
    burrow_filters_free(burrow->filters_list);

  if (burrow->flags & BURROW_FLAG_SELFALLOCATED) {
    burrow_log_debug(burrow, "burrow_destroy: freeing self-allocated structure"); 
    burrow_free(burrow, burrow);
  }
  else
    burrow_log_debug(burrow, "burrow_destroy: not freeing user-provided memory"); 
}

size_t burrow_size(const char *backend)
{
  burrow_backend_functions_st *backend_fns;
  
  backend_fns = burrow_backend_load_functions(backend);
  if (!backend_fns)
    return 0;
    
  return (sizeof(burrow_st) + backend_fns->size());
}

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
  return burrow->backend->set_option(burrow->backend_context, option, value);
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

void burrow_set_account_fn(burrow_st *burrow, burrow_account_fn *callback)
{
  burrow->account_fn = callback;
}

void burrow_set_queue_fn(burrow_st *burrow, burrow_queue_fn *callback)
{
  burrow->queue_fn = callback;
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
  burrow->cmd.command_fn = burrow->backend->get_message;
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
  burrow->cmd.command_fn = burrow->backend->create_message;
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
  burrow->cmd.command_fn = burrow->backend->update_message;
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
  burrow->cmd.command_fn = burrow->backend->delete_message;
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
  burrow->cmd.command_fn = burrow->backend->get_messages;
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
  burrow->cmd.command_fn = burrow->backend->delete_messages;
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
  burrow->cmd.command_fn = burrow->backend->update_messages;
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
  burrow->cmd.command_fn = burrow->backend->get_queues;
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
  burrow->cmd.command_fn = burrow->backend->delete_queues;
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
  burrow->cmd.command_fn = burrow->backend->get_accounts;
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
  burrow->cmd.command_fn = burrow->backend->delete_accounts;
  burrow->cmd.filters = filters;
  
  burrow->state = BURROW_STATE_START;

  if (burrow->options & BURROW_OPT_AUTOPROCESS)
    return burrow_process(burrow);

  return BURROW_OK;
}
