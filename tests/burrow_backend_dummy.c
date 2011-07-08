/*
 * libburrow/tests -- Burrow Client Library Unit Tests
 *
 * Copyright (C) 2011 Tony Wooster (twooster@gmail.com)
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in this directory for full text.
 */

/**
 * @file
 * @brief Burrow_st tests
 */

#include "tests/common.h"
#include <unistd.h>

const char *ACCT = "my_acct";
const char *QUEUE = "my_queue";
const char *MSGID = "my_msg";
const uint8_t *MSGBODY = (uint8_t*)"a message";
const size_t MSGBODY_SIZE = 10;

struct client_st {
  int message_callback_called;
  int queues_callback_called;
  int accounts_callback_called;
  int complete_callback_called;
  int custom_malloc_called;
  int custom_free_called;
  
  int state;
};
 
typedef struct client_st client_st;

static void message_callback(burrow_st *burrow, const char *message_id, const uint8_t *body, ssize_t body_size, const burrow_attributes_st *attributes)
{
  client_st *client = burrow_get_context(burrow);
  client->message_callback_called++;
  if (!body)
    body = (uint8_t*)"";
  printf("message_callback called(%d): id: %s, body: %s, body_size: %d, ttl %d, hide %d\n",
         client->message_callback_called, message_id, body, body_size,
         burrow_attributes_get_ttl(attributes), burrow_attributes_get_hide(attributes));
}

static void queues_callback(burrow_st *burrow, char **queues, size_t size)
{
  client_st *client = burrow_get_context(burrow);
  client->queues_callback_called++;
  printf("queues callback called(%d): size %d, queues: ", client->queues_callback_called, size);
  while (size--)
    printf(" %s", queues[size]);
  printf("\n");
}

static void accounts_callback(burrow_st *burrow, char **accounts, size_t size)
{
  client_st *client = burrow_get_context(burrow);
  client->accounts_callback_called++;
  printf("accounts callback called(%d): size %d, accounts: ", client->accounts_callback_called, size);
  while (size--)
    printf(" %s", accounts[size]);
  printf("\n");
}

static void complete_feedback(burrow_st *burrow)
{
  client_st *client = burrow_get_context(burrow);
  client->complete_callback_called++;
  printf("complete callback called(%d)\n", client->complete_callback_called);
  
  switch(++(client->state)) {
  case 1:
    burrow_get_accounts(burrow, NULL);
    break;

  case 2:
    burrow_get_queues(burrow, ACCT, NULL);
    break;
  
  case 3:
    burrow_get_messages(burrow, ACCT, QUEUE, NULL);
    break;
  
  case 4:
    sleep(2);
    burrow_get_message(burrow, ACCT, QUEUE, MSGID, NULL);

  case 5:
    printf("feedback complete\n");
    break;

  default:
    break;
  }
}

static void *custom_malloc(burrow_st *burrow, size_t size)
{
  client_st *client = burrow_get_context(burrow);
  client->custom_malloc_called++;
  printf("custom malloc called(%d): size %u\n", client->custom_malloc_called, size);
  return malloc(size);
}

static void custom_free(burrow_st *burrow, void *ptr)
{
  client_st *client = burrow_get_context(burrow);
  client->custom_free_called++;
  printf("custom free called(%d): address %x\n", client->custom_free_called, ptr);
  free(ptr);
}

int main(void)
{
  burrow_st *burrow;
  ssize_t size;
  struct client_st client;
  burrow_attributes_st *attr;
  
  memset(&client, 0, sizeof(client_st));
  
  burrow_test("burrow_size dummy");
  if ((size = burrow_size("dummy")) <= 0)
    burrow_test_error("returned <= 0 size");

  /* Autoprocess tests */
  /* This test may fail if the dummy is removed in the future */
  burrow_test("burrow_create dummy");
  if ((burrow = burrow_create(NULL, "dummy")) == NULL)
    burrow_test_error("returned NULL");

  burrow_test("autoprocess initialization");

  burrow_set_context(burrow, &client);
  if (burrow_get_context(burrow) != &client)
    burrow_test_error("failed");

  burrow_set_malloc_fn(burrow, &custom_malloc);
  burrow_set_free_fn(burrow, &custom_free);

  if ((attr = burrow_attributes_create(NULL, burrow)) == NULL)
    burrow_test_error("failed");

  burrow_add_options(burrow, BURROW_OPT_AUTOPROCESS);
  if ((burrow_get_options(burrow) & BURROW_OPT_AUTOPROCESS) != BURROW_OPT_AUTOPROCESS)
    burrow_test_error("failed");

  burrow_test("burrow_create_message autoprocess");
  if (burrow_create_message(burrow, ACCT, QUEUE, MSGID, MSGBODY, MSGBODY_SIZE, NULL) != BURROW_OK)
    burrow_test_error("failed");

  burrow_test("burrow_get_accounts autoprocess");
  if (burrow_get_accounts(burrow, NULL) != BURROW_OK)
    burrow_test_error("failed");

  burrow_test("burrow_get_queues autoprocess");
  if (burrow_get_queues(burrow, ACCT, NULL) != BURROW_OK)
    burrow_test_error("failed");

  burrow_test("burrow_get_messages autoprocess");
  if (burrow_get_messages(burrow, ACCT, QUEUE, NULL) != BURROW_OK)
    burrow_test_error("failed");

  burrow_test("burrow_get_message autoprocess");
  if (burrow_get_message(burrow, ACCT, QUEUE, MSGID, NULL) != BURROW_OK)
    burrow_test_error("failed");

  burrow_remove_options(burrow, BURROW_OPT_AUTOPROCESS);

  burrow_test("burrow_free dummy");
  burrow_free(burrow);

  /* Feedback tests */

  burrow_test("burrow_create dummy");
  if ((burrow = burrow_create(NULL, "dummy")) == NULL)
    burrow_test_error("returned NULL");

  burrow_set_context(burrow, &client);

  burrow_set_malloc_fn(burrow, &custom_malloc);
  burrow_set_free_fn(burrow, &custom_free);

  burrow_test("burrow set message, queues, accounts, complete");
  burrow_set_message_fn(burrow, &message_callback);
  burrow_set_queues_fn(burrow, &queues_callback);
  burrow_set_accounts_fn(burrow, &accounts_callback);
  burrow_set_complete_fn(burrow, &complete_feedback);

  burrow_test("burrow_create_message manual feedback");
  if (burrow_create_message(burrow, ACCT, QUEUE, MSGID, MSGBODY, MSGBODY_SIZE, NULL) != BURROW_OK)
    burrow_test_error("failed");

  burrow_test("burrow_process");
  if (burrow_process(burrow) != BURROW_OK)
    burrow_test_error("returned not BURROW_OK");
    
  client.state = 0;
  
  burrow_add_options(burrow, BURROW_OPT_AUTOPROCESS);
  burrow_test("burrow_create_message manual feedback autoprocess");
  
  attr = burrow_attributes_create(NULL, burrow);
  burrow_attributes_set_ttl(attr, 1);
  
  burrow_create_message(burrow, ACCT, QUEUE, MSGID, MSGBODY, MSGBODY_SIZE, attr);
  /*if (burrow_create_message(burrow, ACCT, QUEUE, MSGID, MSGBODY, MSGBODY_SIZE, NULL) != BURROW_OK)
    burrow_test_error("failed");*/
  
  burrow_remove_options(burrow, BURROW_OPT_AUTOPROCESS);
  
  burrow_test("burrow_free dummy");
  burrow_free(burrow);
}
