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
const void *BODY = (void *)"body";
const size_t BODY_SIZE = 5;

int main(void)
{
  burrow_st *burrow;
  burrow_attributes_st *attr;
  ssize_t size;
  
  (void)size;
  
  burrow_test("burrow_size nonsense");
  if (burrow_size("nonsense") > 0)
    burrow_test_error("returned size > 0");

  burrow_test("burrow_size NULL");
  if (burrow_size(NULL) > 0)
    burrow_test_error("returned size > 0");

  burrow_test("burrow_create nonsense");
  if ((burrow = burrow_create(NULL, "nonsense")) != NULL)
    burrow_test_error("returned non-NULL");

  burrow_test("burrow_create NULL");
  if ((burrow = burrow_create(NULL, NULL)) != NULL)
    burrow_test_error("returned non-NULL");

  /* This test may fail if the dummy is removed in the future */
  burrow_test("burrow_create dummy");
  if ((burrow = burrow_create(NULL, "dummy")) == NULL)
    burrow_test_error("returned NULL");

  burrow_test("bad params initialize");
  burrow_create_message(burrow, ACCT, QUEUE, MSGID, BODY, BODY_SIZE, NULL);
  if (burrow_process(burrow) != BURROW_OK)
    burrow_test_error("couldn't create base message");
  
  if ((attr = burrow_attributes_create(NULL, burrow)) == NULL)
    burrow_test_error("couldn't create attributes structure");
  
  burrow_attributes_set_ttl(attr, 30);
  burrow_attributes_set_hide(attr, 0);

  /* MESSAGE */

  burrow_test("burrow_create_message bad params");
  if (burrow_create_message(burrow, NULL, QUEUE, MSGID, BODY, BODY_SIZE, NULL) == BURROW_OK
      || burrow_create_message(burrow, ACCT, NULL, MSGID, BODY, BODY_SIZE, NULL) == BURROW_OK
      || burrow_create_message(burrow, ACCT, QUEUE, NULL, BODY, BODY_SIZE, NULL) == BURROW_OK
      || burrow_create_message(burrow, ACCT, QUEUE, MSGID, NULL, BODY_SIZE, NULL) == BURROW_OK)
    burrow_test_error("bad command allowed");

  burrow_test("burrow_get_message bad params");
  if (burrow_get_message(burrow, NULL, QUEUE, MSGID, NULL) == BURROW_OK
      || burrow_get_message(burrow, ACCT, NULL, MSGID, NULL) == BURROW_OK
      || burrow_get_message(burrow, ACCT, QUEUE, NULL, NULL) == BURROW_OK)
    burrow_test_error("bad command allowed");

  burrow_test("burrow_delete_message bad params");
  if (burrow_delete_message(burrow, NULL, QUEUE, MSGID, NULL) == BURROW_OK
      || burrow_delete_message(burrow, ACCT, NULL, MSGID, NULL) == BURROW_OK
      || burrow_delete_message(burrow, ACCT, QUEUE, NULL, NULL) == BURROW_OK)
    burrow_test_error("bad command allowed");

  burrow_test("burrow_update_message bad params");
  if (burrow_update_message(burrow, NULL, QUEUE, MSGID, attr, NULL) == BURROW_OK
      || burrow_update_message(burrow, ACCT, NULL, MSGID, attr, NULL) == BURROW_OK
      || burrow_update_message(burrow, ACCT, QUEUE, NULL, attr, NULL) == BURROW_OK
      || burrow_update_message(burrow, ACCT, QUEUE, MSGID, NULL, NULL) == BURROW_OK)
    burrow_test_error("bad command allowed");

  /* MESSAGES */

  burrow_test("burrow_get_messages bad params");
  if (burrow_get_messages(burrow, NULL, QUEUE, NULL) == BURROW_OK
      || burrow_get_messages(burrow, ACCT, NULL, NULL) == BURROW_OK)
    burrow_test_error("bad command allowed");

  burrow_test("burrow_delete_messages bad params");
  if (burrow_delete_messages(burrow, NULL, QUEUE, NULL) == BURROW_OK
      || burrow_delete_messages(burrow, ACCT, NULL, NULL) == BURROW_OK)
    burrow_test_error("bad command allowed");

  burrow_test("burrow_update_messages bad params");
  if (burrow_update_messages(burrow, NULL, QUEUE, attr, NULL) == BURROW_OK
      || burrow_update_messages(burrow, ACCT, NULL, attr, NULL) == BURROW_OK
      || burrow_update_messages(burrow, ACCT, QUEUE, NULL, NULL) == BURROW_OK)
    burrow_test_error("bad command allowed");

  /* QUEUES */
  
  burrow_test("burrow_get_queues bad params");
  if (burrow_get_queues(burrow, NULL, NULL) == BURROW_OK)
    burrow_test_error("bad command allowed");

  burrow_test("burrow_delete_message bad params");
  if (burrow_delete_queues(burrow, NULL, NULL) == BURROW_OK)
    burrow_test_error("bad command allowed");

  /* no bad commands to check for accounts */

  burrow_test("burrow_destroy dummy");
  burrow_destroy(burrow);
  
  /* set options, get options */
  /* set callbacks, test callbacks */
  /* set malloc/free, test malloc/free */
}
