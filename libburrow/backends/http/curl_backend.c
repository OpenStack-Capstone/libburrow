/**
 * libburrow: curl_backend
 *
 * copyright (C) 2011 Adrian Miranda (ade@psg.com)
 * all rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in the main directory for full text.
 */
/**
 * @file
 * 
 * @brief this implements a backed for libburow.  It utilizes libcurl to provide
 * an http connection to a burrow backend server.
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

//#include <libburrow/backends/http/parser/contrib/JSON_PARSER/JSON_parser.h>

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
  bool malloced;
  bool get_body_only;
};
//typedef struct burrow_backend_st burrow_backend_t;

static burrow_result_t burrow_backend_http_process(void *ptr);

#include "curl_backend.h"

#include "json_processing.h"

burrow_command_t
burrow_backend_http_get_command(burrow_backend_t *backend) {
  return backend->burrow->cmd.command;
}

burrow_st *
burrow_backend_http_get_burrow(burrow_backend_t *backend) {
  return backend->burrow;
}

CURL *
burrow_backend_http_get_curl_easy_handle(burrow_backend_t* backend) {
  return backend->chandle;
}

/**
 * given attributes, should return a string suitable for placement on the
 * end of a URL
 */
static char *
burrow_backend_http_attributes_to_string(const burrow_attributes_st *attributes){
  char buf[1024] = "";
  if (attributes == 0)
    return 0;
  if (burrow_attributes_isset_ttl(attributes))
    sprintf(buf, "ttl=%ld", burrow_attributes_get_ttl(attributes));
  if (burrow_attributes_isset_hide(attributes)) {
    if (strlen(buf) > 0)
      sprintf(buf + strlen(buf),"%s", "&");
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

  if (burrow_filters_isset_match_hidden(filters)) {
    if (burrow_filters_get_match_hidden(filters) == true)
      sprintf(buf, "match_hidden=true");
    else 
      sprintf(buf, "match_hidden=false");
  }
  if (burrow_filters_isset_limit(filters)) {
    if (strlen(buf) > 0) sprintf(buf + strlen(buf), "&");
    sprintf(buf + strlen(buf), "limit=%d", burrow_filters_get_limit(filters));
  }
  if (burrow_filters_get_marker(filters) != NULL) {
    if (strlen(buf) > 0) sprintf(buf + strlen(buf), "&");
    sprintf(buf + strlen(buf), "marker=%s", burrow_filters_get_marker(filters));
  }
  if (burrow_filters_isset_wait(filters)) {
    if (strlen(buf) > 0) sprintf(buf + strlen(buf), "&");
    sprintf(buf + strlen(buf), "wait=%d", burrow_filters_get_wait(filters));
  }
  if (burrow_filters_isset_detail(filters)) {
    if (strlen(buf) > 0) sprintf(buf + strlen(buf), "&");
    if (burrow_filters_get_detail(filters) == BURROW_DETAIL_NONE)
      sprintf(buf + strlen(buf), "detail=none");
    else if (burrow_filters_get_detail(filters) == BURROW_DETAIL_ID)
      sprintf(buf + strlen(buf), "detail=id");
    else if (burrow_filters_get_detail(filters) == BURROW_DETAIL_ATTRIBUTES)
      sprintf(buf + strlen(buf), "detail=attributes");
    else if (burrow_filters_get_detail(filters) == BURROW_DETAIL_BODY)
      sprintf(buf + strlen(buf), "detail=body");
    else if (burrow_filters_get_detail(filters) == BURROW_DETAIL_ALL)
      sprintf(buf + strlen(buf), "detail=all");
  }
  
  if (strlen(buf) == 0)
    return 0;
  else
    return strdup(buf);
}


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
    backend->malloced = true;
  } else {
    backend->malloced = false;
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
  burrow_log_debug(backend->burrow, "burrow_backend_http_destroy callled\n");
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
    backend->chandle = 0;
  }
  curl_multi_cleanup(backend->curlptr);
  backend->curlptr = 0;
  
  if (backend->malloced)
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
  } else {
    burrow_error(backend->burrow,
		 BURROW_ERROR_UNSUPPORTED,
		 "ERROR: Called set_option with illegal option: %s\n",
		 optionname);
    return BURROW_ERROR_UNSUPPORTED;
  }

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


