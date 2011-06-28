/*
  EXAMPLE FILE
  
  Crazy outdated -- do not refer to.
*/

#include <stdio.h>
#include <event2/event.h>
#include <burrow/burrow.h>

/* typedefs for libevent structures (not typedef'd by default) */
typedef struct event_base event_base_st;
typedef struct event event_st;

/* Simple linked list structure */
typedef struct {
  linked_list_st *next;
  void *data;
} linked_list_st; 

/* Client states */
typedef enum {
  S_START,
  S_ACCTS,
  S_QUEUES,
  S_MSGS,
  S_DONE,
  S_ERROR
} client_state_t;

/* Client structure */
typedef struct {
  burrow_st *burrow;

  linked_list *accts;
  linked_list *queues;
  linked_list *msgs;
  
  burrow_account_st *cur_account;
  burrow_queue_st *cur_queue;
  client_state_t state;
  
  event_base_st event_base;
} client_st;


/* Client functions */
client_st *client_create(burrow_st *burrow, event_base *ev_base);
void client_free(client_st *client);

/* Callback that will be raised by libevent */
void event_raised_callback(evutil_socket_t socket, short event, void *context);

/* Callback that will be raised by libburrow */
void wait_for_event_callback(int fd, short wait_events, void *context)

int main(void)
{
  const int CLIENT_COUNT = 4;
  event_base_st *ev_base;
  burrow_st *burrow;
  client_st *clients[CLIENT_COUNT];
  int i;
  
  burrow = burrow_create("http");
  burrow_set_options(burrow, BURROW_OPT_NONBLOCK);
  /* eday seemed to support this type of interface for backend parameters
     for modularity, stating that most  */
  burrow_set_backend_option_string("server", "burrow.example.com");
  burrow_set_backend_option_int("port", 1234);
  /* We won't implement connection pooling. This is just an example: */
  burrow_set_backend_option_int("connection_pool", 4);
  
  burrow_set_event_fn(burrow, &wait_for_event_callback);
  
  burrow_set_message_fn(burrow, &message_callback);
  burrow_set_queue_fn(burrow, &queue_callback);
  burrow_set_account_fn(burrow, &account_callback);
  burrow_set_error_fn(burrow, &error_callback);

  ev_base = event_base_new(); /* Set up libevent base */
  
  for (i = 0; i < CLIENT_COUNT; i++) {
    client = client_create(burrow, ev_base);
    /* Kick off client processing */
    client_process(client);
  }
  
  event_base_dispatch(ev_base); /* This encapsulates the whole select/wait loop */

  for (i = 0; i < CLIENT_COUNT; i++)
    free(client);

  event_base_free(ev_base);
  burrow_free(burrow);
}

client_st *client_create(burrow_st *burrow, event_base *ev_base)
{
  client_st *client;
  
  client = malloc(sizeof(client_st));
  if (!client)
    return NULL;

  client->burrow = burrow;
  client->event_base = ev_base;
  client->accts = list_new();
  client->queues = list_new();
  client->msgs = list_new();
  client->cur_account = NULL;
  client->cur_queue = NULL;

  client->state = S_START;
}

void client_free(client_st *client)
{
  if (client) {
    list_free(client->accts);
    list_free(client->queues);
    list_free(client->msgs);
    free(client);
  }
}

