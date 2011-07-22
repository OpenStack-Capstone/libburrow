#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <uuid/uuid.h>
#include <math.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <libburrow/burrow.h>


typedef struct incoming_st
{
  char *id;
  char *data;
  size_t size;
  
  struct incoming_st *next;
} incoming_st;

typedef struct app_state_st
{
  int error;
  int generator;
  char *last_msg_id;

  incoming_st *msg;
} app_state_st;

#define FATAL(msg) \
do { \
  fprintf(stderr, "FATAL: %s\n", msg); \
  exit(1); \
} while(0);


#define FATALF(msg, ...) \
  do { \
    fprintf(stderr, "FATAL: " ); \
    fprintf(stderr, msg, __VA_ARGS__); \
    fprintf(stderr, "\n"); \
    exit(1); \
  } while(0);

static void random_equation(char *buf, int size);
static double process_equation(const char *buf);

static void print_help(const char *invocation)
{
  printf("\nUsage: %s [options]\n"
         "\t-h                - See this message\n"
         "\t-s <server>       - Specify the server; default localhost\n"
         "\t-p <port>         - Specify the port; default 8080\n"
         "\t-a <account>      - Specify the account; default: current login name\n"
         "\t-q <queue>        - Specify the queue; default: 'demu'\n"
         "\t-e <seed>         - Specify random seed for message generator\n"
         "\t-m <count>        - Specify number of messages to act upon, 0 unlimited\n"
         "\t-l <count>        - Specify maximum sleep between actions\n"
         "\t-g                - Act as a generator, exclusive with:\n"
         "\t-c                - Act as a consumer (default)\n"
         "\t-v                - Be verbose\n",
         invocation);
}


static void message_callback(burrow_st *burrow, const char *message_id, const void *body, size_t body_size, const burrow_attributes_st *attributes)
{
  app_state_st *state = burrow_get_context(burrow);
  (void) attributes;
  
  if (state->generator || !body)
    return;
  
  printf("msg received: %s\n", message_id);

  incoming_st *incoming = malloc(sizeof(incoming_st));
  char *data = malloc(body_size + 1);
  char *id = strdup(message_id);
  
  if (!incoming || !data || !id)
    FATAL("ran out of memory");

  memcpy(data, body, body_size);
  data[body_size] = '\0';
  
  incoming->data = data;
  incoming->size = body_size;
  incoming->id = id;
  incoming->next = state->msg;
  
  state->msg = incoming;
}

static void complete_callback(burrow_st *burrow)
{
  app_state_st *state = burrow_get_context(burrow);
  incoming_st *incoming;

  if (!state->msg)
    return;

  free(state->last_msg_id);
  state->last_msg_id = strdup(state->msg->id);
  
  while((incoming = state->msg)) {
    double result = process_equation((char*)incoming->data);
    printf("result: %f\n", result);
    
    free(incoming->data);
    free(incoming->id);

    state->msg = incoming->next;
    free(incoming);
  }
}

static void error_callback(burrow_st *burrow, burrow_verbose_t err, const char *msg)
{
  app_state_st *state = burrow_get_context(burrow);
  if (err >= BURROW_VERBOSE_ERROR)
    state->error = 1;
  printf("%s: burrow: %s\n", burrow_verbose_name(err), msg);
}

