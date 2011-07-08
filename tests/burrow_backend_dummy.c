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

const char *ACCT = "my_acct";
const char *QUEUE = "my_queue";
const char *MSGID = "my_msg";
const uint8_t *MSGBODY = (uint8_t*)"a message";
const size_t MSGBODY_SIZE = 10;

int message_callback_called;
int queues_callback_called;
int accounts_callback_called;
int complete_callback_called;
int custom_malloc_called;
int custom_free_called;

static void message_callback(burrow_st *burrow, const char *message_id, const uint8_t *body, ssize_t body_size, const burrow_attributes_st *attributes)
{
  (void) burrow;
  if (!body)
    body = (uint8_t*)"";
  printf("message_callback called: id: %s, body: %s, body_size: %d, ttl %d, hide %d\n", message_id, body, body_size, burrow_attributes_get_ttl(attributes), burrow_attributes_get_hide(attributes));
  message_callback_called++;
}

static void queues_callback(burrow_st *burrow, char **queues, size_t size)
{
  (void) burrow;
  printf("queues callback called: size %d, queues: ", size);
  do {
    printf(" %s", queues[size]);
  } while (size--);
  printf("\n");
  queues_callback_called++;
}

static void accounts_callback(burrow_st *burrow, char **accounts, size_t size)
{
  (void) burrow;
  printf("accounts callback called: size %d, accounts: ", size);
  do {
    printf(" %s", accounts[size]);
  } while (size--);
  printf("\n");
  accounts_callback_called++;
}

static void complete_feedback(burrow_st *burrow)
{
  (void) burrow;
  printf("complete callback called\n");
  complete_callback_called++;
  
  switch(complete_callback_called) {
  case 1:
    burrow_test("burrow_get_accounts feedback");
    if (burrow_get_accounts(burrow, NULL) != BURROW_OK)
      burrow_test_error("failed");
    break;

  case 2:
    burrow_test("burrow_get_queues feedback");
    if (burrow_get_queues(burrow, ACCT, NULL) != BURROW_OK)
      burrow_test_error("failed");
    break;
  
  case 3:
    burrow_test("burrow_get_messages feedback");
    if (burrow_get_messages(burrow, ACCT, QUEUE, NULL) != BURROW_OK)
      burrow_test_error("failed");
    break;
  
  case 4:
    burrow_test("burrow_get_message feedback");
    if (burrow_get_message(burrow, ACCT, QUEUE, MSGID, NULL) != BURROW_OK)
      burrow_test_error("failed");

  case 5:
    printf("feedback complete\n");
    break;

  default:
    break;
  }
}

static void *custom_malloc(burrow_st *burrow, size_t size)
{
  (void) burrow;
  printf("custom malloc called: size %u\n", size);
  custom_malloc_called++;
  return malloc(size);
}

static void custom_free(burrow_st *burrow, void *ptr)
{
  (void) burrow;
  custom_free_called++;
  printf("custom free called: address %x\n", ptr);
  free(ptr);
}

int main(void)
{
  burrow_st *burrow;
  ssize_t size;
  
  message_callback_called = 0;
  accounts_callback_called = 0;
  queues_callback_called = 0;
  complete_callback_called = 0;
  custom_malloc_called = 0;
  custom_free_called = 0;
  
  burrow_test("burrow_size dummy");
  if ((size = burrow_size("dummy")) <= 0)
    burrow_test_error("returned <= 0 size");

  /* This test may fail if the dummy is removed in the future */
  burrow_test("burrow_create dummy");
  if ((burrow = burrow_create(NULL, "dummy")) == NULL)
    burrow_test_error("returned NULL");

  burrow_set_malloc_fn(burrow, &custom_malloc);
  burrow_set_free_fn(burrow, &custom_free);

  burrow_add_options(burrow, BURROW_OPT_AUTOPROCESS);

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

  burrow_test("burrow_free dummy");
  burrow_free(burrow);
}
