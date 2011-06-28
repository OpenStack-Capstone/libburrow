/* Blocking case -- pseudocode provided by Eric Day */
#include <burrow.h>
#include <stdio.h>

typedef struct client_st
{
  int return_code;
  ...
} client_st;

void _complete(void *context)
{
  client_st *client = (client_st *)context;
  client.current_message = client.current_message->next;
  if (client.current_message == NULL)
    return;

  /* Since this only sets next start state, we don't have to worry about recursion. When
     this callback returns the loop continues with this next message. */
  burrow_create_message(burrow, client->account, client->queue,
    client->current_message->message, NULL, client->current_message->body);
}

void _log(burrow_log_level_t level, char *message, int errno, void *context)
{
  client_st *client = (client_st *)context;
  if (level == BURROW_LOG_LEVEL_ERROR)
  {
    printf("Error: %s (%d)", message, errno);
    client->return_code = 1;
  }
}

int main(void)
{
  burrow_st *burrow;
  client_st client;
  client.message_count = 0;
  client.return_code = 0;
  client.account = "account";
  client.queue = "queue";
  client.messages = (linked list of messages)
  client.current_message = client.messages;

  burrow = burrow_create("http");
  burrow_set_backend_option("server", "burrow.example.com");
  burrow_set_backend_option("port", "1234");

  burrow_set_context(burrow, &client);
  burrow_set_complete_fn(burrow, &_complete);
  burrow_set_log_fn(burrow, &_log);

  /* Insert the first one here to kick the loop off. This only sets start state,
     it doesn't run the loop. */
  message = client.current_messages;
  burrow_create_message(burrow, client.account, client.queue,
    client.current_message->message, NULL, client.current_message->body);

  /* This runs until there are no more tasks. */
  burrow_process(burrow);

  return client->return_code;
}