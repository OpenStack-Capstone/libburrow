/*
 * curl_backend
 */

#include "config.h"
#include <curl/curl.h>
#include <libburrow/common.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "user_buffer.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include <libburrow/backends/http/parser/contrib/JSON_PARSER/JSON_parser.h>

struct burrow_backend_st {
  char *proto;
  char *server;
  char *port;
  char *baseurl;
  char *proto_version;
  burrow_st *burrow;
  struct user_buffer_st *buffer;

  CURL *chandle;
  CURLM *curlptr;
  int malloced;
  bool get_body_only;
};
typedef struct burrow_backend_st burrow_backend_t;

static burrow_result_t burrow_backend_http_process(void *ptr);

//#include "curl_backend.h"

/**
 * given attributes, should return a string suitable for placement on the
 * end of a URL
 */
static char *
burrow_backend_http_attributes_to_string(const burrow_attributes_st *attributes){
  char buf[1024] = "";
  if (attributes == 0)
    return 0;
  if (burrow_attributes_check(attributes, BURROW_ATTRIBUTES_TTL))
    sprintf(buf, "ttl=%ld:", burrow_attributes_get_ttl(attributes));
  if (burrow_attributes_check(attributes, BURROW_ATTRIBUTES_HIDE)) {
    if (strlen(buf) > 0)
      sprintf(buf + strlen(buf), "&");
    sprintf(buf + strlen(buf), "hide=%ld",
	    burrow_attributes_get_hide(attributes));
  }
  if (strlen(buf) == 0)
    return 0;
  else 
    return strdup(buf);
}

/* Given filters, return a malloced space containing something
   suitable for adding to the end of a url
*/
static char *
burrow_backend_http_filters_to_string(const burrow_filters_st *filters) {
  char buf[1024] = "";
  if (filters== 0)
    return 0;

  if (filters->set & BURROW_FILTERS_MATCH_HIDDEN){
    if (filters->match_hidden == true)
      sprintf(buf, "match_hidden=true");
    else if (filters->match_hidden == false)
      sprintf(buf, "match_hidden=false");
  }
  if (filters->set & BURROW_FILTERS_LIMIT) {
    if (strlen(buf) > 0) sprintf(buf + strlen(buf), "&");
    sprintf(buf + strlen(buf), "limit=%d", filters->limit);
  }
  if (filters->set & BURROW_FILTERS_MARKER) {
    if (strlen(buf) > 0) sprintf(buf + strlen(buf), "&");
    sprintf(buf + strlen(buf), "marker=%s", filters->marker);
  }
  if (filters->set & BURROW_FILTERS_WAIT) {
    if (strlen(buf) > 0) sprintf(buf + strlen(buf), "&");
    sprintf(buf + strlen(buf), "wait=%d", filters->wait);
  }
  if (filters->set & BURROW_FILTERS_DETAIL) {
    if (strlen(buf) > 0) sprintf(buf + strlen(buf), "&");
    if (filters->detail == BURROW_DETAIL_NONE)
      sprintf(buf + strlen(buf), "detail=none");
    else if (filters->detail == BURROW_DETAIL_ID)
      sprintf(buf + strlen(buf), "detail=id");
    else if (filters->detail == BURROW_DETAIL_ATTRIBUTES)
      sprintf(buf + strlen(buf), "detail=attributes");
    else if (filters->detail == BURROW_DETAIL_BODY)
      sprintf(buf + strlen(buf), "detail=body");
    else if (filters->detail == BURROW_DETAIL_ALL)
      sprintf(buf + strlen(buf), "detail=all");
  }
  if (strlen(buf) == 0)
    return 0;
  else
    return strdup(buf);
}


//static void *
//burrow_backend_http_create(void *ptr, burrow_st *burrow);

static size_t
burrow_backend_http_size(void){
  return sizeof(burrow_backend_t);
}

