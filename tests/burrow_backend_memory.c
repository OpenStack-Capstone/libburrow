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
  
  CALL_QUEUE        = (1 << 0),
  MATCH_QUEUE_ONLY  = (1 << 1),
  MULT_QUEUE_ONLY   = (1 << 2),
  MATCH_QUEUE       = CALL_QUEUE | MATCH_QUEUE_ONLY,
  MULT_QUEUE        = CALL_QUEUE | MULT_QUEUE_ONLY,

  NO_CALL_QUEUE     = CALL_QUEUE,
  NO_MATCH_QUEUE    = MATCH_QUEUE_ONLY,
  NO_MULT_QUEUE     = MULT_QUEUE_ONLY,
                
  CALL_ACCT         = (1 << 3),
  MATCH_ACCT_ONLY   = (1 << 4),
  MULT_ACCT_ONLY    = (1 << 5),
  MATCH_ACCT        = CALL_ACCT | MATCH_ACCT_ONLY,
  MULT_ACCT         = CALL_ACCT | MULT_ACCT_ONLY,

  NO_CALL_ACCT      = CALL_ACCT,
  NO_MATCH_ACCT     = MATCH_ACCT_ONLY,
  NO_MULT_ACCT      = MULT_ACCT_ONLY,
                    
  CALL_MSG          = (1 << 6),
  MATCH_MSG_ONLY    = (1 << 7),
  MULT_MSG_ONLY     = (1 << 8),
  MATCH_MSG         = CALL_MSG | MATCH_MSG_ONLY,
  MULT_MSG          = CALL_MSG | MULT_MSG_ONLY,

  NO_CALL_MSG       = CALL_MSG,
  NO_MATCH_MSG      = MATCH_MSG_ONLY,
  NO_MULT_MSG       = MULT_MSG_ONLY,
                    
  LOG_ERROR         = (1 << 9),
  NO_LOG_ERROR      = LOG_ERROR,

  EXPECT_ALL        = ~EXPECT_NONE
} expectation_t;

typedef enum {
  VERBOSE_DEBUG,
  VERBOSE_WARN,
  VERBOSE_ERROR,
  VERBOSE_NONE,
} verbose_t;

struct client_st {
  int message_callback_called;
  int queue_callback_called;
  int account_callback_called;
  int complete_callback_called;
  int custom_malloc_called;
  int custom_free_called;
  
  expectation_t must, must_not;
  expectation_t result;
  
  verbose_t verbose;
  
  const char *acct;
  const char *queue;
  const char *msgid;
  size_t body_size;
  void *body;
};
 
typedef struct client_st client_st;

static void log_callback(burrow_st *burrow, burrow_verbose_t error_level, const char *msg)
{
  client_st *client = burrow_get_context(burrow);
  if (error_level >= BURROW_VERBOSE_ERROR)
    client->result |= LOG_ERROR;

  if ((error_level >= BURROW_VERBOSE_ERROR && VERBOSE_ERROR >= client->verbose) ||
      (error_level >= BURROW_VERBOSE_WARN && VERBOSE_WARN >= client->verbose) ||
      (error_level >= BURROW_VERBOSE_DEBUG && VERBOSE_DEBUG >= client->verbose))
    printf("log_callback: %s\n", msg);
}

static void message_callback(burrow_st *burrow, const char *message_id, const void *body, size_t body_size, const burrow_attributes_st *attributes)
{
  client_st *client = burrow_get_context(burrow);
  client->message_callback_called++;
  
  if (VERBOSE_DEBUG >= client->verbose)
    printf("message_callback called(%d): id: '%s', body: '%s', body_size: %u, ttl %d, hide %d\n",
           client->message_callback_called, message_id, (body ? (const char *)body : ""), body_size,
           burrow_attributes_get_ttl(attributes), burrow_attributes_get_hide(attributes));

  if (client->result & CALL_MSG)
    client->result |= MULT_MSG_ONLY;
  else
    client->result |= CALL_MSG;
  
  if (!strcmp(message_id, client->msgid) &&
      (!body || (body_size == client->body_size && !memcmp(body, client->body, body_size) )) )
    client->result |= MATCH_MSG_ONLY;
}

