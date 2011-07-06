#include <libburrow/burrow.h>

struct burrow_backend_dummy_st
{
  int selfallocated;
  burrow_st burrow;

  char *account;
  char *queue;
  char *message_id;
  uint8_t *body;
  size_t body_size;
  time_t ttl;
  time_t hide;
};

static void dummy_free_internals(burrow_backend_dummy_st *dummy)
{
  if (dummy->account)
    dummy->burrow->free_fn(dummy->burrow, dummy->account);
  if (dummy->queue)
    dummy->burrow->free_fn(dummy->burrow, dummy->queue);
  if (dummy->messge_id)
    dummy->burrow->free_fn(dummy->burrow, dummy->message_id);
  if (dummy->body)
    dummy->burrow->free_fn(dummy->burrow, dummy->body);
}

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
  
  /* The differences should be within 32 bits, but time_t may not be signed, so we do this: */
  attr.ttl = (int32_t)((int64_t)dummy->ttl - (int64_t)curtime);
  if (dummy->hide != 0 && dummy->hide < curtime) {
    attr.hide = (int32_t)((int64_t)dummy->hide - (int64_t)curtime);
  } else
    attr.hide = 0;
  
  switch(detail)
  {
  case BURROW_DETAIL_ID:
    body = NULL;
    body_size = -1;
    attrptr = NULL;
    break;
  case BURROW_DETAIL_BODY:
    attrptr = NULL;
    break;
  case BURROW_DETAIL_ATTRIBUTES:
    body = NULL;
    body_size = -1;
    break;
  case BURROW_DETAIL_ALL:
  case BURROW_DETAIL_UNSET:
  default:
    break;
  }
  dummy->burrow->callbacks->message(dummy->burrow, id, body, body_size, attrptr);
}

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
      && (!filters || filters->limit > 0))
    return 1; /* always match */
  
  if ((account && !strcmp(account, dummy->account))
      && (queue && !strcmp(queue, dummy->queue))
      && (message && !strcmp(message, dummy->message_id)))
      return 1;

  return 0;
}

void *burrow_backend_dummy_create(void *ptr, burrow_st *burrow)
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
}

void *burrow_backend_dummy_free(void *ptr)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  
  dummy_free_internals(dummy);
  
  if (dummy->selfallocated)
    dummy->burrow->free_fn(dummy->burrow, ptr);
}

size_t burrow_backend_dummy_size()
{
  return sizeof(burrow_backend_dummy_st);
}

void *burrow_backend_dummy_clone(void *dst, void *src)
{
  (void) dst;
  (void) src;
  
  /* TODO: unimplemented. semantics yet unclear */
  return NULL;
}

void burrow_backend_set_option(void *ptr, const char *key, const char *value)
{
  (void) ptr;
  (void) key;
  (void) value;
}

void burrow_backend_dummy_cancel(void *ptr)
{
  (void) ptr;
}

burrow_result_t burrow_backend_dummy_process(void *ptr)
{
  (void) ptr;
  return BURROW_OK;
}


burrow_result_t burrow_backend_dummy_event_raised(void *ptr, int fd, burrow_ioevent_t event)
{
  (void) ptr;
  (void) fd;
  (void) event;
  
  return BURROW_OK;
}

burrow_result_t burrow_backend_dummy_get_accounts(void *ptr, const burrow_filters_st *filters)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  
  if (!search_matches(dummy, NULL, NULL, NULL, filters))
    return BURROW_OK;
    
  dummy->burrow->callback->accounts(dummy->burrow, &dummy->account, 1);
  return BURROW_OK;
}

burrow_result_t burrow_backend_dummy_delete_accounts(void *ptr, const burrow_filters_st *filters)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  
  dummy_free_internals(dummy);
  return BURROW_OK;
}

burrow_result_t burrow_backend_dummy_get_queues(void *ptr, const char *account, const burrow_filters_st *filters)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  
  if (!search_matches(dummy, account, NULL, NULL, filters))
    return BURROW_OK;

  dummy->burrow->callback->queues(dummy->burrow, &dummy->queue, 1);
  return BURROW_OK;
}

burrow_result_t burrow_backend_dummy_delete_queues(void *ptr, const char *account, const burrow_filters_st *filters)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;

  if (!search_matches(dummy, account, NULL, NULL, filters))
    return BURROW_OK;
  
  dummy_free_internals(dummy);
  return BURROW_OK;
}