static void *
burrow_backend_http_create(void *ptr, burrow_st *burrow)
{
  burrow_backend_t *backend = (burrow_backend_t *)ptr;

  if (backend == 0) {
    backend = (burrow_backend_t *)malloc(sizeof(burrow_backend_t));
    backend->malloced = 1;
  } else {
    backend->malloced = 0;
  }
  if (backend == 0)
    return 0;

  backend->burrow = burrow;
  backend->proto = strdup("http");
  backend->server = 0;
  backend->port = 0;
  backend->baseurl = 0;
  backend->proto_version = strdup("v1.0");
  backend->buffer = 0;
  backend->chandle = 0;
  backend->get_body_only = false;

  backend->curlptr = curl_multi_init();
  return (void *)backend;
}

static void
burrow_backend_http_destroy(void * ptr) {
  burrow_backend_t *backend = (burrow_backend_t *)ptr;
  if (backend->proto != 0)
    free(backend->proto);
  if (backend->server !=0)
    free(backend->server);
  if (backend->port)
    free(backend->port);
  if (backend->baseurl)
    free(backend->baseurl);
  if (backend->proto_version)
    free(backend->proto_version);
  if (backend->buffer)
    user_buffer_destroy(backend->buffer);
  if (backend->chandle) {
    curl_multi_remove_handle(backend->curlptr, backend->chandle);
    curl_easy_cleanup(backend->chandle);
  }
  curl_multi_cleanup(backend->curlptr);
  
  if (backend->malloced == 1)
    free(backend);
}

static void *
burrow_backend_http_clone(void *dst, void*src)
{
  (void)dst;
  (void)src;
  return NULL;
}

static burrow_result_t
burrow_backend_http_set_option(void *ptr,
			       const char *optionname, const char *value)
{
  burrow_backend_t *backend=(burrow_backend_t *)ptr;

  int url_affecting = 0;
  if (strcmp(optionname, "server") == 0) {
    backend->server = strdup(value);
    url_affecting = 1;
  } else if (strcmp(optionname, "port") == 0) {
    backend->port = strdup(value);
    url_affecting = 1;
  } else
    return BURROW_ERROR_UNSUPPORTED;
    


  if ((url_affecting) && (backend->server != 0) && (backend->port != 0) &&
      (backend->proto != 0))
    {
      if (backend->baseurl != 0)
	free(backend->baseurl);

      backend->baseurl = malloc(strlen(backend->proto) + strlen(backend->server) +
				       strlen(backend->port) + 20);
      sprintf(backend->baseurl, "%s://%s:%s",
	      backend->proto,
	      backend->server,
	      backend->port
	      );
    }
  return BURROW_OK;
}

struct json_processing_st {
  burrow_backend_t* backend;
  char *body;
  size_t body_size;
  char *message_id;
  int is_key;
  char *key;
  burrow_attributes_st *attributes;

};

typedef struct json_processing_st json_processing_t;

static json_processing_t* burrow_easy_json_st_create(burrow_backend_t* backend)
{
  json_processing_t *jproc = malloc(sizeof(json_processing_t));
  jproc->backend = backend;
  jproc->body = 0;
  jproc->body_size = 0;
  jproc->message_id = 0;
  jproc->is_key = 0;
  jproc->key = 0;
  jproc->attributes = burrow_attributes_create(0, 0);
  return jproc;
}

static void
burrow_easy_json_st_destroy(json_processing_t *jproc) {
  if (jproc->body)
    free(jproc->body);
  if (jproc->message_id)
    free(jproc->message_id);
  if (jproc->key)
    free(jproc->key);
  if (jproc->attributes)
    free(jproc->attributes);
  free(jproc);
}

/*
static int message_fn(burrow_handle burrow, char *message_id, char *body,
		      size_t body_size, char **attrptr)
{
  printf("Got message \"%s\" with body \"%s\"\n", message_id, body);
}
*/