static void queue_callback(burrow_st *burrow, const char *queue)
{
  client_st *client = burrow_get_context(burrow);

  client->queue_callback_called++;

  if (client->result & CALL_QUEUE)
    client->result |= MULT_QUEUE_ONLY;
  else
    client->result |= CALL_QUEUE;

  if (VERBOSE_DEBUG >= client->verbose)
    printf("queue callback called(%d): queue: '%s'\n", client->queue_callback_called, queue);

  if (!strcmp(client->queue, queue)) {
    client->result |= MATCH_QUEUE_ONLY;
  }
}

static void account_callback(burrow_st *burrow, const char *account)
{
  client_st *client = burrow_get_context(burrow);

  client->account_callback_called++;

  if (client->result & CALL_ACCT)
    client->result |= MULT_ACCT_ONLY;
  else
    client->result |= CALL_ACCT;

  if (VERBOSE_DEBUG >= client->verbose)
    printf("account callback called(%d): account: '%s'\n", client->account_callback_called, account);

  if (!strcmp(client->acct, account)) {
    client->result |= MATCH_ACCT_ONLY;
  }
}

static void complete_feedback(burrow_st *burrow)
{
  client_st *client = burrow_get_context(burrow);
  client->complete_callback_called++;
  if (VERBOSE_DEBUG >= client->verbose)
    printf("complete callback called(%d)\n", client->complete_callback_called);
}

static void *custom_malloc(burrow_st *burrow, size_t size)
{
  client_st *client = burrow_get_context(burrow);
  client->custom_malloc_called++;
  if (VERBOSE_DEBUG >= client->verbose)
    printf("custom malloc called(%d): size %u\n", client->custom_malloc_called, size);
  return malloc(size);
}

