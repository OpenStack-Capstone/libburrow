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


static void complete_fn(burrow_st *burrow)
{
  (void) burrow;
  puts("burrow: command complete\n");
}

static void log_fn(burrow_st *burrow, burrow_verbose_t err, const char *msg)
{
  (void) burrow;
  if (err >= BURROW_VERBOSE_ERROR) {
    fprintf(stderr, "burrow[%s]: %s\n", burrow_verbose_name(err), msg);
    exit(3);
  }
}

int main(int argc, char **argv)
{
  const char *server = "localhost";
  const char *port = "8080";
  const char *account = "teamd";
  const char *queue = "syslog";
  char uuidbuf[36 + 1];
  uuid_t uuid;
  
  argc--;
  argv++;
  if (argc != 1) {
    fputs("exactly one argument -- message contents -- expected!\n", stderr);
    return 1;
  }
  
  burrow_st *burrow = burrow_create(NULL, "http");
  if (!burrow) {
    fputs("error creating burrow", stderr);
    return 2;
  }
  
  burrow_set_log_fn(burrow, &log_fn);
  burrow_set_complete_fn(burrow, &complete_fn);
  
  burrow_set_backend_option(burrow, "server", server);
  burrow_set_backend_option(burrow, "port", port);


  uuid_generate(uuid);
  uuid_unparse(uuid, uuidbuf);

  burrow_create_message(burrow, account, queue, uuidbuf, (void*)*argv, strlen(*argv), NULL);
  puts("sending message\n");
  burrow_process(burrow);

  burrow_destroy(burrow);
  return 0;
}