static int burrow_backend_http_json_callback(void *ctx,
					     int type,
					     const JSON_value* value)
{
  json_processing_t* jproc = (json_processing_t *)ctx;

  /* This is for commands that return lists of messages, or even
     just one message.
  */
  burrow_command_t bcommand = jproc->backend->burrow->cmd.command;
  if (
      (bcommand == BURROW_CMD_GET_MESSAGE) ||
      (bcommand == BURROW_CMD_GET_MESSAGES) ||
      (bcommand == BURROW_CMD_UPDATE_MESSAGE) ||
      (bcommand == BURROW_CMD_UPDATE_MESSAGES) ||
      (bcommand == BURROW_CMD_DELETE_MESSAGE) ||
      (bcommand == BURROW_CMD_DELETE_MESSAGES))
    {
      switch(type) {
      case JSON_T_ARRAY_BEGIN:
      case JSON_T_ARRAY_END:
	break;
      case JSON_T_OBJECT_BEGIN:
	// make sure we have attributes, and they are unset
	jproc->attributes = burrow_attributes_create(jproc->attributes, 0);
	break;
      case JSON_T_OBJECT_END:
	printf("Ok, got the end of an object\n");
	
	/*
	jproc->backend->burrow->message_fn(jproc->backend->burrow,
					   jproc->message_id,
					   (uint8_t *)jproc->body,
					   (ssize_t)jproc->body_size,
					   jproc->attributes);
	*/
	burrow_callback_message(jproc->backend->burrow,
				jproc->message_id,
				(uint8_t *)jproc->body,
				jproc->body_size,
				jproc->attributes);
	free(jproc->message_id);
	jproc->message_id = 0;
	free(jproc->body);
	jproc->body = 0;
	break;
      case JSON_T_KEY:
	jproc->is_key = 1;
	if (jproc->key)
	  free(jproc->key);
	jproc->key = strdup(value->vu.str.value);
	break;
      case JSON_T_STRING:
	if(jproc->is_key) {
	  jproc->is_key = 0;
	  if (strcmp(jproc->key, "id") == 0) {
	    jproc->message_id = strdup(value->vu.str.value);
	  } else if (strcmp(jproc->key, "body") == 0) {
	    jproc->body = strdup(value->vu.str.value);
	    jproc->body_size = strlen(value->vu.str.value);
	  } else
	    fprintf(stderr, "WARNING!  unrecognized key \"%s\"\n", jproc->key);
	}
	break;
      case JSON_T_INTEGER:
	if(jproc->is_key) {
	  jproc->is_key = 0;
	  if (strcmp(jproc->key, "hide") == 0) {
	    burrow_attributes_set_hide(jproc->attributes,
				       (uint32_t)value->vu.integer_value);
	  } else if (strcmp(jproc->key, "ttl") == 0) {
	    burrow_attributes_set_ttl(jproc->attributes,
				      (uint32_t)value->vu.integer_value);
	  } else
	    fprintf(stderr, "WARNING! unrecognized integer key \"%s\"=%d\n",
		    jproc->key, value->vu.integer_value);
	}
	break;
      default:
	fprintf(stderr, "WARNING: while parsing get response, unexpected type = %d\n",
		type);
	break;
      }
    } else if ((bcommand == BURROW_CMD_GET_QUEUES) ||
	       (bcommand == BURROW_CMD_GET_ACCOUNTS))
    {
      /* This is for commands that return lists of strings, such as when
	 you list the queues or the accounts
      */
      switch(type) {
      case JSON_T_ARRAY_BEGIN:
	break;
      case JSON_T_ARRAY_END:
	/* Does not seem to be anything to do here. */
	break;
      case JSON_T_STRING:
	if (bcommand == BURROW_CMD_GET_ACCOUNTS) {
	  burrow_callback_account(jproc->backend->burrow,
				  value->vu.str.value);
	}
	break;
      default:
	assert("We should never get here" == 0);
	break;
      }
    }

  return 1;
}

static int burrow_backend_http_parse_json(burrow_backend_t *backend,
					  char *jsontext,
					  size_t jsonsize)
{
  JSON_config config;
  struct JSON_parser_struct* jc = NULL;
  init_JSON_config(&config);
  
  json_processing_t *json_processing = burrow_easy_json_st_create(backend);

  config.depth                  = 19;
  config.callback               = &burrow_backend_http_json_callback;
  config.callback_ctx		= json_processing;
  config.allow_comments         = 1;
  config.handle_floats_manually = 0;
  jc = new_JSON_parser(&config);

  jsontext[jsonsize]='\0';
  fprintf(stderr, "Text: \"%s\"\n\n", jsontext);

  int i;
  for (i = 0; i < (int)jsonsize; ++i) {
    int retval;
    int nextchar = jsontext[i];
    if ((retval = JSON_parser_char(jc, nextchar)) <= 0) {
      fprintf(stderr, "JSON_parser_char returned error (%d) at byte %d (%d = '%c')\n",
	      retval, i, nextchar, nextchar);
      //return -1;
    }
  }    
  fprintf(stderr, "Ok, parsed through the entire JSON message\n");
  if (!JSON_parser_done(jc)) {
    fprintf(stderr, "JSON_parser_end: syntax error\n");
    burrow_easy_json_st_destroy(json_processing);
    return -1;
  }
  burrow_easy_json_st_destroy(json_processing);
  return 0;
}