static burrow_result_t
burrow_backend_http_create_message(void *ptr,
				   const burrow_command_st *cmd)
{
  char *account;
  char *queue;
  char *message_id;
  const uint8_t *body = cmd->body;
  size_t body_size = cmd->body_size;
  const burrow_attributes_st *attributes = cmd->attributes;
  burrow_backend_t * backend = (burrow_backend_t *)ptr;
  
  CURL *chandle = curl_easy_init();
  account = curl_easy_escape(chandle, cmd->account,0);
  queue = curl_easy_escape(chandle,cmd->queue,0);
  message_id = curl_easy_escape(chandle,cmd->message_id,0);

  char *attr_string = burrow_backend_http_attributes_to_string(attributes);
  char *url = malloc(strlen(backend->baseurl) + strlen(account) + 
		     strlen(queue) + strlen(message_id) +
		     (attr_string ? strlen(attr_string) : 0) +
		     20);
  sprintf(url, "%s/%s/%s/%s/%s",
	  backend->baseurl, backend->proto_version,
	  account, queue, message_id);
  curl_free(account); account = 0;
  curl_free(queue); queue = 0;
  curl_free(message_id); message_id = 0;
  if (attr_string != 0) {
    sprintf(url + strlen(url), "?%s", attr_string);
    free(attr_string);
  }
  burrow_log_debug(backend->burrow, "create_message url = \"%s\"\n", url);
  curl_easy_setopt(chandle, CURLOPT_URL, url);

  user_buffer *buffer = user_buffer_create_sized(0, body, body_size);
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

  burrow_log_debug(backend->burrow, "burrow_backend_http_process starting\n");
  do {
    retval = curl_multi_perform(backend->curlptr, &running_handles);
  } while (retval == CURLM_CALL_MULTI_PERFORM);

  burrow_log_debug(backend->burrow, "burrow_backend_http_process finished looking at stuff, running handles = %d\n", running_handles);

  if (retval != CURLM_OK) {
    // it appears some kind of error occured...
    burrow_error(backend->burrow, BURROW_ERROR_SERVER,
		 "Call to libcurl failed(%d): %s\n",
		 retval,
		 curl_multi_strerror(retval));
    return BURROW_ERROR_SERVER;
  }
  // At this point, the curl_multi interface didn't have a problem,
  // However, there could still have been errors on transfer...
  CURLMsg *curlmsg;
  int msgs_in_queue;
  curlmsg = curl_multi_info_read(backend->curlptr, &msgs_in_queue);
  while (curlmsg != NULL) {
    if (curlmsg->data.result != CURLE_OK) {
      burrow_error(backend->burrow, BURROW_ERROR_SERVER,
		   "Error transferring (%d): %s\n",
		   curlmsg->data.result,
		   curl_easy_strerror(curlmsg->data.result));
      return BURROW_ERROR_SERVER;
    } else {
      burrow_log_debug(backend->burrow,
		       "Transfer completed successfully\n");
    }
    curlmsg = curl_multi_info_read(backend->curlptr, &msgs_in_queue);
  }

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
	burrow_error(backend->burrow,
		     BURROW_ERROR_BAD_ARGS,
		     "ERROR! libcurl wants to monitor exceptions on file_descriptor=%d, not presently supported",
		     i);
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
	  if (user_buffer_get_size(backend->buffer) > 0) {
	    int json_return =
	    burrow_backend_http_parse_json(backend,
					   user_buffer_get_text(backend->buffer),
					   user_buffer_get_size(backend->buffer));
	    if (json_return < 0) {
	      // apparently an error occured.
	      burrow_error(backend->burrow,
			   BURROW_ERROR_SERVER,
			   "Error occured while trying to parse JSON message: \"%s\"\n",
			   user_buffer_get_text(backend->buffer)
			   );
			   
	    }
	  }
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
  /* in future it would be good to pay attention, because without it
     libcurl needs to check the descriptors before it knows which has
     activity
  */
  (void)ptr;
  (void)fd;
  (void)event;
  return BURROW_OK;
}

/**
 * gets lists of strings, specifically get_queues and get_accounts
 *
 * @param ptr the burrow_back_t pointer
 * @param cmd the burrow_command_st structure, which tells us what we must do.
 * @return burrow_result_t which tells us if it is complete or still working.
 */