burrow_result_t burrow_backend_dummy_get_messages(void *ptr, const char *account, const char *queues, const burrow_filters_st *filters)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  burrow_detail_t detail = (filters ? filters->detail || BURROW_DETAIL_ALL : BURROW_DETAIL_ALL);
  
  if (!search_matches(dummy, account, queues, NULL, filters))
    return BURROW_OK;
    
  message_with_detail(dummy, detail);
  return BURROW_OK;
}

burrow_result_t burrow_backend_dummy_delete_messages(void *ptr, const char *account, const char *queues, const burrow_filters_st *filters)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;

  if (!search_matches(dummy, account, queues, NULL, filters))
    return BURROW_OK;

  dummy_free_internals(dummy);
  return BURROW_OK;
}

burrow_result_t burrow_backend_dummy_update_messages(void *ptr, const char *account, const char *queues, const burrow_attributes_st *attributes, const burrow_filters_st *filters)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  burrow_detail_t detail = (filters ? filters->detail || BURROW_DETAIL_ATTRIBUTES : BURROW_DETAIL_ATTRIBUTES);
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

burrow_result_t burrow_backend_dummy_get_message(void *ptr, const char *account, const char *queues, const char *message_id, const burrow_filters_st *filters)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  burrow_detail_t detail = (filters ? filters->detail || BURROW_DETAIL_ALL : BURROW_DETAIL_ALL);
  
  if (!search_matches(dummy, account, queues, message_id, NULL))
    return BURROW_OK;
    
  message_with_detail(dummy, detail);
  return BURROW_OK;
}

burrow_result_t burrow_backend_dummy_delete_message(void *ptr, const char *account, const char *queues, const char *message_id, const burrow_filters_st *filters)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  burrow_detail_t detail = (filters ? filters->detail || BURROW_DETAIL_ALL : BURROW_DETAIL_ALL);

  if (!search_matches(dummy, account, queues, message_id, NULL))
    return BURROW_OK;

  dummy_free_internals(dummy);
  return BURROW_OK;
}

burrow_result_t burrow_backend_dummy_update_message(void *ptr, const char *account, const char *queues, const burrow_attributes_st *attributes, const burrow_filters_st *filters)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;
  burrow_detail_t detail = (filters ? filters->detail || BURROW_DETAIL_ATTRIBUTES : BURROW_DETAIL_ATTRIBUTES);
  time_t curtime = time(NULL);

  if (!attributes)
    return BURROW_OK;
  
  if (!search_matches(dummy, account, queues, message_id, filters))
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

burrow_result_t burrow_backend_dummy_create_message(void *ptr, const char *account, const char *queues, const char *id, const uint8_t *body, size_t body_size, const burrow_attributes_st *attributes)
{
  burrow_backend_dummy_st *dummy = (burrow_backend_dummy_st *)ptr;

  uint8_t *body_copy = NULL;
  char *account_copy = NULL;
  char *queue_copy = NULL;
  char *id_copy = NULL;

  time_t curtime = time(NULL);
  

//  dummy_free_internals(dummy);
  
  /* most likely malloc to fail */
  body_copy = dummy->burrow->malloc_fn(dummy->burrow, body_len);
  id_copy = dummy->burrow->malloc_fn(dummy->burrow, strlen(id) + 1);
  account_copy = dummy->burrow->malloc_fn(dummy->burrow, strlen(account) + 1);
  queue_copy = dummy->burrow->malloc_fn(dummy->burrow, strlen(queue) + 1);

  if (!body_copy || !id_copy || !account_copy || !queue_copy)
  {
    dummy->burrow->free_fn(dummy->burrow, body_copy);
    dummy->burrow->free_fn(dummy->burrow, id_copy);
    dummy->burrow->free_fn(dummy->burrow, account_copy);
    dummy->burrow->free_fn(dummy->burrow, queue_copy);
    return burrow_error(BURROW_ERROR_MEMORY, "burrow_backend_dummy_create_message", "malloc body");
  }

  dummy_free_internals(dummy);

  strcpy(account_copy, account);
  strcpy(queue_copy, queue);
  strcpy(id_copy, message_id);
  memcpy(body_copy, body, body_size * sizeof(uint8_t));

  dummy->account = account_copy;
  dummy->queue = queue_copy;
  dummy->message_id = id_copy;
  dummy->body = body_copy;

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

burrow_backend_functions_st burrow_backend_dummy_functions = {
  .create = &burrow_backend_dummy_create,
  .free = &burrow_backend_dummy_free,
  .size = &burrow_backend_dummy_size,
  .clone = &burrow_backend_dummy_clone,

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
  .create_message = &burrow_backend_dummy_create_message

  .process = &burrow_backend_dummy_process,
};