static void custom_free(burrow_st *burrow, void *ptr)
{
  client_st *client = burrow_get_context(burrow);
  client->custom_free_called++;
  if (VERBOSE_DEBUG >= client->verbose)
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

/*static void client_must(client_st *client, expectation_t must)
{
  client_expect(client, must, 0);
}*/

static void client_may(client_st *client, expectation_t may_only)
{
  client_expect(client, 0, EXPECT_ALL ^ may_only);
}

static void client_should_may(client_st *client, expectation_t should, expectation_t may)
{
  client_expect(client, should, EXPECT_ALL ^ (should | may) );
}

static bool client_passed(client_st *client)
{
  return ((client->result & client->must) == client->must) && ((client->result & client->must_not) == 0);
}


static void print_expectation(expectation_t e)
{
  const struct {
    expectation_t e;
    const char *s;
  } descriptions[] = {
    {CALL_QUEUE, "CALL_QUEUE"},
    {MATCH_QUEUE_ONLY, "MATCH_QUEUE"},
    {MULT_QUEUE_ONLY, "MULT_QUEUE"},
    {CALL_ACCT, "CALL_ACCT"},
    {MATCH_ACCT_ONLY, "MATCH_ACCT"},
    {MULT_ACCT_ONLY, "MULT_ACCT"},
    {CALL_MSG, "CALL_MSG"},
    {MATCH_MSG_ONLY, "MATCH_MSG"},
    {MULT_MSG_ONLY, "MULT_MSG"},
    {LOG_ERROR, "LOG_ERROR"},
  };
  const int count = sizeof(descriptions) / sizeof(descriptions[0]);
  int i = 0;
  
  for(i = 0; i < count; i++)
    if ((e & descriptions[i].e) == descriptions[i].e) {
      printf("%s", descriptions[i].s);
      break;
    }
  for(i++; i < count; i++)
    if ((e & descriptions[i].e) == descriptions[i].e) {
      printf(", %s", descriptions[i].s);
      break;
    }
}


static void _print_problems(client_st *client)
{
  if (!client_passed(client)) {
    printf("*** failed:");
    if ((client->result & client->must) != client->must) {
      printf("\n***   should've occurred but didn't:   ");
      print_expectation(client->must ^ (client->result & client->must));
    }
    if ((client->result & client->must_not) != 0) {
      printf("\n***   shouldn't have occurred but did: ");
      print_expectation(client->must_not & client->result);
    }
    printf("\n");
  }
}

#define client_check(client) \
  do { \
    if (!client_passed(client)) { \
      _print_problems(client); \
      printf("*** %s:%d *** failed\n", __FILE__, __LINE__); \
      exit(1); \
    } \
  } while(0);


int main(void)
{
  burrow_st *burrow;
  size_t size;
  client_st *client = malloc(sizeof(client_st));
  burrow_attributes_st *attr;
  
  memset(client, 0, sizeof(client_st));
  client->acct = "my acct";
  client->queue = "my queue";
  client->msgid = "my messageid";
  client->body = (void *)"msg body";
  client->body_size = strlen((char*)client->body) + 1;
  client->verbose = VERBOSE_DEBUG;
  
  burrow_test("burrow_size memory");

    if ((size = burrow_size("memory")) <= 0)
      burrow_test_error("returned <= 0 size");

  /* Autoprocess tests */
  /* This test may fail if the memory is removed in the future */
  burrow_test("burrow_create memory");

    if ((burrow = burrow_create(NULL, "memory")) == NULL)
      burrow_test_error("returned NULL");

  burrow_test("autoprocess initialization");

    burrow_set_context(burrow, client);
    if (burrow_get_context(burrow) != client)
      burrow_test_error("failed");

    burrow_set_malloc_fn(burrow, &custom_malloc);
    burrow_set_free_fn(burrow, &custom_free);

    burrow_set_message_fn(burrow, &message_callback);
    burrow_set_queue_fn(burrow, &queue_callback);
    burrow_set_account_fn(burrow, &account_callback);
    burrow_set_complete_fn(burrow, &complete_feedback);
    burrow_set_log_fn(burrow, &log_callback);

    if ((attr = burrow_attributes_create(NULL, burrow)) == NULL)
      burrow_test_error("returned NULL");

    burrow_add_options(burrow, BURROW_OPT_AUTOPROCESS);
    if ((burrow_get_options(burrow) & BURROW_OPT_AUTOPROCESS) != BURROW_OPT_AUTOPROCESS)
      burrow_test_error("failed");

  /* TEST SET: Create a message, and test that it creates accts, queues and msg itself */
  burrow_test("burrow_create_message autoprocess");

    /* Create message */
    client_may(client, MATCH_MSG);
    burrow_create_message(burrow, client->acct, client->queue, client->msgid, client->body, client->body_size, NULL);
    client_check(client);

  burrow_test("burrow_get_accounts");

    /* Verify account exists; other accounts may exist */
    client_should_may(client, MATCH_ACCT, MULT_ACCT);
    burrow_get_accounts(burrow, NULL);
    client_check(client);
  
  burrow_test("burrow_get_queues");

    /* Verify queue exists; other queues may exist */
    client_should_may(client, MATCH_QUEUE, MULT_QUEUE);
    burrow_get_queues(burrow, client->acct, NULL);
    client_check(client);

  burrow_test("burrow_get_messages");

    /* Verify message exists; other messages may exist */
    client_should_may(client, MATCH_MSG, MULT_MSG);
    burrow_get_messages(burrow, client->acct, client->queue, NULL);
    client_check(client);

  burrow_test("burrow_get_message");

    /* Verify exact message exists */
    client_only(client, MATCH_MSG);
    burrow_get_message(burrow, client->acct, client->queue, client->msgid, NULL);
    client_check(client);

  /* TEST SET: Hide the message, check that group commands don't affect it, unhide and check normal behavior */

  /* hide = 10, ttl not set */
  burrow_attributes_set_hide(attr, 10);

  burrow_test("burrow_update_message hide=10");

    /* Update message to hide it */
    client_only(client, MATCH_MSG);
    burrow_update_message(burrow, client->acct, client->queue, client->msgid, attr, NULL);
    client_check(client);

  /* hide = 0, ttl = 100 */
  burrow_attributes_set_hide(attr, 0);
  burrow_attributes_set_ttl(attr, 100);

  burrow_test("burrow_update_messages hide=0 ttl=100 skip hidden");

    /* This shouldn't match, because the original message should be hidden */
    client_may(client, MULT_MSG);
    burrow_update_messages(burrow, client->acct, client->queue, attr, NULL);
    client_check(client);

  burrow_test("burrow_delete_messages skip hidden");

    /* This shouldn't match, because the original message should be hidden */
    client_may(client, MULT_MSG);
    burrow_delete_messages(burrow, client->acct, client->queue, NULL);
    client_check(client);

  burrow_test("burrow_get_message hidden");

    /* This should match because the message is referred to directly */
    client_only(client, MATCH_MSG);
    burrow_get_message(burrow, client->acct, client->queue, client->msgid, NULL);
    client_check(client);

  burrow_test("burrow_update_message hidden hide=0 ttl=100");

    /* This should match because the message is referred to directly */
    client_only(client, MATCH_MSG);
    burrow_update_message(burrow, client->acct, client->queue, client->msgid, attr, NULL);
    client_check(client);

  burrow_test("burrow_get_messages");

    /* This should match because the message is now visible */
    client_only(client, MATCH_MSG);
    burrow_get_messages(burrow, client->acct, client->queue, NULL);
    client_check(client);

  /* TEST SET: Delete the message, make sure that the acct, queue and msg are all gone */
  burrow_test("burrow_delete_message");

    /* Delete message */
    client_may(client, MATCH_MSG);
    burrow_delete_message(burrow, client->acct, client->queue, client->msgid, NULL);
    client_check(client);

  burrow_test("burrow_get_accounts");
    
    /* May return any number of accounts, but may not match */
    client_may(client, MULT_ACCT);
    burrow_get_accounts(burrow, NULL);
    client_check(client);

  burrow_test("burrow_get_queues");

    /* May return any number of queues, but may not match */
    client_may(client, MULT_QUEUE);
    burrow_get_queues(burrow, client->acct, NULL);
    client_check(client);

  burrow_test("burrow_get_messages");

    /* May return any number of messages, but may not match */
    client_may(client, MULT_MSG);
    burrow_get_messages(burrow, client->acct, client->queue, NULL);
    client_check(client);

  burrow_test("burrow_get_message");

    /* No callbacks should occur */
    client_only(client, EXPECT_NONE);
    burrow_get_message(burrow, client->acct, client->queue, client->msgid, NULL);
    client_check(client);

  /* TEST SET: Create, verify, delete messages, verify no messages, verify no exact message */

  burrow_test("burrow_create_message");

    client_may(client, MATCH_MSG);
    burrow_create_message(burrow, client->acct, client->queue, client->msgid, client->body, client->body_size, NULL);
    client_check(client);

  burrow_test("burrow_get_message");

    /* Verify existence */
    client_only(client, MATCH_MSG);
    burrow_get_message(burrow, client->acct, client->queue, client->msgid, NULL);
    client_check(client);

  burrow_test("burrow_delete_messages");
    
    /* Delete all messages */
    client_may(client, MATCH_MSG | MULT_MSG);
    burrow_delete_messages(burrow, client->acct, client->queue, NULL);
    client_check(client);

  burrow_test("burrow_get_messages");

    /* All messages have been deleted; no callbacks expected */
    client_only(client, EXPECT_NONE);
    burrow_get_messages(burrow, client->acct, client->queue, NULL);
    client_check(client);

  burrow_test("burrow_get_message");

    /* All messages have been deleted; no callbacks expected */
    client_only(client, EXPECT_NONE);
    burrow_get_message(burrow, client->acct, client->queue, client->msgid, NULL);
    client_check(client);

  burrow_test("burrow_get_queues");

    /* Some queues may exist, but not the one whose contents were just deleted */
    client_may(client, MULT_QUEUE);
    burrow_get_queues(burrow, client->acct, NULL);
    client_check(client);

  /* TEST SET: Create, verify, delete queues, verify no messages, verify no exact message, verify no queues */

  burrow_test("burrow_create_message");

    /* Create */
    client_may(client, MATCH_MSG);
    burrow_create_message(burrow, client->acct, client->queue, client->msgid, client->body, client->body_size, NULL);
    client_check(client);

  burrow_test("burrow_get_message");

    /* Verify existence */
    client_only(client, MATCH_MSG);
    burrow_get_message(burrow, client->acct, client->queue, client->msgid, NULL);
    client_check(client);

  burrow_test("burrow_delete_queues");

    /* Delete all queues -- should return nothing */
    client_only(client, EXPECT_NONE);
    burrow_delete_queues(burrow, client->acct, NULL);
    client_check(client);

  burrow_test("burrow_get_messages");

    /* All queues/messages have been deleted, so no callbacks should occur */
    client_only(client, EXPECT_NONE);
    burrow_get_messages(burrow, client->acct, client->queue, NULL);
    client_check(client);

  burrow_test("burrow_get_message");

    /* All queues/messages have been deleted, so no callbacks should occur */
    client_only(client, EXPECT_NONE);
    burrow_get_message(burrow, client->acct, client->queue, client->msgid, NULL);
    client_check(client);

  burrow_test("burrow_get_queues");

    /* All queues/messages have been deleted, so no callbacks should occur */
    client_only(client, EXPECT_NONE);
    burrow_get_queues(burrow, client->acct, NULL);
    client_check(client);

  burrow_test("burrow_get_accounts");

    /* Upon all queues deleted, the account should no longer exist, so no match accepted */
    client_may(client, MULT_ACCT);
    burrow_get_accounts(burrow, NULL);
    client_check(client);

  /* TEST SET: Create, verify, delete queues, verify no messages, verify no exact message, verify no queues */

  burrow_test("burrow_create_message");

    /* Create */
    client_may(client, MATCH_MSG);
    burrow_create_message(burrow, client->acct, client->queue, client->msgid, client->body, client->body_size, NULL);
    client_check(client);

  burrow_test("burrow_get_message");

    /* Verify existence */
    client_only(client, MATCH_MSG);
    burrow_get_message(burrow, client->acct, client->queue, client->msgid, NULL);
    client_check(client);

  burrow_test("burrow_delete_accounts");

    /* No callbacks expected on delete_accounts */
    client_only(client, EXPECT_NONE);
    burrow_delete_accounts(burrow, NULL);
    client_check(client);

  burrow_test("burrow_get_messages");

    /* All accounts/queues/msgs have been deleted, no callbacks expected */
    client_only(client, EXPECT_NONE);
    burrow_get_messages(burrow, client->acct, client->queue, NULL);
    client_check(client);

  burrow_test("burrow_get_message");

    /* All accounts/queues/msgs have been deleted, no callbacks expected */
    client_only(client, EXPECT_NONE);
    burrow_get_message(burrow, client->acct, client->queue, client->msgid, NULL);
    client_check(client);

  burrow_test("burrow_get_queues");

    /* All accounts/queues/msgs have been deleted, no callbacks expected */
    client_only(client, EXPECT_NONE);
    burrow_get_queues(burrow, client->acct, NULL);
    client_check(client);

  burrow_test("burrow_get_accounts");

    /* All accounts/queues/msgs have been deleted, no callbacks expected */
    client_only(client, EXPECT_NONE);
    burrow_get_accounts(burrow, NULL);
    client_check(client);

  burrow_remove_options(burrow, BURROW_OPT_AUTOPROCESS);

  burrow_test("burrow_destroy memory");
  burrow_destroy(burrow);

  free(client);
}