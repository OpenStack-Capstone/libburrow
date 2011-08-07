/**
 * Simple rsyslogd dispatcher.
 */
#include <stdio.h>
#include <stdlib.h>
#include <uuid/uuid.h>
#include <string.h>
#include <libburrow/burrow.h>

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
    return 1;
  }
  
  burrow_st *burrow = burrow_create(NULL, "http");
  if (!burrow) {
    return 2;
  }
  
  burrow_set_backend_option(burrow, "server", server);
  burrow_set_backend_option(burrow, "port", port);
  burrow_set_verbosity(BURROW_VERBOSE_NONE);


  uuid_generate(uuid);
  uuid_unparse(uuid, uuidbuf);

  burrow_create_message(burrow, account, queue, uuidbuf, (void*)*argv, strlen(*argv), NULL);
  burrow_process(burrow);

  burrow_destroy(burrow);
  return 0;
}