static burrow_result_t
burrow_backend_http_create_message(void *ptr,
				   const burrow_command_st *cmd)
{
  const char *account = cmd->account;
  const char *queue = cmd->queue;
  const char *message_id = cmd->message_id;
  const uint8_t *body = cmd->body;
  size_t body_size = cmd->body_size;
  const burrow_attributes_st *attributes = cmd->attributes;
  burrow_backend_t * backend = (burrow_backend_t *)ptr;
  
  CURL *chandle = curl_easy_init();
  char *attr_string = burrow_backend_http_attributes_to_string(attributes);
  char *url = malloc(strlen(backend->baseurl) + strlen(account) + 
		     strlen(queue) + strlen(message_id) +
		     (attr_string ? strlen(attr_string) : 0) +
		     20);
  sprintf(url, "%s/%s/%s/%s/%s",
	  backend->baseurl, backend->proto_version,
	  account, queue, message_id);
  if (attr_string != 0) {
    sprintf(url + strlen(url), "?%s", attr_string);
    free(attr_string);
  }
printf("create_message url = \"%s\"\n", url);
  curl_easy_setopt(chandle, CURLOPT_URL, url);

  user_buffer *buffer = user_buffer_create(0, body);
  curl_easy_setopt(chandle, CURLOPT_READFUNCTION, user_buffer_curl_read_function);
  curl_easy_setopt(chandle, CURLOPT_READDATA, buffer);

  curl_easy_setopt(chandle, CURLOPT_UPLOAD, 1);
  curl_easy_setopt(chandle, CURLOPT_INFILESIZE, body_size);

  curl_easy_setopt(chandle, CURLOPT_VERBOSE, 1);
  curl_easy_setopt(chandle, CURLOPT_HEADER, 0);

  if (backend->chandle) {
    curl_multi_remove_handle(backend->curlptr, backend->chandle);
    curl_easy_cleanup(backend->chandle);
  }
  backend->chandle = chandle;
  curl_multi_add_handle(backend->curlptr, chandle);

  if(backend->buffer) {
    user_buffer_destroy(backend->buffer);
  }
  backend->buffer = buffer;
  url[0] = '\0';
  free(url);

  return burrow_backend_http_process((void*)backend);
}
  