static burrow_result_t
burrow_backend_http_common_getlists(void *ptr, const burrow_command_st *cmd)
{
  burrow_backend_t *backend = (burrow_backend_t *)ptr;
  char *account = 0;
  const burrow_filters_st *filters = cmd->filters;
  burrow_st *burrow = backend->burrow;
  burrow_command_t command = burrow->cmd.command;
  size_t urllen = 0;
  char *filter_str = 0;
  backend->get_body_only = false;

  CURL *chandle;
  chandle = curl_easy_init();
  if (command == BURROW_CMD_GET_QUEUES) {
    account = curl_easy_escape(chandle, cmd->account, 0);
  }
  filter_str = burrow_backend_http_filters_to_string(filters);
  
  urllen = strlen(backend->baseurl) +
    strlen(backend->proto_version) +
    (account == 0 ? 0 : strlen(account)) +
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
    curl_free(account);
  }

  curl_easy_setopt(chandle, CURLOPT_URL, url);
  free(url); url = 0;
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
  return burrow_backend_http_common_getlists(ptr, cmd);
}

static burrow_result_t
burrow_backend_http_get_queues(void *ptr, const burrow_command_st *cmd)
{
  return burrow_backend_http_common_getlists(ptr, cmd);
}

/**
 * common code for deleting queues or accounts
 */
static burrow_result_t
burrow_backend_http_common_delete(void *ptr, const burrow_command_st *cmd)
{
  burrow_backend_t *backend = (burrow_backend_t *)ptr;
  char *account = 0;

  const burrow_filters_st *filters = cmd->filters;
  burrow_st *burrow = backend->burrow;
  burrow_command_t command = burrow->cmd.command;
  size_t urllen = 0;
  char *filter_str = 0;

  backend->get_body_only = false;
  CURL *chandle;
  chandle = curl_easy_init();
  if (command == BURROW_CMD_DELETE_QUEUES)
    account = curl_easy_escape(chandle, cmd->account, 0);

  filter_str = burrow_backend_http_filters_to_string(filters);
  urllen = strlen(backend->baseurl) +
    strlen(backend->proto_version) +
    (account==0 ? 0 : strlen(account)) +
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
    curl_free(account); account = 0;
  }
  if (filter_str != 0) {
    len_so_far = strlen(url);
    snprintf(url+len_so_far, urllen-len_so_far, "?%s", filter_str);
  }

  curl_easy_setopt(chandle, CURLOPT_URL, url);
  free(url); url = 0;
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
  char *account;
  char *queue;
  char *message_id;
  const burrow_attributes_st *attributes = cmd->attributes;
  const burrow_filters_st *filters = cmd->filters;
  burrow_st *burrow = backend->burrow;
  burrow_command_t command = burrow->cmd.command;
  size_t urllen = 0;
  char *filter_str = 0;
  char *attribute_str = 0;

  if ((filters) &&
      (burrow_filters_isset_detail(filters)) &&
      (burrow_filters_get_detail(filters) == BURROW_DETAIL_BODY))
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
  account = curl_easy_escape(chandle, cmd->account,0);
  queue = curl_easy_escape(chandle, cmd->queue, 0);

  if ((command == BURROW_CMD_UPDATE_MESSAGE) || (command == BURROW_CMD_DELETE_MESSAGE)||
      (command == BURROW_CMD_GET_MESSAGE))
    message_id = curl_easy_escape(chandle, cmd->message_id, 0);
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
  curl_free(account); account = 0;
  curl_free(queue); queue = 0;
  if ((command == BURROW_CMD_UPDATE_MESSAGE) || (command == BURROW_CMD_DELETE_MESSAGE)||
      (command == BURROW_CMD_GET_MESSAGE)) {
    len_so_far = strlen(url);
    snprintf(url + len_so_far, urllen-len_so_far, "/%s",
	     message_id);
    curl_free(message_id); message_id = 0;
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
      snprintf(url+len_so_far, urllen-len_so_far, "?%s", attribute_str);
    free(attribute_str); attribute_str = 0;
  }
  burrow_log_debug(backend->burrow,
		   "URL to send is \"%s\"\n",
		   url);

  curl_easy_setopt(chandle, CURLOPT_URL, url);
  /* We weren't freeing this -- is this where this free should go? */
  free(url);
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
