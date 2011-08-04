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
 * @brief Burrow frontend definitions
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

void burrow_internal_log(burrow_st *burrow,
                         burrow_verbose_t verbose,
                         const char *msg,
                         va_list args)
{ 
  char buffer[BURROW_MAX_ERROR_SIZE];

  if (burrow->log_fn == NULL)
  {
    printf("%5s: ", burrow_verbose_name(verbose));
    vprintf(msg, args);
    printf("\n");
  }
  else
  {
    vsnprintf(buffer, BURROW_MAX_ERROR_SIZE, msg, args);
    burrow->log_fn(burrow, verbose, buffer);
  }
}


int burrow_internal_watch_fd(burrow_st *burrow, int fd, burrow_ioevent_t events)
{
  struct pollfd *pfd;
  uint32_t needed;

  needed = burrow->watch_size + 1;

  if (burrow->pfds_size < needed)
  {
    pfd = realloc(burrow->pfds, needed * sizeof(struct pollfd));
    if (!pfd)
    {
      burrow_log_error(burrow,
                       "burrow_internal_watch_fd: couldn't reallocate pfds");
      return ENOMEM;
    }
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

  return 0;
}

int burrow_internal_poll_fds(burrow_st *burrow)
{
  int count;
  uint32_t watch_size;
  struct pollfd *pfd;
  struct pollfd *last_pfd;
  
  if (burrow->watch_size == 0) /* nothing to watch */
    return 0;

  count = poll(burrow->pfds, burrow->watch_size, burrow->timeout);
  if (count == -1)
  {
    burrow_log_error(burrow,
                     "burrow_internal_poll_fds: poll: error encountered %d",
                     errno);
    return errno;
  }
  else if (count == 0)
  {
    /* Timeout has occurred */
    burrow_log_info(burrow,
                    "burrow_internal_poll_fds: timeout %d reached",
                    burrow->timeout);
    burrow_cancel(burrow);
    return ETIMEDOUT;
  }

  pfd = burrow->pfds;
  watch_size = burrow->watch_size;
  /* OSX will return one 'count' value per event raised, not per fd;
     this is a bug in its poll implementation. We work around by simply
     checking all fds, otherwise ignoring the count retrieved above */
  while(pfd < (burrow->pfds + watch_size))
  {
    if (pfd->revents) /* found a live event */
    {
      
      /* Dispatch it: */
      burrow_ioevent_t event = BURROW_IOEVENT_NONE;      
      if (pfd->revents & POLLIN)
        event |= BURROW_IOEVENT_READ;
      if (pfd->revents & POLLOUT)
        event |= BURROW_IOEVENT_WRITE;
      burrow_event_raised(burrow, pfd->fd, event);

      /* And copy the last pfd to this location */
      watch_size--;
      last_pfd = &burrow->pfds[watch_size];
      
      /* But not if we're at the last pfd entry */
      if (last_pfd > pfd)
      {
        pfd->fd = last_pfd->fd;
        pfd->events = last_pfd->events;
        pfd->revents = last_pfd->revents;
      }
      /* Note that we don't increment pfd here, because this location
         now has new data */
    }
    else /* this event not live -- skip it */
      pfd++;
  }
  burrow->watch_size = watch_size;
  return 0;
}

int burrow_process(burrow_st *burrow)
{
  int result = 0;

  if (burrow->flags & BURROW_FLAG_PROCESSING)/* prevent recursion */
    return EAGAIN; /* parent process loop will pick it up */

  burrow->flags |= BURROW_FLAG_PROCESSING;

  while (burrow->state != BURROW_STATE_IDLE)
  {
    switch(burrow->state)
    {
    case BURROW_STATE_START:
      /* command is initialized, but hasn't kicked off */
      result = burrow->cmd.command_fn(burrow->backend_context, &burrow->cmd);
      if (result == EAGAIN)
        burrow->state = BURROW_STATE_WAITING;
      else /* could be error or OK */
        burrow->state = BURROW_STATE_FINISH;
      break;

    case BURROW_STATE_READY:
      /* io events have made the backend ready */
      result = burrow->backend->process(burrow->backend_context);
      if (result == EAGAIN)
        burrow->state = BURROW_STATE_WAITING;
      else /* could be error or OK */
        burrow->state = BURROW_STATE_FINISH;
      break;

    case BURROW_STATE_WAITING: /* backend is blocking on io */
      if (burrow->watch_size == 0)
        return EAGAIN; /* waiting is performed by the client */
      
      if ((result = burrow_internal_poll_fds(burrow)) != 0)
        return result; /* error received */
      break;

    case BURROW_STATE_FINISH: /* backend is done */
      if (burrow->watch_size > 0)
        burrow_log_error(burrow, "burrow_process: finishd with active fds");
        
      burrow->state = BURROW_STATE_IDLE; /* we now accept new commands */
      burrow->cmd.command = BURROW_CMD_NONE;

      /* Note: this could update burrow state by calling a command again: */
      burrow_callback_complete(burrow); 
      break;
    
    case BURROW_STATE_IDLE: /* suppress compiler warning */
    default:
      burrow_log_warn(burrow, "burrow_process: unexpected or unknown state %x",
                      burrow->state);
      break;
    }
  }
  
  burrow->flags ^= BURROW_FLAG_PROCESSING;
  return result;
}

int burrow_event_raised(burrow_st *burrow, int fd, burrow_ioevent_t event)
{
  int result;
  
  if (!burrow->backend->event_raised)
  {
    burrow_log_warn(burrow,
                "burrow_event_raised: event raised but not no handler defined");
    return ENOTCONN;
  }

  if (burrow->state != BURROW_STATE_WAITING)
    burrow_log_warn(burrow,
                    "burrow_event_raised: unexpected event, fd %d, event %x",
                    fd, event);
        
  result = burrow->backend->event_raised(burrow->backend_context, fd, event);
  
  if (result == 0)
  {
    burrow->state = BURROW_STATE_READY;
    if (burrow->options & BURROW_OPT_AUTOPROCESS)
      return burrow_process(burrow);
  }
  return result;
}

void burrow_cancel(burrow_st *burrow)
{
  /* TODO: Does not yet have the semantics as documented */
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
  
  if (!burrow)
  {
    /* We allocate to include the backend just after the base
       burrow struct */
    burrow = malloc(sizeof(burrow_st) + backend_fns->size());
    if (!burrow)
      return NULL;
    burrow->flags = BURROW_FLAG_SELFALLOCATED;
  }
  else
    burrow->flags = 0;
  
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
  burrow_log_debug(burrow, "burrow_destroy: freeing backend"); 
  burrow->backend->destroy((void*)(burrow+1));

  burrow_free(burrow, burrow->pfds);

  burrow_log_debug(burrow, "burrow_destroy: attributes list %c= NULL",
                   (burrow->attributes_list == NULL ? '=' : '!')); 
  while (burrow->attributes_list != NULL)
    burrow_attributes_destroy(burrow->attributes_list);

  burrow_log_debug(burrow, "burrow_destroy: filters list %c= NULL",
                   (burrow->filters_list == NULL ? '=' : '!')); 
  while (burrow->filters_list != NULL)
    burrow_filters_destroy(burrow->filters_list);

  if (burrow->flags & BURROW_FLAG_SELFALLOCATED)
  {
    burrow_log_debug(burrow,
                     "burrow_destroy: freeing self-allocated structure"); 
    burrow_free(burrow, burrow);
  }
  else
    burrow_log_debug(burrow,
                     "burrow_destroy: not freeing user-provided memory"); 
}

size_t burrow_size(const char *backend)
{
  burrow_backend_functions_st *backend_fns;
  
  backend_fns = burrow_backend_load_functions(backend);
  if (!backend_fns)
    return 0;
    
  return (sizeof(burrow_st) + backend_fns->size());
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

void burrow_remove_options(burrow_st *burrow,
                           burrow_options_t options_to_remove)
{
  burrow->options &= ~options_to_remove;
}

burrow_options_t burrow_get_options(burrow_st *burrow)
{
  return burrow->options;
}

int burrow_set_backend_option(burrow_st *burrow,
                              const char *option,
                              const char *value)
{
  if (!burrow->backend->set_option)
    return EINVAL;
  return burrow->backend->set_option(burrow->backend_context, option, value);
}

int burrow_set_backend_option_int(burrow_st *burrow,
                                  const char *option,
                                  int32_t value)
{
  (void) burrow;
  (void) option;
  (void) value;
  return EINVAL;
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

void burrow_set_verbosity(burrow_st *burrow, burrow_verbose_t verbosity)
{
  burrow->verbose = verbosity;
}


int burrow_get_message(burrow_st *burrow,
                       const char *account,
                       const char *queue,
                       const char *message_id,
                       const burrow_filters_st *filters)
{
  if (burrow->state != BURROW_STATE_IDLE)
  {
    burrow_log_error(burrow, "burrow_get_message: burrow not idle");
    return EINPROGRESS;
  }

  if (!account || !queue || !message_id)
  {
    burrow_log_error(burrow, "burrow_get_message: invalid parameters");
    return EINVAL;
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

  return 0;
}

int burrow_create_message(burrow_st *burrow,
                          const char *account, 
                          const char *queue, 
                          const char *message_id,
                          const uint8_t *body,
                          size_t body_size,
                          const burrow_attributes_st *attributes)
{
  if (burrow->state != BURROW_STATE_IDLE)
  {
    burrow_log_error(burrow, "burrow_create_message: burrow not idle");
    return EINPROGRESS;
  }
  
  if (!account || !queue || !message_id || !body)
  {
    burrow_log_error(burrow, "burrow_create_message: invalid parameters");
    return EINVAL;
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

  return 0;
}

int burrow_update_message(burrow_st *burrow,
                                      const char *account,
                                      const char *queue,
                                      const char *message_id,
                                      const burrow_attributes_st *attributes,
                                      const burrow_filters_st *filters)
{
  if (burrow->state != BURROW_STATE_IDLE)
  {
    burrow_log_error(burrow, "burrow_update_message: burrow not idle");
    return EINPROGRESS;
  }
  
  if (!account || !queue || !message_id || !attributes)
  {
    burrow_log_error(burrow, "burrow_update_message: invalid parameters");
    return EINVAL;
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

  return 0;
}

int burrow_delete_message(burrow_st *burrow,
                          const char *account,
                          const char *queue,
                          const char *message_id,
                          const burrow_filters_st *filters)
{
  if (burrow->state != BURROW_STATE_IDLE)
  {
    burrow_log_error(burrow, "burrow_delete_message: burrow not idle");
    return EINPROGRESS;
  }
  
  if (!account || !queue || !message_id)
  {
    burrow_log_error(burrow, "burrow_delete_message: invalid parameters");
    return EINVAL;
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

  return 0;
}

int burrow_get_messages(burrow_st *burrow,
                        const char *account,
                        const char *queue,
                        const burrow_filters_st *filters)
{
  if (burrow->state != BURROW_STATE_IDLE)
  {
    burrow_log_error(burrow, "burrow_get_messages: burrow not idle");
    return EINPROGRESS;
  }
  
  if (!account || !queue)
  {
    burrow_log_error(burrow, "burrow_get_messages: invalid parameters");
    return EINVAL;
  }
  
  burrow->cmd.command = BURROW_CMD_GET_MESSAGES;
  burrow->cmd.command_fn = burrow->backend->get_messages;
  burrow->cmd.account = account;
  burrow->cmd.queue = queue;
  burrow->cmd.filters = filters;
  
  burrow->state = BURROW_STATE_START;

  if (burrow->options & BURROW_OPT_AUTOPROCESS)
    return burrow_process(burrow);

  return 0;
}


int burrow_delete_messages(burrow_st *burrow,
                           const char *account,
                           const char *queue,
                           const burrow_filters_st *filters)
{
  if (burrow->state != BURROW_STATE_IDLE)
  {
    burrow_log_error(burrow, "burrow_delete_messages: burrow not idle");
    return EINPROGRESS;
  }
  
  if (!account || !queue)
  {
    burrow_log_error(burrow, "burrow_delete_messages: invalid parameters");
    return EINVAL;
  }
  
  burrow->cmd.command = BURROW_CMD_DELETE_MESSAGES;
  burrow->cmd.command_fn = burrow->backend->delete_messages;
  burrow->cmd.account = account;
  burrow->cmd.queue = queue;
  burrow->cmd.filters = filters;
  
  burrow->state = BURROW_STATE_START;

  if (burrow->options & BURROW_OPT_AUTOPROCESS)
    return burrow_process(burrow);

  return 0;
}


int burrow_update_messages(burrow_st *burrow,
                           const char *account,
                           const char *queue,
                           const burrow_attributes_st *attributes,
                           const burrow_filters_st *filters)
{
  if (burrow->state != BURROW_STATE_IDLE)
  {
    burrow_log_error(burrow, "burrow_update_messages: burrow not idle");
    return EINPROGRESS;
  }
  
  if (!account || !queue || !attributes)
  {
    burrow_log_error(burrow, "burrow_update_messages: invalid parameters");
    return EINVAL;
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

  return 0;
}


int burrow_get_queues(burrow_st *burrow,
                      const char *account,
                      const burrow_filters_st *filters)
{
  if (burrow->state != BURROW_STATE_IDLE)
  {
    burrow_log_error(burrow, "burrow_get_queues: burrow not idle");
    return EINPROGRESS;
  }
  
  if (!account)
  {
    burrow_log_error(burrow, "burrow_get_queues: invalid parameters");
    return EINVAL;
  }
  
  burrow->cmd.command = BURROW_CMD_GET_QUEUES;
  burrow->cmd.command_fn = burrow->backend->get_queues;
  burrow->cmd.account = account;
  burrow->cmd.filters = filters;
  
  burrow->state = BURROW_STATE_START;

  if (burrow->options & BURROW_OPT_AUTOPROCESS)
    return burrow_process(burrow);

  return 0;
}

int burrow_delete_queues(burrow_st *burrow,
                         const char *account,
                         const burrow_filters_st *filters)
{
  if (burrow->state != BURROW_STATE_IDLE)
  {
    burrow_log_error(burrow, "burrow_delete_queues: burrow not idle");
    return EINPROGRESS;
  }
  
  if (!account)
  {
    burrow_log_error(burrow, "burrow_delete_queues: invalid parameters");
    return EINVAL;
  }
  
  burrow->cmd.command = BURROW_CMD_DELETE_QUEUES;
  burrow->cmd.command_fn = burrow->backend->delete_queues;
  burrow->cmd.account = account;
  burrow->cmd.filters = filters;
  
  burrow->state = BURROW_STATE_START;

  if (burrow->options & BURROW_OPT_AUTOPROCESS)
    return burrow_process(burrow);

  return 0;
}

int burrow_get_accounts(burrow_st *burrow, const burrow_filters_st *filters)
{
  if (burrow->state != BURROW_STATE_IDLE)
  {
    burrow_log_error(burrow, "burrow_get_accounts: burrow not idle");
    return EINPROGRESS;
  }
  
  burrow->cmd.command = BURROW_CMD_GET_ACCOUNTS;
  burrow->cmd.command_fn = burrow->backend->get_accounts;
  burrow->cmd.filters = filters;
  
  burrow->state = BURROW_STATE_START;

  if (burrow->options & BURROW_OPT_AUTOPROCESS)
    return burrow_process(burrow);

  return 0;
}

int burrow_delete_accounts(burrow_st *burrow, const burrow_filters_st *filters)
{
  if (burrow->state != BURROW_STATE_IDLE)
  {
    burrow_log_error(burrow, "burrow_delete_accounts: burrow not idle");
    return EINPROGRESS;
  }
  
  burrow->cmd.command = BURROW_CMD_DELETE_ACCOUNTS;
  burrow->cmd.command_fn = burrow->backend->delete_accounts;
  burrow->cmd.filters = filters;
  
  burrow->state = BURROW_STATE_START;

  if (burrow->options & BURROW_OPT_AUTOPROCESS)
    return burrow_process(burrow);

  return 0;
}