static burrow_result_t
burrow_backend_http_process(void *ptr) {
  burrow_backend_t *backend = (burrow_backend_t *)ptr;
  CURLMcode retval;
  int running_handles;

  printf("burrow_backend_http_process starting\n");
  do {
    retval = curl_multi_perform(backend->curlptr, &running_handles);
  } while (retval == CURLM_CALL_MULTI_PERFORM);

  printf("burrow_backend_http_process finished looking at stuff, running handles = %d\n", running_handles);
  
  /* If curl still monitoring fds, we need to tell burrow "frontend"
     which ones to watch for.
  */
  if (running_handles > 0) {
    fd_set read_fd_set, write_fd_set, exec_fd_set;
    int max_fd = -1;
    FD_ZERO(&read_fd_set); FD_ZERO(&write_fd_set); FD_ZERO(&exec_fd_set);
    retval =
      curl_multi_fdset(backend->curlptr, &read_fd_set,
		       &write_fd_set, &exec_fd_set, &max_fd);
    if (max_fd == -1)
      return BURROW_OK;
    for (int i = 0; i <= max_fd; ++i) {
      burrow_ioevent_t burrow_event = BURROW_IOEVENT_NONE;
      if (FD_ISSET(i, &read_fd_set))
	burrow_event |= BURROW_IOEVENT_READ;
      if (FD_ISSET(i, &write_fd_set))
	burrow_event |= BURROW_IOEVENT_WRITE;
      if (FD_ISSET(i, &exec_fd_set))
	fprintf(stderr, "WARNING!  libcurl wants to monitor exceptions on %d, but we cannot.\n", i);
      if (burrow_event != BURROW_IOEVENT_NONE) {
	burrow_watch_fd(backend->burrow, i, burrow_event);
      }
    }
    return BURROW_OK_WAITING;
  } else {
    /* this should mean I have retrieved everything I need... I hope.
       therefore, process it.  The problem is, different commands can
       give back different results.  More importantly, the same command
       can produce different results depending on attributes.

       For example, the get command will normall get JSON structures containg
       one or more message.  But if the attributes say only get body, it
       will only contain the body, not JSON.

       As another example, the DELETE command will normally get nothing,
       but if you use the detail attribute, it will get complete messages.
       On the other hand, should be able to parse that as json, or ignore
       it because empty...
    */

    burrow_command_t command = backend->burrow->cmd.command;

    if ((command == BURROW_CMD_GET_MESSAGES) ||
	(command == BURROW_CMD_GET_MESSAGE) ||
	(command == BURROW_CMD_DELETE_MESSAGES) ||
	(command == BURROW_CMD_DELETE_MESSAGE) ||
	(command == BURROW_CMD_UPDATE_MESSAGES) ||
	(command == BURROW_CMD_UPDATE_MESSAGE) ||
	(command == BURROW_CMD_GET_ACCOUNTS) ||
	(command == BURROW_CMD_GET_QUEUES)
	)
      {
	if (backend->get_body_only)
	  burrow_callback_message(backend->burrow,
				  0,
				  user_buffer_get_text(backend->buffer),
				  user_buffer_get_size(backend->buffer),
				  0);
	else
	  if (user_buffer_get_size(backend->buffer) > 0)
	    burrow_backend_http_parse_json(backend,
					   user_buffer_get_text(backend->buffer),
					   user_buffer_get_size(backend->buffer));
	
      }
    
    return BURROW_OK;
  }
}

static burrow_result_t
burrow_backend_http_event_raised(void *ptr,
				 int fd,
				 burrow_ioevent_t event)
{
  /* stub.  we are ignoring this for now.*/
  /* in future it might be useful to pay attention, since right now,
     libcurl needs to check the descriptors before it knows which has
     activity
  */
  (void)ptr;
  (void)fd;
  (void)event;
  return BURROW_OK;
}

/**
 * gets lists of strings, specificaly get_queues and get_accounts
 */
static burrow_result_t
burrow_backend_http_get_lists(void *ptr, const burrow_command_st *cmd)
{
  burrow_backend_t *backend = (burrow_backend_t *)ptr;
  const char *account = cmd->account;
  const burrow_filters_st *filters = cmd->filters;
  burrow_st *burrow = backend->burrow;
  burrow_command_t command = burrow->cmd.command;
  size_t urllen = 0;
  char *filter_str = 0;
  backend->get_body_only = false;

  CURL *chandle;
  chandle = curl_easy_init();
  filter_str = burrow_backend_http_filters_to_string(filters);
  
  urllen = strlen(backend->baseurl) +
    strlen(backend->proto_version) +
    strlen(account) +
    (filter_str == 0? 0 : strlen(filter_str)) + 
    + 128;
  char *url = malloc(urllen);
  size_t len_so_far = 0;

  snprintf(url, urllen, "%s/%s",
	   backend->baseurl,
	   backend->proto_version
	   );

  if (command == BURROW_CMD_GET_QUEUES) {
    len_so_far = strlen(url);
    snprintf(url+len_so_far, urllen-len_so_far, "/%s", account);
  }

  curl_easy_setopt(chandle, CURLOPT_URL, url);
  curl_easy_setopt(chandle, CURLOPT_UPLOAD, 0L);
  curl_easy_setopt(chandle, CURLOPT_HTTPGET, 1L);

  user_buffer *buffer = user_buffer_create(0,0);
  curl_easy_setopt(chandle, CURLOPT_WRITEFUNCTION,
		   user_buffer_curl_write_function);
  curl_easy_setopt(chandle, CURLOPT_WRITEDATA, buffer);

  curl_easy_setopt(chandle, CURLOPT_VERBOSE, 1);
  curl_easy_setopt(chandle, CURLOPT_HEADER, 0);

  // Toss old curl handle, if present and different
  if (backend->chandle) {
    if (backend->chandle != chandle) {
      curl_multi_remove_handle(backend->curlptr, backend->chandle);
      curl_easy_cleanup(backend->chandle);
      backend->chandle=chandle;
      curl_multi_add_handle(backend->curlptr, chandle);
    }
  } else {
    backend->chandle = chandle;
    curl_multi_add_handle(backend->curlptr, chandle);
  }

  // Toss old buffer, if present
  if (backend->buffer != 0)
    user_buffer_destroy(backend->buffer);
  backend->buffer = buffer;

  return burrow_backend_http_process(backend);
}