void client_process(client_st *client)
{
  burrow_message_st *msg;
  burrow_account_st *acct;
  burrow_queue_st *queue;
  
  /* We do this to keep the libevent loop busy, in case the transition from
     one state to the next doesn't block */
  while(burrow_status(client->burrow) != BURROW_STATUS_WAITING) {
    switch(client->state) {
      case S_START:
        client->state = S_ACCTS;
        burrow_get_accounts(client->burrow, NULL);
        break;
      case S_ACCTS:
        acct = (burrow_account_st *)list_pop(client->accts);
        if (acct == NULL)
          client->state = S_DONE;
        else {
          client->current_account = acct;
          client->state = S_QUEUES;
          burrow_get_queues(client->burrow,
                            burrow_account_name(client->cur_account),
                            NULL);
        }
        break;
      case S_QUEUES:
        queue = (burrow_queue_st *)list_pop(client->queues);
        if (queue == NULL)
          client->state = S_ACCTS;
        else {
          client->current_queue = queue;
          client->state = S_MSGS;
          burrow_get_messages(client->burrow,
                              burrow_account_name(client->cur_account),
                              burrow_queue_name(client->cur_queue),
                              NULL);
        }
        break;
      case S_MSGS:
        while ((msg = (burrow_message_st *)list_pop(client->msgs)) != NULL)
          print_burrow_msg(msg);
        client->state = S_QUEUES;
        break;
      case S_ERROR:
        /* Clean up the lists */
        while ((msg = (burrow_message_st *)list_pop(client->msgs)) != NULL)
          burrow_message_free(msg);

        while ((queue = (burrow_queue_st *)list_pop(client->queues)) != NULL)
          burrow_queue_free(queue);

        while ((acct = (burrow_queue_st *)list_pop(client->accts)) != NULL)
          burrow_account_free(acct);

        fprintf(stderr, "!!! ERROR encountered, terminating early.\n");
        break;
      case S_DONE:
        printf("*** Done printing all messages in all queues in all accounts.\n");
        break;
    }
  }
  return;
}

void print_burrow_msg(burrow_message_st *msg)
{
  printf("%s->%s->%s:\n", burrow_message_get_id(msg));
  fwrite("")
}


void event_raised_callback(evutil_socket_t socket, short event, void *context)
{
  client_st *client = (client_st *)context;
  
  /* The following will trigger processing within burrow: */
  burrow_event_raised(client->burrow, (int)socket, event);

  /* Which we will then attempt to take advantage of;
     if burrow is still waiting, this call will simply do no work */
  client_process(client);
}

/* libburrow callbacks: */
void wait_for_event_callback(int fd, short wait_events, void *context)
{
  client_st *client = (client_st *)context;
  event_st *event;
  short what;
  
  if (wait_events & BURROW_WAIT_READ)
    what = EV_READ;
  else if (wait_events & BURROW_WAIT_WRITE)
    what = EV_WRITE;
  else
    return; /* error! */
  
  event = event_new(client->event_base, fd, what, &event_raised_callback, (void*)client);
  event_add(event, NULL); /* Event is now pending */
}

void message_callback(burrow_message_st *message, void *context)
{
  client_st *client = (client_st *)context;
  burrow_message_st *copy = burrow_message_new();
  burrow_message_clone(copy, message)
  list_push(client->msgs, (void*)copy);
}

void queue_callback(burrow_queue_st *queue, void *context)
{
  client_st *client = (client_st *)context;
  burrow_queue_st *copy = burrow_queue_new();
  burrow_queue_clone(copy, queue)
  list_push(client->queues, (void*)copy);
}

void account_callback(burrow_account_st *acct, void *context)
{
  client_st *client = (client_st *)context;
  burrow_account_st *account = burrow_account_new();
  burrow_account_clone(copy, acct)
  list_push(client->accts, (void*)copy);
}

/* Not entirely sure how this should look... */
void error_callback(burrow_error_t err, void *context)
{
  client_st *client = (client_st *)context;
  fprintf(stderr, "!!! ERROR: libburrow(%d): %s\n", burrow_error_code(err), burrow_error_msg(err));
  client->state = S_ERROR;
}

/* Open questions:

  * Should the burrow structure be passed into the callbacks above?
  
  * What should the error callback accept?:
    + an error message string?
    + just the burrow structure?
      + should those burrow_error_codes be applied to burrow_st?

  * Should the new/allocation functions, such as burrow_account_new() accept
    a burrow_st so that they can be associated with and subsequently auto-
    deallocated along with the burrow_st?
    + And should they accept NULL, in which case no association is made?
*/













