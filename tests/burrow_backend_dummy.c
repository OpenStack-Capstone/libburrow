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

typedef enum {
  EXPECT_NONE = 0,
  
  EXPECT_QUEUES_CALLED  = (1 << 0),
  EXPECT_QUEUES_MATCH   = EXPECT_QUEUES_CALLED | (1 << 1),
//  EXPECT_QUEUES         = EXPECT_QUEUES_NOCALL | EXPECT_QUEUES_MATCH | EXPECT_QUEUES_NOMATCH, 


  EXPECT_ACCTS_CALLED   = (1 << 2),
  EXPECT_ACCTS_MATCH    = EXPECT_ACCTS_CALLED | (1 << 3),
//  EXPECT_ACCTS          = EXPECT_ACCTS_NOCALL | EXPECT_ACCTS_MATCH | EXPECT_ACCTS_NOMATCH, 

  EXPECT_MSG_CALLED     = (1 << 4),
  EXPECT_MSG_MATCH      = EXPECT_MSG_CALLED | (1 << 5),
/*  EXPECT_MSG            = EXPECT_MSG_NOCALL | EXPECT_MSG_MATCH | EXPECT_MSG_NOMATCH, 
  
  EXPECT_NEGATIVES = EXPECT_ACCTS_NOCALL | EXPECT_ACCTS_NOMATCH
                   | EXPECT_QUEUES_NOCALL | EXPECT_QUEUES_NOMATCH
                   | EXPECT_MSG_NOCALL | EXPECT_MSG_NOMATCH, */
} expectation_t;

struct client_st {
  int message_callback_called;
  int queues_callback_called;
  int accounts_callback_called;
  int complete_callback_called;
  int custom_malloc_called;
  int custom_free_called;
  
  int state;

  expectation_t must, must_not;
  expectation_t result;
  
  const char *acct;
  const char *queue;
  const char *msgid;
  size_t body_size;
  const uint8_t *body;
  
};
 
typedef struct client_st client_st;

static void message_callback(burrow_st *burrow, const char *message_id, const uint8_t *body, ssize_t body_size, const burrow_attributes_st *attributes)
{
  client_st *client = burrow_get_context(burrow);
  client->message_callback_called++;
  
  printf("message_callback called(%d): id: %s, body: %s, body_size: %d, ttl %d, hide %d\n",
         client->message_callback_called, message_id, (body ? (const char *)body : ""), body_size,
         burrow_attributes_get_ttl(attributes), burrow_attributes_get_hide(attributes));

  client->result |= EXPECT_MSG_CALLED;
  
  if (!strcmp(message_id, client->msgid) &&
      (!body || (body_size == (ssize_t)client->body_size && !memcmp(body, client->body, body_size) )) )
    client->result |= EXPECT_MSG_MATCH;
}

static void queues_callback(burrow_st *burrow, char **queues, size_t size)
{
  client_st *client = burrow_get_context(burrow);
  int i;

  client->queues_callback_called++;

  i = size;
  printf("queues callback called(%d): size %d, queues: ", client->queues_callback_called, size);
  while (i--)
    printf(" %s", queues[i]);
  printf("\n");

  client->result |= EXPECT_QUEUES_CALLED;

  i = (int)size;
  while (i--)
    if (!strcmp(client->queue, queues[i])) {
      client->result |= EXPECT_QUEUES_MATCH;
      break;
    }
}

static void accounts_callback(burrow_st *burrow, char **accounts, size_t size)
{
  client_st *client = burrow_get_context(burrow);
  int i;

  client->accounts_callback_called++;

  i = (int)size;
  printf("accounts callback called(%d): size %d, accounts: ", client->accounts_callback_called, size);
  while (i--)
    printf(" %s", accounts[i]);
  printf("\n");

  client->result |= EXPECT_ACCTS_CALLED;

  i = (int)size;
  while (i--)
    if (!strcmp(client->acct, accounts[i])) {
      client->result |= EXPECT_ACCTS_MATCH;
      break;
    }
}