static burrow_result_t
burrow_backend_http_get_accounts(void* ptr,
				 const burrow_command_st *cmd)
{
  return burrow_backend_http_get_lists(ptr, cmd);
}

static burrow_result_t
burrow_backend_http_get_queues(void *ptr, const burrow_command_st *cmd)
{
  return burrow_backend_http_get_lists(ptr, cmd);
}

/**
 * for deleting queues or accounts
 */
static burrow_result_t
burrow_backend_http_common_delete(void *ptr, const burrow_command_st *cmd)
{
  burrow_backend_t *backend = (burrow_backend_t *)ptr;
  const char *account = cmd->account;
  const char *queue = cmd->queue;
  const burrow_filters_st *filters = cmd->filters;
  burrow_st *burrow = backend->burrow;
  burrow_command_t command = burrow->cmd.command;
  size_t urllen = 0;
  char *filter_str = 0;

  backend->get_body_only = false;
  CURL *chandle;
  chandle = curl_easy_init();

  filter_str = burrow_backend_http_filters_to_string(filters);
  urllen = strlen(backend->baseurl) +
    strlen(backend->proto_version) +
    (account ? 0 : strlen(account)) +
    strlen(queue) +
    (filter_str == 0? 0 : strlen(filter_str)) +
    128;

  char *url = malloc(urllen);
  size_t len_so_far = 0;

  snprintf(url, urllen, "%s/%s",
	   backend->baseurl,
	   backend->proto_version
	   );

  if (command == BURROW_CMD_DELETE_QUEUES) {
    len_so_far = strlen(url);
    snprintf(url+len_so_far, urllen-len_so_far, "/%s", account);
  }
  if (filter_str != 0) {
    len_so_far = strlen(url);
    snprintf(url+len_so_far, urllen-len_so_far, "?%s", filter_str);
  }

  curl_easy_setopt(chandle, CURLOPT_URL, url);
  curl_easy_setopt(chandle, CURLOPT_UPLOAD, 0L);
  curl_easy_setopt(chandle, CURLOPT_CUSTOMREQUEST, "DELETE");

  user_buffer *buffer = user_buffer_create(0,0);
  curl_easy_setopt(chandle, CURLOPT_WRITEFUNCTION,
		   user_buffer_curl_write_function);
  curl_easy_setopt(chandle, CURLOPT_WRITEDATA, buffer);

  curl_easy_setopt(chandle, CURLOPT_VERBOSE, 1);
  curl_easy_setopt(chandle, CURLOPT_HEADER, 0);
  // Toss old curl handle, if present and different
  if (backend->chandle) {
    if (backend->chandle != chandle) {
      curl_multi_remove_handle(backend->curlptr, backend->chandle);
      curl_easy_cleanup(backend->chandle);
      backend->chandle=chandle;
      curl_multi_add_handle(backend->curlptr, chandle);
    }
  } else {
    backend->chandle = chandle;
    curl_multi_add_handle(backend->curlptr, chandle);
  }

  // Toss old buffer, if present
  if (backend->buffer != 0)
    user_buffer_destroy(backend->buffer);
  backend->buffer = buffer;

  return burrow_backend_http_process(backend);
}

