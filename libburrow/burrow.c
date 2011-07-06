
/* Functions visible to the backend: */

void burrow_default_watch_fd(burrow_st *burrow, int fd, burrow_ioevent_t events)
{
  struct pollfd *pfd;
  uint32_t count;
  short evtemp;

  count = burrow->watch_count + 1;

  if (burrow->pfds_count < count) {
    pfd = realloc(burrow->pfds, count * sizeof(struct pollfd));
    if (!pfd) {
      /* raise error */
      return;
    }
    burrow->pfds = pfd;
    burrow->pfds_count = count;
  }

  burrow->watch_count = count;

  pfd = burrow->pfds[count-1];
  pfd->fd = fd;
  pfd->events = 0;
  if (events & BURROW_IOEVENTS_READ)
    pfd->events |= POLLIN;
  if (events & BURROW_IOEVENTS_WRITE)
    pfd->events |= POLLOUT;
}


burrow_result_t burrow_get_message(burrow_st *burrow, char *account, char *queue, char *msgid)
{
  if (burrow->state != BURROW_STATE_IDLE) {
    burrow_log_error("burrow_get_message: burrow not in an idle state")
    return BURROW_ERROR_NOT_READY;
  }
  
  burrow->cmd.command = BURROW_CMD_GET_MESSAGE;
  burrow->cmd.account = account;
  burrow->cmd.queue = queue;
  burrow->cmd.message_id = msgid;
  
  burrow->state = BURROW_STATE_START;

  if (burrow->options & BURROW_OPT_AUTOPROCESS)
    return burrow_process(burrow);

  return BURROW_OK;
}

burrow_result_t burrow_start_command(burrow_st *burrow)
{
  void *context = burrow->backend_context;
  
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
      return burrow->backend->get_message(context, cmd->account, cmd->queue, cmd->message_id);
    
    case BURROW_CMD_UPDATE_MESSAGE:
      return burrow->backend->update_message(context, cmd->account, cmd->queue, cmd->message_id, cmd->attributes);
    
    case BURROW_CMD_DELETE_MESSAGE:
      return burrow->backend->delete_message(context, cmd->account, cmd->queue, cmd->message_id);

    case BURROW_CMD_CREATE_MESSAGE:
      return burrow->backend->create_message(context, cmd->account, cmd->queue, cmd->message_id, cmd->body, cmd->body_size, cmd->attributes);
  }
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
      if (burrow->callbacks->complete)
        burrow->callbacks->complete(burrow); /* could update burrow state by calling a command again */
      break;
    }
  }
  
  burrow->flags ^= BURROW_FLAG_PROCESSING;

  return result;
}