int main(int argc, char **argv)
{
  const char *server = "localhost";
  const char *port = "8080";
  const char *account = getlogin();
  const char *queue = "demo";
  int c;
  int verbose = 0;
  int seed = (int)time(NULL);
  int messages = INT_MAX;
  int maxsleep = 5;
    
  app_state_st state = { 0, 0, 0, 0 };
  
  while ((c = getopt(argc, argv, "s:p:a:q:gchve:m:l:")) != -1)
  {
    switch(c) {
    case 'v':
      verbose++;
      break;
    case 'g':
      state.generator = 1;
      break;
    case 'c':
      state.generator = 0;
      break;
    case 's':
      server = optarg;
      break;
    case 'p':
      port = optarg;
      break;
    case 'a':
      account = optarg;
      break;
    case 'q':
      queue = optarg;
      break;
    case 'e':
      seed = atoi(optarg);
      break;
    case 'm':
      messages = atoi(optarg);
      break;
    case 'l':
      maxsleep = atoi(optarg);
      break;
    default:
    case 'h':
      print_help(argv[0]);
      return 0;
    }
  }
  
  srand((unsigned int)seed);
  if (verbose > 1)
    printf("info: using random seed %d", seed);

  burrow_st *burrow;
  
  burrow = burrow_create(NULL, "http");
  if (!burrow)
    FATAL("burrow creation failed");
    
  burrow_backend_set_option(burrow, "server", server);
  burrow_backend_set_option(burrow, "port", port);
  
  burrow_add_options(burrow, BURROW_OPT_AUTOPROCESS);
  
  burrow_set_log_fn(burrow, &error_callback);
  burrow_set_message_fn(burrow, &message_callback);
  burrow_set_complete_fn(burrow, &complete_callback);
  
  burrow_set_context(burrow, &state);
  
  
  burrow_attributes_st *attr = burrow_attributes_create(NULL, burrow);
  if (!attr)
    FATAL("couldn't allocate attributes\n");

  if (state.generator) {
    char buf[1024];
    char uuidbuf[36 + 1];
    uuid_t uuid;
    
    while (messages--) {
      random_equation(buf, 1024);
      
      uuid_generate(uuid);
      uuid_unparse(uuid, uuidbuf);
      printf("Sending: %s\nExpected value: %f\n", buf, process_equation(buf));
      burrow_create_message(burrow, account, queue, uuidbuf, (void*)buf, strlen(buf), NULL);
      
      if (state.error)
        FATAL("encountered error");
    
      if (messages && maxsleep) {
        int sl = rand() % maxsleep;
        if (verbose)
          printf("info: sleeping for %d seconds\n", sl);
        usleep((__useconds_t)sl * 10000);
      }
    }
    if (verbose)
      printf("info: done sending messages\n");
  } else { /* Consumer */
    burrow_filters_st *filters = burrow_filters_create(NULL, burrow);
    if (!filters)
      FATAL("couldn't create filters or attributes");
    burrow_filters_set_detail(filters, BURROW_DETAIL_ALL);
    while(messages--) {
      if (state.last_msg_id)
        burrow_filters_set_marker(filters, state.last_msg_id);
      burrow_get_messages(burrow, account, queue, filters);
      if (messages && maxsleep) {
        int sl = rand() % maxsleep;
        if (verbose)
          printf("info: sleeping for %d seconds\n", sl);
        usleep((__useconds_t)sl * 10000);
      }
    }
    free(state.last_msg_id);
  }
  
  burrow_destroy(burrow);
  return 0;
}


static void random_equation(char *buf, int size)
{
  const int CUTOFF = size - 1 - 1 - 2; /* null, op, maxint strlen */
  const int MAXINT = 99;
  const int STOP_CHANCE = 11; // 1/STOP_CHANCE chance of stopping each iteration
  char *p = buf;

  if (CUTOFF <= 0) {
    *p = 0;
    return;
  }
  
  p += sprintf(buf, "%d", rand() % MAXINT + 1);
  while(p - buf < CUTOFF)
  {
    if (rand() % STOP_CHANCE == 0)
      break;
    char op = (char)(rand() % 4);
    
    switch(op)
    {
      default:
      case 0:
        op = '+';
        break;
      case 1:
        op = '-';
        break;
      case 2:
        op = '*';
        break;
      case 3:
        op = '/';
        break;
    }
    p += sprintf(p, "%c%d", op, rand() % MAXINT + 1);
  }
  *p = '\0';
}

static double process_equation(const char *buf)
{
  double value;
  int intval, count;
  const char *p = buf;
  
  if (sscanf(p, "%d%n", &intval, &count) <= 0) {
    return NAN;
  }
  p += count;
  value = intval;
  
  while (*p) {
    char op = *p;
    ++p;
    
    if (sscanf(p, "%d%n", &intval, &count) <= 0) {
      return NAN;      
    }
    
    p += count;

    switch(op) {
      case '+':
        value += intval;
        break;
      case '*':
        value *= intval;
        break;
      case '/':
        value /= intval;
        break;
      case '-':
        value -= intval;
        break;
      default:
        return NAN;
    }
  }
  return value;
}