static burrow_result_t
burrow_backend_http_delete_accounts(void *ptr, const burrow_command_st *cmd)
{
  return burrow_backend_http_common_delete(ptr, cmd);
}

static burrow_result_t
burrow_backend_http_delete_queues(void *ptr, const burrow_command_st *cmd)
{
  return burrow_backend_http_common_delete(ptr, cmd);
}

/**
 * Performs anything that can get a message(s).  That includes get_message,
 * update_message and delete_message
 */
static burrow_result_t
burrow_backend_http_common_getting(void *ptr,
				   const burrow_command_st *cmd)
{  
  burrow_backend_t *backend = (burrow_backend_t *)ptr;
  const char *account = cmd->account;
  const char *queue = cmd->queue;
  const char *message_id = cmd->message_id;
  const burrow_attributes_st *attributes = cmd->attributes;
  const burrow_filters_st *filters = cmd->filters;
  burrow_st *burrow = backend->burrow;
  burrow_command_t command = burrow->cmd.command;
  size_t urllen = 0;
  char *filter_str = 0;
  char *attribute_str = 0;

  if ((filters) &&
      (filters->set | BURROW_FILTERS_DETAIL) &&
      (filters->detail == BURROW_DETAIL_BODY))
    backend->get_body_only = true;
  else
    backend->get_body_only = false;

  CURL *chandle;

  //if (backend->chandle) {
  //chandle = backend->chandle;
  //curl_easy_reset(chandle);
  //} else {
    chandle = curl_easy_init();
    //}

  if ((command == BURROW_CMD_UPDATE_MESSAGE) || (command == BURROW_CMD_DELETE_MESSAGE)||
      (command == BURROW_CMD_GET_MESSAGE))
    message_id = cmd->message_id;
  else
    message_id = 0;

  filter_str = burrow_backend_http_filters_to_string(filters);

  // If this is an update, attributes are also sent.
  if ((command == BURROW_CMD_UPDATE_MESSAGES) || (command == BURROW_CMD_UPDATE_MESSAGE))
    {
      attribute_str = burrow_backend_http_attributes_to_string(attributes);
    }
    
  urllen = strlen(backend->baseurl) +
    strlen(backend->proto_version) +
    strlen(account) +
    strlen(queue) +
    (message_id == 0? 0 : strlen(message_id)) +
    (filter_str == 0? 0 : strlen(filter_str)) + 
    (attribute_str == 0 ? 0 : strlen(attribute_str)) +
    + 128;
  char *url = malloc(urllen);
  size_t len_so_far = 0;

  snprintf(url, urllen, "%s/%s/%s/%s",
	   backend->baseurl,
	   backend->proto_version,
	   account,
	   queue);
  if ((command == BURROW_CMD_UPDATE_MESSAGE) || (command == BURROW_CMD_DELETE_MESSAGE)||
      (command == BURROW_CMD_GET_MESSAGE)) {
    len_so_far = strlen(url);
    snprintf(url + len_so_far, urllen-len_so_far, "/%s",
	     message_id);
  }
  if (filter_str != 0) {
    len_so_far = strlen(url);
    snprintf(url + len_so_far, urllen - len_so_far, "?%s", filter_str);
    free(filter_str);
  }
  if (attribute_str != 0) {
    len_so_far = strlen(url);
    if (filter_str != 0)
      snprintf(url+len_so_far, urllen-len_so_far, "&%s", attribute_str);
    else
      snprintf(url+len_so_far, urllen-len_so_far, "?%s", filter_str);
  }
  fprintf(stderr, "URL to send is \"%s\"\n", url);

  curl_easy_setopt(chandle, CURLOPT_URL, url);
  if ((command == BURROW_CMD_GET_MESSAGES) || (command == BURROW_CMD_GET_MESSAGE)){
    curl_easy_setopt(chandle, CURLOPT_UPLOAD, 0L);
    curl_easy_setopt(chandle, CURLOPT_HTTPGET, 1L);
  } else if ((command == BURROW_CMD_UPDATE_MESSAGES) ||
	     (command == BURROW_CMD_UPDATE_MESSAGE)){
    //curl_easy_setopt(chandle, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(chandle, CURLOPT_POST, 1L);
    curl_easy_setopt(chandle, CURLOPT_POSTFIELDSIZE, 0L);
  } else if ((command == BURROW_CMD_DELETE_MESSAGES) ||
	     (command==BURROW_CMD_DELETE_MESSAGE)){
    curl_easy_setopt(chandle, CURLOPT_UPLOAD, 0L);
    curl_easy_setopt(chandle, CURLOPT_CUSTOMREQUEST, "DELETE");
  }

  user_buffer *buffer = user_buffer_create(0,0);
  curl_easy_setopt(chandle, CURLOPT_WRITEFUNCTION,
		   user_buffer_curl_write_function);
  curl_easy_setopt(chandle, CURLOPT_WRITEDATA, buffer);

  if ((command == BURROW_CMD_UPDATE_MESSAGE) ||
      (command == BURROW_CMD_UPDATE_MESSAGES)) {
    curl_easy_setopt(chandle, CURLOPT_READFUNCTION,
		     user_buffer_curl_read_nothing_function);
    curl_easy_setopt(chandle, CURLOPT_READDATA, 4000);
  }
  curl_easy_setopt(chandle, CURLOPT_VERBOSE, 1);
  curl_easy_setopt(chandle, CURLOPT_HEADER, 0);

  // Toss old curl handle, if present and different
  if (backend->chandle) {
    if (backend->chandle != chandle) {
      curl_multi_remove_handle(backend->curlptr, backend->chandle);
      curl_easy_cleanup(backend->chandle);
      backend->chandle=chandle;
      curl_multi_add_handle(backend->curlptr, chandle);
    }
  } else {
    backend->chandle = chandle;
    curl_multi_add_handle(backend->curlptr, chandle);
  }

  // Toss old buffer, if present
  if (backend->buffer != 0)
    user_buffer_destroy(backend->buffer);
  backend->buffer = buffer;

  return burrow_backend_http_process(backend);
}