static void complete_feedback(burrow_st *burrow)
{
  client_st *client = burrow_get_context(burrow);
  client->complete_callback_called++;
  printf("complete callback called(%d)\n", client->complete_callback_called);
  
/*  switch(++(client->state)) {
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
  }*/
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

static void client_expect(client_st *client, expectation_t must, expectation_t must_not)
{
  client->must = must;
  client->must_not = must_not;
  client->result = EXPECT_NONE;
}

static void client_only(client_st *client, expectation_t only)
{
  client_expect(client, only, ~only);
}

/*static void client_may(client_st *client, expectation_t may)
{
  client_expect(client, may, 0);
}*/

static bool client_passed(client_st *client)
{
  return ((client->result & client->must) == client->must) && ((client->result & client->must_not) == 0);
}

int main(void)
{
  burrow_st *burrow;
  ssize_t size;
  client_st *client = malloc(sizeof(client_st));
  burrow_attributes_st *attr;
  
  memset(client, 0, sizeof(client_st));
  client->acct = "my acct";
  client->queue = "my queue";
  client->msgid = "my messageid";
  client->body = (uint8_t *)"msg body";
  client->body_size = strlen((char*)client->body) + 1;
  
  burrow_test("burrow_size dummy");
  if ((size = burrow_size("dummy")) <= 0)
    burrow_test_error("returned <= 0 size");

  /* Autoprocess tests */
  /* This test may fail if the dummy is removed in the future */
  burrow_test("burrow_create dummy");
  if ((burrow = burrow_create(NULL, "dummy")) == NULL)
    burrow_test_error("returned NULL");

  burrow_test("autoprocess initialization");

  burrow_set_context(burrow, client);
  if (burrow_get_context(burrow) != client)
    burrow_test_error("failed");

  burrow_set_malloc_fn(burrow, &custom_malloc);
  burrow_set_free_fn(burrow, &custom_free);

  burrow_set_message_fn(burrow, &message_callback);
  burrow_set_queues_fn(burrow, &queues_callback);
  burrow_set_accounts_fn(burrow, &accounts_callback);
  burrow_set_complete_fn(burrow, &complete_feedback);

  if ((attr = burrow_attributes_create(NULL, burrow)) == NULL)
    burrow_test_error("failed");

  burrow_add_options(burrow, BURROW_OPT_AUTOPROCESS);
  if ((burrow_get_options(burrow) & BURROW_OPT_AUTOPROCESS) != BURROW_OPT_AUTOPROCESS)
    burrow_test_error("failed");

  burrow_test("burrow_create_message autoprocess");
  client_only(client, EXPECT_NONE);
  burrow_create_message(burrow, client->acct, client->queue, client->msgid, client->body, client->body_size, NULL);
  if (!client_passed(client))
    burrow_test_error("failed");

  burrow_test("burrow_get_accounts");
  client_only(client, EXPECT_ACCTS_MATCH);
  burrow_get_accounts(burrow, NULL);
  if (!client_passed(client))
    burrow_test_error("failed");

  burrow_test("burrow_get_queues");
  client_only(client, EXPECT_QUEUES_MATCH);
  burrow_get_queues(burrow, client->acct, NULL);
  if (!client_passed(client))
    burrow_test_error("failed");

  burrow_test("burrow_get_messages");
  client_only(client, EXPECT_MSG_MATCH);
  burrow_get_messages(burrow, client->acct, client->queue, NULL);
  if (!client_passed(client))
    burrow_test_error("failed");

  burrow_test("burrow_get_message");
  client_only(client, EXPECT_MSG_MATCH);
  burrow_get_message(burrow, client->acct, client->queue, client->msgid, NULL);
  if (!client_passed(client))
    burrow_test_error("failed");


  burrow_remove_options(burrow, BURROW_OPT_AUTOPROCESS);

  burrow_test("burrow_free dummy");
  burrow_free(burrow);

  /* Feedback tests */


#if 0
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
  if (burrow_create_message(burrow, ACCT, QUEUE, MSGID, BODY, BODY_SIZE, NULL) != BURROW_OK)
    burrow_test_error("failed");

  burrow_test("burrow_process");
  if (burrow_process(burrow) != BURROW_OK)
    burrow_test_error("returned not BURROW_OK");
    
  client->state = 0;
  
  burrow_add_options(burrow, BURROW_OPT_AUTOPROCESS);
  burrow_test("burrow_create_message manual feedback autoprocess");
  
  attr = burrow_attributes_create(NULL, burrow);
  burrow_attributes_set_ttl(attr, 1);
  
  burrow_create_message(burrow, ACCT, QUEUE, MSGID, BODY, BODY_SIZE, attr);
  /*if (burrow_create_message(burrow, ACCT, QUEUE, MSGID, BODY, BODY_SIZE, NULL) != BURROW_OK)
    burrow_test_error("failed");*/
  
  burrow_remove_options(burrow, BURROW_OPT_AUTOPROCESS);
  
  burrow_test("burrow_free dummy");
  burrow_free(burrow);
#endif
  free(client);
}