void burrow_poll_fds(burrow_st *burrow)
{
  int count, watch_size;
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
      burrow_raise_event(buffow, pfd->fd, event);

      /* And copy the last pfd to this location */
      count--;
      watch_size--;
      last_pfd = burrow->pfds[watch_size - 1];
      
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


void burrow_event_raised(burrow_st *burrow, int fd, burrow_ioevent_t event)
{
  if (burrow->state != BURROW_STATE_WAITING)
    /* log warning */;
    
  burrow_result_t result = burrow->backend->event_raised(fd, event);
  
  if (result == BURROW_OK) {
    burrow->state = BURROW_STATE_READY;
    if (burrow->options & BURROW_FLAG_AUTOPROCESS)
      burrow_process(burrow);
  }
}


/**
 * Cancels an ongoing command. 
 * Will trigger a command-complete callback if any command is pending.
 * TODO: Should trigger a command complete call always?
 *
 * @param burrow Burrow object
 */
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

void burrow_default_message_fn(burrow_st *burrow, char *message_id,
                              uint8_t *body,
                              size_t body_size,
                              burrow_attributes_st *attributes)
{
  burrow_log_info(burrow, "Message received, but message callback function undefined.");
}

void burrow_default_queues_fn(burrow_st *burrow, char **queues)
{
  burrow_log_info(burrow, "Queues received, but queues callback function undefined.");
}

void burrow_default_accounts_fn(burrow_st *burrow, char **accounts)
{
  burrow_log_info(burrow, "Accounts received, but accounts callback function undefined.");
}

void burrow_default_complete_fn(burrow_st *burrow)
{
  burrow_log_info(burrow, "Command complete, but complete callback function undefined.");
}

void *burrow_default_malloc_fn(burrow_st *burrow, size_t size)
{
  return malloc(size);
}

void burrow_default_free_fn(burrow_st *burrow, void *ptr)
{
  free(ptr);
}

burrow_st *burrow_create(burrow_st *burrow, char *backend)
{
  burrow_backend_functions_st *backend_fns;
  
  backend_fns = burrow_backend_load_functions(backend);
  if (!backend_fns)
    return NULL;
  
  if (!burrow) {
    /* We allocate to include the backend just after the base
       burrow struct */
    burrow = malloc(sizeof(burrow_st) + backend->size());
    if (!burrow)
      return NULL;
    burrow->flags = BURROW_FLAG_SELFALLOCATED;
  } else
    burrow->flags = 0;
  
  burrow->options = 0;
  burrow->verbose = BURROW_VERBOSE_ERROR;
  burrow->state = BURROW_STATE_IDLE;
  burrow->cmd.command = BURROW_COMMAND_NONE;
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

  return burrow;
}

/**
 * Frees a burrow object, utilizing the free function set in the burrow
 * object (if set).
 *
 * @param burrow Burrow object to be freed
 */
void burrow_free(burrow_st *burrow)
{
  /* TODO: more */
  burrow->backend->free((void*)(burrow+1));
  if (burrow->flags & BURROW_FLAG_SELFALLOCATED)
    free(burrow)
}

/**
 * Returns the size required to allocate a burrow object, optionally
 * with a specific backend included.
 *
 * @param backend Backend for size calculation
 * @return The size of a complete burrow structure
 */
ssize_t burrow_size(char *backend)
{
  burrow_backend_functions_st *backend_fns;
  
  backend_fns = burrow_backend_load_functions(backend);
  if (!backend_fns)
    return -1;
    
  return sizeof(burrow_st) + backend_fns->size();
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
  return NULL;
}
 
/**
 * Sets an associated context pointer. This will be passed to all
 * callback functions.
 *
 * @param burrow Burrow object
 * @param context Pointer to context
 */
void burrow_set_context(burrow_st *burrow, void *context)
{
  burrow->context = context;
}

/**
 * Gets the associated context pointer.
 *
 * @param burrow Burrow object
 */
void *burrow_get_context(burrow_st *burrow)
{
  return burrow->context;
}

/**
 * Sets burrow options
 *
 * @param burrow Burrow object
 * @param options Options to be set
 */
void burrow_set_options(burrow_st *burrow, burrow_options_t options)
{
  burrow->options = options;
}

/**
 * Enables specified options.
 *
 * @param burrow Burrow object
 * @param options Options to be enabled
 */
void burrow_add_options(burrow_st *burrow, burrow_options_t options)
{
  burrow->options |= options;
}

/**
 * Disables specified options.
 *
 * @param burrow Burrow object
 * @param options Options to be disabled
 */
void burrow_remove_options(burrow_st *burrow, burrow_options_t options_to_remove)
{
  burrow->options &= ~options_to_remove;
}

/**
 * Returns current burrow options.
 *
 * @param burrow Burrow object
 * @return Current burrow options
 */
burrow_options_t burrow_get_options(burrow_st *burrow)
{
  return burrow->options;
}

/**
 * Sets a string backend option.
 *
 * @param burrow Burrow object
 * @param option Option name
 * @param value  Option value, string
 */
burrow_result_t burrow_backend_set_option(burrow_st *burrow, const char *option, const char *value)
{
  return burrow->backend_fns->set_option(option, value);
}

burrow_result_t burrow_backend_set_option_int(burrow_st *burrow, char *option, int32_t value)
{
  return BURROW_ERROR_UNIMPLEMENTED;
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

void burrow_set_complete_fn(burrow_st *burrow, burrow_complete_fn *callback);
{
  burrow->complete_fn = callback;
}


void burrow_set_watch_fd_fn(burrow_st *burrow, burrow_event_wait_fn *callback)
{
  burrow->watch_fd_fn = callback;
}


void burrow_set_malloc_fn(burrow_st *burrow, burrow_malloc_fn *func)
{
  burrow->malloc_fn = callback;
}


void burrow_set_free_fn(burrow_st *burrow, burrow_free_fn *func)
{
  burrow->free_fn = callback;
}