static burrow_result_t
burrow_backend_http_delete_message(void *ptr, const burrow_command_st *cmd)
{
  return burrow_backend_http_common_getting(ptr, cmd);
}

static burrow_result_t
burrow_backend_http_delete_messages(void *ptr, const burrow_command_st *cmd)
{
  return burrow_backend_http_common_getting(ptr, cmd);
}

static burrow_result_t
burrow_backend_http_get_message(void *ptr, const burrow_command_st *cmd)
{
  return burrow_backend_http_common_getting(ptr, cmd);
}

static burrow_result_t
burrow_backend_http_get_messages(void *ptr, const burrow_command_st *cmd)
{
  return burrow_backend_http_common_getting(ptr, cmd);
}

static burrow_result_t
burrow_backend_http_update_message(void *ptr, const burrow_command_st *cmd)
{
  return burrow_backend_http_common_getting(ptr, cmd);
}

static burrow_result_t
burrow_backend_http_update_messages(void *ptr, const burrow_command_st *cmd)
{
  return burrow_backend_http_common_getting(ptr, cmd);

}


/* This is the structure that the frontend will use to access our
   internal functions. This is filled out in C99 style, which I think
   is acceptable. We can, however, default to C90 style if req'd. */
burrow_backend_functions_st burrow_backend_http_functions = {
  .create = &burrow_backend_http_create,
  .destroy = &burrow_backend_http_destroy,
  .size = &burrow_backend_http_size,
  .clone = &burrow_backend_http_clone,

  .set_option = &burrow_backend_http_set_option,

  .event_raised = &burrow_backend_http_event_raised,

  .get_accounts = &burrow_backend_http_get_accounts,
  .delete_accounts = &burrow_backend_http_delete_accounts,

  .get_queues = &burrow_backend_http_get_queues,
  .delete_queues = &burrow_backend_http_delete_queues,

  .get_messages = &burrow_backend_http_get_messages,
  .update_messages = &burrow_backend_http_update_messages,
  .delete_messages = &burrow_backend_http_delete_messages,

  .get_message = &burrow_backend_http_get_message,
  .update_message = &burrow_backend_http_update_message,
  .delete_message = &burrow_backend_http_delete_message,
  .create_message = &burrow_backend_http_create_message,

  .process = &burrow_backend_http_process,
};
