/* Blocking case -- pseudocode provided by Eric Day */
#include <libburrow/burrow.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

typedef struct msg_st {
  char *msg_id;
  const uint8_t *body;
  size_t body_size;
  
  struct msg_st *next;
} msg_st;

typedef struct client_st
{
  int return_code;
  int message_count;

  char *account;
  char *queue;
  
  msg_st *messages;
  msg_st *current_message;
} client_st;

static void _complete(burrow_st *burrow)
{
  client_st *client = burrow_get_context(burrow);

  if (client->current_message == NULL) {
    printf("Done whatever we were doing\n");
    return;
  }

  printf("done: %s/%s/%s: %s\n", client->account, client->queue, client->current_message->msg_id, client->current_message->body);

  client->current_message = client->current_message->next;
  if (client->current_message == NULL)
    return;

  /* Since this only sets next start state, we don't have to worry about recursion. When
     this callback returns the loop continues with this next message. */
  burrow_create_message(burrow, client->account, client->queue,
    client->current_message->msg_id, client->current_message->body, client->current_message->body_size, NULL);
}

static void _log(burrow_st *burrow, burrow_verbose_t verbose, const char *message)
{
  client_st *client = burrow_get_context(burrow);
  if (verbose >= BURROW_VERBOSE_ERROR)
  {
    printf("Error: %s", message);
    client->return_code = 1;
  }
}

static void _message(burrow_st * burrow,
	      const char *message_id,
	      const void *body,
	      size_t body_size,
	      const burrow_attributes_st *attributes)
{
  (void) burrow;
  (void) message_id;
  (void) body;
  (void) body_size;
  (void) attributes;
  fprintf(stderr, "_message: called, msgid: '%s', body size %d, body = \"%s\"\n",
	  (message_id ? message_id : "UNSET"),
	  body_size, (char *)body);
  if (attributes) {
    if (burrow_attributes_isset_ttl(attributes))
      fprintf(stderr, "\tttl = %d\n", burrow_attributes_get_ttl(attributes));
    if (burrow_attributes_isset_hide(attributes))
      fprintf(stderr, "\thide = %d\n", burrow_attributes_get_hide(attributes));
  }
}

static void _account(burrow_st *burrow, const char *account) {
  (void) burrow;
  fprintf(stderr, "_account() called, got account = \"%s\"\n", account);
}

int main(int argc, char **argv)
{
  burrow_st *burrow;
  client_st client;
  msg_st *msg;
  int use_http = 0;

  argc--;
  argv++;

  if (strcmp(argv[0], "http") == 0) {
    use_http = 1;
    argc--;
    argv++;
  }

  if (argc < 4 || argc % 2 != 0)
    return -1;

  client.account = argv[0];
  client.queue = argv[1];
  argc -= 2;
  argv += 2;

  client.messages = NULL;
  client.message_count = 0;

  while (argc) {
    msg = malloc(sizeof(msg_st));
    if (!msg)
      return -2;

    msg->msg_id = argv[0];
    msg->body = (uint8_t *)argv[1];
    msg->body_size = strlen(argv[1]);
    msg->next = client.messages;
    client.messages = msg;
    client.message_count++;

    argc -= 2;
    argv += 2;
  }

  client.return_code = 0;
  client.current_message = client.messages;

  if (use_http == 0) {
    burrow = burrow_create(NULL, "dummy");
    printf("burrow = %p\n", burrow);
  } else {
    burrow = burrow_create(NULL, "http");
    printf("burrow = %p\n", burrow);
    burrow_backend_set_option(burrow, "server", "localhost");
    burrow_backend_set_option(burrow, "port", "8080");
  }

  burrow_set_context(burrow, &client);
  burrow_set_log_fn(burrow, &_log);
  burrow_set_message_fn(burrow, &_message);

  (void) _complete;
  /* Insert the first one here to kick the loop off. This only sets start state,
     it doesn't run the loop. */
  burrow_set_complete_fn(burrow, &_complete);
  msg = client.current_message;
  printf("Calling create_message(%s,%s,%s,\"%s\",..)\n\n", client.account,
	 client.queue, msg->msg_id, msg->body);
  burrow_create_message(burrow, client.account, client.queue,
  			msg->msg_id, msg->body, msg->body_size, NULL);

  /* This runs until there are no more tasks. */
  burrow_process(burrow);

  printf("Ok, now let us see what messages the server has\n");
  /* Now see what the server has */
  burrow_get_messages(burrow, client.account, client.queue, NULL);
  burrow_result_t result;
  do {
    result = burrow_process(burrow);
  } while (result != BURROW_OK);

  /* Now get the first message */
  {
    printf("Now we will get the first message\n");
    burrow_filters_st *filters = burrow_filters_create(0,0);

    burrow_filters_set_detail(filters, BURROW_DETAIL_ALL);
    burrow_get_message(burrow, client.account, client.queue, 
		       client.messages->msg_id,
		       filters
		       );
    burrow_process(burrow);
  }

  /*
  burrow_destroy(burrow);
  if (use_http == 0) {
    burrow = burrow_create(NULL, "dummy");
    printf("burrow = %p\n", burrow);
  } else {
    burrow = burrow_create(NULL, "http");
    printf("burrow = %p\n", burrow);
    burrow_backend_set_option(burrow, "server", "localhost");
    burrow_backend_set_option(burrow, "port", "8080");
  }
  */

  /* Now update the first message to hide for a couple seconds */
  uint32_t seconds = 3;
  printf("Now we will update the first message to hide it for %d seconds\n",seconds);
  burrow_filters_st *filters = burrow_filters_create(0,0);
  burrow_attributes_st *attributes = burrow_attributes_create(0,0);
  burrow_attributes_set_hide(attributes, seconds);
  burrow_filters_set_detail(filters, BURROW_DETAIL_ALL);
  burrow_update_message(burrow, client.account, client.queue, 
			client.messages->msg_id,
			attributes,
			filters
			);
  burrow_process(burrow);

  printf("Get all messages, one should be missing\n");
  burrow_get_messages(burrow, client.account, client.queue, NULL);
  burrow_process(burrow);
  printf("Now sleep until the messages should reappear\n");
  sleep(seconds + 1);

  printf("Now get all messages again\n");
  burrow_get_messages(burrow, client.account, client.queue, NULL);
  burrow_process(burrow);
  
  printf("Ok, now let us delete one message\n");
  burrow_filters_destroy(filters);
  filters = burrow_filters_create(0,0);
  burrow_filters_set_detail(filters, BURROW_DETAIL_ALL);
  burrow_delete_message(burrow, client.account, client.queue, client.messages->msg_id,
			NULL);
  burrow_process(burrow);

  printf("Get all messages, one should be missing\n");
  burrow_get_messages(burrow, client.account, client.queue, NULL);
  burrow_process(burrow);

  printf("Get list of accounts\n");
  burrow_set_account_fn(burrow, &_account);
  burrow_get_accounts(burrow, NULL);
  burrow_process(burrow);

  printf("finishing up\n");

  burrow_destroy(burrow);
  exit(0);

  return client.return_code;
}
