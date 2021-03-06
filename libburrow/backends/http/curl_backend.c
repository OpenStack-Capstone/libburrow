/*
 * libburrow -- Burrow Client Library
 *
 * Copyright 2011 Adrian Miranda
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
  char proto[32];
  size_t proto_len;
  char proto_version[32];
  size_t proto_version_len;
  char port[32];
  size_t port_len;

  char *server;
  size_t server_len;
  char *baseurl;
  size_t baseurl_len;
  burrow_st *burrow;
  struct user_buffer_st *buffer;

  CURL *chandle;
  CURLM *curlptr;
  bool malloced;
  bool get_body_only;
};
//typedef struct burrow_backend_st burrow_backend_t;

static int burrow_backend_http_process(void *ptr);

#include "curl_backend.h"

#include "json_processing.h"

/**
 * helper function to find out which command we are executing right now.
 *
 * @param backend
 * @return command that we are executing
 */

burrow_command_t
burrow_backend_http_get_command(burrow_backend_t *backend) {
  return backend->burrow->cmd.command;
}

/**
 * helper function to obtain the burrow struct that corresponds to this backend
 *
 * @param backend
 * @return burrow the burrow_st pointer.
 */
burrow_st *
burrow_backend_http_get_burrow(burrow_backend_t *backend) {
  return backend->burrow;
}

/**
 * helper function to get the curl_easy_handle in use.
 *
 * @param backend
 * @return the CURL* we are using.
 */
CURL *
burrow_backend_http_get_curl_easy_handle(burrow_backend_t* backend) {
  if (backend->chandle == NULL)
    backend->chandle = curl_easy_init();
  return backend->chandle;
}

/**
 * given to libcurl for printing debug messages
 *
 * @param chancle The CURL handle in use
 * @param type A curl_infotype to indicate type of message
 * @param mesg Pointer to the message, which has a linefeed, but no
 * NULL termination
 * @param mesg_size size of the message
 * @return 0
 */
static int
burrow_backend_http_curldebug(CURL *chandle, curl_infotype type,
			      char *mesg, size_t mesg_size, void *ptr)
{
  (void)chandle;
  (void)type;
  const char *preface;
  burrow_backend_t *backend = (burrow_backend_t *)ptr;
  if (type == CURLINFO_TEXT)
    preface = "  ";
  else if (type == CURLINFO_HEADER_IN)
    preface = "";
  else if (type == CURLINFO_HEADER_OUT)
    preface = "";
  else if (type == CURLINFO_DATA_IN)
    preface = "< ";
  else if (type == CURLINFO_DATA_OUT)
    preface = "> ";
  burrow_log_debug(backend->burrow, "%s%.*s", preface, mesg_size, mesg);
  return 0;
}

/**
 * given attributes, should return a string suitable for placement on the
 * end of a URL
 *
 * @param attributes
 * @param size length of malloced string, or errno val if fails
 * and (return value will be null)
 * @return a malloced string that contains something we can tack onto a URL.
 * or NULL if there is nothing in attributes (and size will be zero), or
 * of NULL if there is a failure (and size will be errno value)
 */
static char *
burrow_backend_http_attributes_to_string(const burrow_attributes_st *attributes,
					 int *size)
{
  char buf[1024] = "";
  size_t len = 0;
  if (attributes == 0) {
    *size = 0;
    return 0;
  }
  if (burrow_attributes_isset_ttl(attributes))
    len = (size_t)snprintf(buf, sizeof(buf), "ttl=%ld",
			   burrow_attributes_get_ttl(attributes));
  if (burrow_attributes_isset_hide(attributes)) {
    if (len > 0)
      len += (size_t)snprintf(buf + len, sizeof(buf) - len, "%shide=%ld", "&",
			      burrow_attributes_get_hide(attributes));
    else
      len = (size_t)snprintf(buf, sizeof(buf), "hide=%ld",
		     burrow_attributes_get_hide(attributes));
  }
  if (len == 0) {
    return 0;
  } else {
    char *ptr = malloc(len+1);
    if (ptr == NULL) {
      *size = ENOMEM;
      return 0;
    }
    memcpy(ptr, buf, len+1);
    *size = (int)len;
    return ptr;
  }
}

/**
 * Given filters, return a malloced space containing something
 * suitable for adding to the end of a url
 * @param backend
 * @param filters The filters we want to examine (can be NULL)
 * @return a malloced string containing a string representation of the filters
 * that we can tack on the end of a URL
 */
static char *
burrow_backend_http_filters_to_string(burrow_backend_t *backend,
				      const burrow_filters_st *filters,
				      int *size)
{
  char buf[1024] = "";
  size_t len = 0;
  if (filters == 0) {
    *size = 0;
    return 0;
  }

  if (burrow_filters_isset_match_hidden(filters)) {
    if (burrow_filters_get_match_hidden(filters) == true)
      len = (size_t)snprintf(buf, sizeof(buf), "match_hidden=true");
    else 
      len = (size_t)snprintf(buf, sizeof(buf), "match_hidden=false");
  }
  if (burrow_filters_isset_limit(filters)) {
    if (len > 0) {
      len += (size_t)snprintf(buf + len, sizeof(buf) - len, "&limit=%d",
			      burrow_filters_get_limit(filters));
    } else {
      len = (size_t)snprintf(buf + len, sizeof(buf) - len, "limit=%d",
			     burrow_filters_get_limit(filters));
    }
  }
  if (burrow_filters_isset_wait(filters)) {
    if (len > 0) {
      len += (size_t)snprintf(buf + len, sizeof(buf) - len, "&wait=%d",
			      burrow_filters_get_wait(filters));
    } else {
      len = (size_t)snprintf(buf + len, sizeof(buf)-len, "wait=%d",
			     burrow_filters_get_wait(filters));
    }
  }
  if (burrow_filters_isset_detail(filters)) {
    if (len > 0) {
      len += (size_t)snprintf(buf + len, sizeof(buf)-len, "&");
    }
    if (burrow_filters_get_detail(filters) == BURROW_DETAIL_NONE)
      len += (size_t)snprintf(buf + len, sizeof(buf) - len, "detail=none");
    else if (burrow_filters_get_detail(filters) == BURROW_DETAIL_ID)
      len += (size_t)snprintf(buf + len, sizeof(buf) - len, "detail=id");
    else if (burrow_filters_get_detail(filters) == BURROW_DETAIL_ATTRIBUTES)
      len += (size_t)snprintf(buf + len, sizeof(buf) - len, "detail=attributes");
    else if (burrow_filters_get_detail(filters) == BURROW_DETAIL_BODY)
      len += (size_t)snprintf(buf + len, sizeof(buf) - len, "detail=body");
    else if (burrow_filters_get_detail(filters) == BURROW_DETAIL_ALL)
      len += (size_t)snprintf(buf + len, sizeof(buf) - len, "detail=all");
  }
  
  if (burrow_filters_get_marker(filters) != NULL) {
    char *marker =
      curl_easy_escape(backend->chandle, burrow_filters_get_marker(filters),0);
    if (len > 0) {
      len += (size_t)snprintf(buf + len, sizeof(buf) - len, "&marker=%s",
			      marker);
    } else {
      len = (size_t)snprintf(buf, sizeof(buf), "marker=%s", marker);
    }
    curl_free(marker);
  }

  if (len == 0){
    *size = 0;
    return 0;
  } else {
    char*ptr = (char*)malloc(len+1);
    if (ptr == NULL) {
      *size = ENOMEM;
      return ptr;
    }
    memcpy(ptr, buf, len+1);
    *size = (int)len;
    return ptr;
  }
}


/**
 * Return the size of a backend object, if the user should want to
 * allocate the space themself.
 * @return the size of a backend object
 */
static size_t
burrow_backend_http_size(void){
  return sizeof(burrow_backend_t);
}

/**
 * Create a new burrow backend object.
 *
 * @param ptr Optionally points to memory where we can create the
 * backend object.  If NULL, space will be malloced.
 * @param burrow pointer to an already existing burrow "frontend" object.
 * @return pointer to a new burrow backend object.
 */
static void *
burrow_backend_http_create(void *ptr, burrow_st *burrow)
{
  burrow_backend_t *backend = (burrow_backend_t *)ptr;

  if (backend == 0) {
    backend = (burrow_backend_t *)malloc(sizeof(burrow_backend_t));
    if (backend == NULL) {
      burrow_error(burrow,
		   ENOMEM,
		   "Failed to malloc space for a new backend object\n");
      return (void *)0;
    }
    backend->malloced = true;
  } else {
    backend->malloced = false;
  }
  if (backend == 0)
    return 0;

  backend->burrow = burrow;
  strcpy(backend->proto, "http");
  backend->proto_len = 4;
  backend->server = 0;
  backend->server_len = 0;
  backend->port[0] = 0;
  backend->port_len = 0;
  backend->baseurl = 0;
  backend->baseurl_len = 0;
  strcpy(backend->proto_version, "v1.0");
  backend->proto_version_len = 4;
  backend->buffer = 0;
  backend->chandle = 0;
  backend->get_body_only = false;

  backend->curlptr = curl_multi_init();
  return (void *)backend;
}

/**
 * Destroy a previously created http backend
 * @param ptr pointer to a previously created http backend
 */
static void
burrow_backend_http_destroy(void * ptr) {
  burrow_backend_t *backend = (burrow_backend_t *)ptr;
  burrow_log_debug(backend->burrow, "burrow_backend_http_destroy called\n");
  if (backend->server !=0)
    free(backend->server);
  if (backend->baseurl)
    free(backend->baseurl);
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

/**
 * Sets an option for this backend
 *
 * @param ptr Pointer to the backend object
 * @param optionname String name of the option
 * @param value String value of the option.  (yes, ports can be passed
 * as strings)
 * @return 0 if successful, EINVAL or other errno value otherwise.
 */
static int
burrow_backend_http_set_option(void *ptr,
			       const char *optionname, const char *value)
{
  burrow_backend_t *backend=(burrow_backend_t *)ptr;

  int url_affecting = 0;
  if (strcmp(optionname, "server") == 0) {
    backend->server_len = strlen(value);
    backend->server = malloc(backend->server_len + 1);
    if (backend->server == 0) {
      burrow_error(backend->burrow,
		   ENOMEM,
		   "Attempt to malloc space to store server name failed\n");
      return ENOMEM;
    }
    strcpy(backend->server, value);
    url_affecting = 1;
  } else if (strcmp(optionname, "port") == 0) {
    backend->port_len = strlen(value);
    if (backend->port_len >= (sizeof(backend->port) - 1)){
      burrow_log_error(backend->burrow, "ERROR: Attempt to set port = \"%s\" failed, because it has %d bytes, but the max is %d\n",
		       value, (int)backend->port_len, (int)sizeof(backend) - 1);
      backend->port_len = strlen(backend->port);
      return EINVAL;
    }
    strcpy(backend->port, value);
    url_affecting = 1;
  } else {
    burrow_log_error(backend->burrow,
		    "Called set_option with illegal option: %s\n", optionname);
    return EINVAL;
  }

  if ((url_affecting) && (backend->server != 0) && (backend->port_len != 0) &&
      (backend->proto_len != 0))
    {
      if (backend->baseurl != 0)
	free(backend->baseurl);

      size_t lenurl = backend->proto_len + strlen(backend->server) +
	backend->port_len + 20;
      backend->baseurl = malloc(lenurl);
      if (backend->baseurl == NULL) {
	burrow_error(backend->burrow,
		     ENOMEM,
		     "Attempt to malloc space for URL failed\n");
	return ENOMEM;
      }
      backend->baseurl_len = 
	(size_t)snprintf(backend->baseurl,
			 lenurl,
			 "%s://%s:%s",
			 backend->proto,
			 backend->server,
			 backend->port
			 );
    }
  return 0;
}


/**
 * Send a message to the burrow server
 *
 * @param ptr Pointer to the backend object
 * @param cmd pointer to a burrow_command_st
 * return 0 if successful, errno otherwise.
 */
static int
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
  /* Make sure that that which goes into the url is escaped appropriately. */
  account = curl_easy_escape(chandle, cmd->account,0);
  queue = curl_easy_escape(chandle,cmd->queue,0);
  message_id = curl_easy_escape(chandle,cmd->message_id,0);

  /* Now build up the url string, and hand it off to libcurl */
  int attr_str_len;
  char *attr_string = burrow_backend_http_attributes_to_string(attributes,
							       &attr_str_len);
  if ((attr_string == NULL) && (attr_str_len != 0)) {
    burrow_error(backend->burrow,
		 attr_str_len,
		 "Attempt to create attribute string from attributes failed");
    return attr_str_len;
  }
  size_t urllen = backend->baseurl_len +
    backend->proto_version_len +
    strlen(account) + 
    strlen(queue) + strlen(message_id) +
    (size_t)attr_str_len + 20;
  char *url = (char *)malloc(urllen);
  if (url == NULL) {
    burrow_error(backend->burrow,
		 ENOMEM,
		 "Failed to malloc space for URL\n");
    return ENOMEM;
  }
  size_t urllen_sofar =
    (size_t)snprintf(url, urllen, "%s/%s/%s/%s/%s",
		     backend->baseurl,
		     backend->proto_version,
		     account, queue, message_id);
  curl_free(account); account = 0;
  curl_free(queue); queue = 0;
  curl_free(message_id); message_id = 0;
  if (attr_string != 0) {

    urllen_sofar += (size_t)snprintf(url + urllen_sofar, urllen - urllen_sofar,
				     "?%s", attr_string);
    free(attr_string);
  }
  burrow_log_debug(backend->burrow, "create_message url = \"%s\"\n", url);
  curl_easy_setopt(chandle, CURLOPT_URL, url);

  /* Set up the data we want to send to burrowd. */
  user_buffer *buffer = user_buffer_create_sized(0, body, body_size);
  curl_easy_setopt(chandle, CURLOPT_READFUNCTION,
		   user_buffer_curl_read_function);
  curl_easy_setopt(chandle, CURLOPT_READDATA, buffer);

  curl_easy_setopt(chandle, CURLOPT_UPLOAD, 1);
  curl_easy_setopt(chandle, CURLOPT_INFILESIZE, body_size);

/*  curl_easy_setopt(chandle, CURLOPT_VERBOSE, 1); */
  curl_easy_setopt(chandle, CURLOPT_HEADER, 0);
  curl_easy_setopt(chandle, CURLOPT_DEBUGFUNCTION,
		   burrow_backend_http_curldebug);
  curl_easy_setopt(chandle, CURLOPT_DEBUGDATA, backend);

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
  
/**
 * Process what we have been told to do, or as much of it as we can do without
 * blocking.
 * @param ptr Pointer to backend object.
 * @return 0 if successful, errno if not.
 */
static int
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
    burrow_error(backend->burrow, EINVAL,
		 "Call to libcurl failed(%d): %s\n",
		 retval,
		 curl_multi_strerror(retval));
    return EINVAL;
  }
  // At this point, the curl_multi interface didn't have a problem,
  // However, there could still have been errors on transfer...
  CURLMsg *curlmsg;
  int msgs_in_queue;
  curlmsg = curl_multi_info_read(backend->curlptr, &msgs_in_queue);
  while (curlmsg != NULL) {
    if (curlmsg->data.result != CURLE_OK) {
      burrow_error(backend->burrow, EINVAL,
		   "Error transferring (%d): %s\n",
		   curlmsg->data.result,
		   curl_easy_strerror(curlmsg->data.result));
      return EINVAL;
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
      return 0;
    for (int i = 0; i <= max_fd; ++i) {
      burrow_ioevent_t burrow_event = BURROW_IOEVENT_NONE;
      if (FD_ISSET(i, &read_fd_set))
	burrow_event |= BURROW_IOEVENT_READ;
      if (FD_ISSET(i, &write_fd_set))
	burrow_event |= BURROW_IOEVENT_WRITE;
      if (FD_ISSET(i, &exec_fd_set))
	burrow_error(backend->burrow,
		     ENOTSUP,
		     "ERROR! libcurl wants to monitor exceptions on file_descriptor=%d, not presently supported",
		     i);
      if (burrow_event != BURROW_IOEVENT_NONE) {
	burrow_watch_fd(backend->burrow, i, burrow_event);
      }
    }
    return EAGAIN;
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
	    if (json_return > 0) {
	      // apparently an error occured.
	      burrow_error(backend->burrow,
			   json_return,
			   "Error occured while trying to parse JSON message: \"%s\"\n",
			   user_buffer_get_text(backend->buffer)
			   );
	      return json_return;
	    }
	  }
      }
    return 0;
  }
}

/**
 * Called to tell us that an event has occurred on a file descriptor we
 * previously requested be monitored.  We do not do anything when called,
 * as the caller will next need to call burrow_backend_http_process
 *
 * @param ptr pointer to a backend object
 * @param fd file descriptor where something happened.
 * @param event The event will occurred.
 * @return 0 if successful, errno if not
 */
static int
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
  return 0;
}

/**
 * gets lists of strings, specifically get_queues and get_accounts
 *
 * @param ptr the burrow_back_t pointer
 * @param cmd the burrow_command_st structure, which tells us what we must do.
 * @return int which tells us if it is complete or still working.
 */
static int
burrow_backend_http_common_getlists(void *ptr, const burrow_command_st *cmd)
{
  burrow_backend_t *backend = (burrow_backend_t *)ptr;
  char *account = 0;
  size_t account_len = 0;
  const burrow_filters_st *filters = cmd->filters;
  burrow_st *burrow = backend->burrow;
  burrow_command_t command = burrow->cmd.command;
  size_t urllen = 0;
  char *filter_str = 0;
  int filter_str_len;
  backend->get_body_only = false;

  CURL *chandle;
  chandle = curl_easy_init();
  if (command == BURROW_CMD_GET_QUEUES) {
    account = curl_easy_escape(chandle, cmd->account, 0);
    account_len = strlen(account);
  }
  filter_str = burrow_backend_http_filters_to_string(backend, filters,
						     &filter_str_len);
  if ((filter_str == NULL) && (filter_str_len != 0)) {
    burrow_error(burrow,
		 filter_str_len,
		 "Attempt to create filterURL from filters failed\n");
    return filter_str_len;
  }
  
  /* Build up the url, and pass to libcurl */
  urllen = backend->baseurl_len +
    backend->proto_version_len +
    account_len +
    (size_t)filter_str_len +
    128;
  char *url = malloc(urllen);
  if (url == NULL) {
    burrow_error(backend->burrow,
		 ENOMEM,
		 "Failed to malloc space for URL\n");
    return ENOMEM;
  }
  size_t len_so_far = 0;

  len_so_far = (size_t)snprintf(url, urllen, "%s/%s",
				backend->baseurl,
				backend->proto_version
				);

  if (command == BURROW_CMD_GET_QUEUES) {
    snprintf(url+len_so_far, urllen-len_so_far, "/%s", account);
    curl_free(account);
  }

  curl_easy_setopt(chandle, CURLOPT_URL, url);
  free(url); url = 0;

  curl_easy_setopt(chandle, CURLOPT_UPLOAD, 0L);
  curl_easy_setopt(chandle, CURLOPT_HTTPGET, 1L);

  /* set up the buffer where libcurl will save what it reads */
  user_buffer *buffer = user_buffer_create(0,0);
  curl_easy_setopt(chandle, CURLOPT_WRITEFUNCTION,
		   user_buffer_curl_write_function);
  curl_easy_setopt(chandle, CURLOPT_WRITEDATA, buffer);

  curl_easy_setopt(chandle, CURLOPT_DEBUGFUNCTION,
		   burrow_backend_http_curldebug);
  curl_easy_setopt(chandle, CURLOPT_DEBUGDATA, backend);
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

  // Toss old buffer, if present.  Then set new one.
  if (backend->buffer != 0)
    user_buffer_destroy(backend->buffer);
  backend->buffer = buffer;

  return burrow_backend_http_process(backend);
}

/**
 * Get a list of accounts.
 *
 * @param ptr pointer to a backend object
 * @param cmd pointer to a parsed command structure
 * @return 0 if successful, errno if not
 */
static int
burrow_backend_http_get_accounts(void* ptr,
				 const burrow_command_st *cmd)
{
  return burrow_backend_http_common_getlists(ptr, cmd);
}

/**
 * Get a list of queues under a given account
 *
 * @param ptr pointer to a backend object
 * @param cmd pointer to a parsed command structure
 * @return 0 if successful, errno if not
 */
static int
burrow_backend_http_get_queues(void *ptr, const burrow_command_st *cmd)
{
  return burrow_backend_http_common_getlists(ptr, cmd);
}

/**
 * common code for deleting queues or accounts
 * @param ptr pointer to a backend object
 * @param cmd pointer to a parsed command structure
 * @return 0 if successful, errno if not
 */
static int
burrow_backend_http_common_delete(void *ptr, const burrow_command_st *cmd)
{
  burrow_backend_t *backend = (burrow_backend_t *)ptr;
  char *account = 0;
  size_t account_len = 0;

  const burrow_filters_st *filters = cmd->filters;
  burrow_st *burrow = backend->burrow;
  burrow_command_t command = burrow->cmd.command;
  size_t urllen = 0;
  char *filter_str = 0;
  int filter_str_len;

  backend->get_body_only = false;
  CURL *chandle;
  chandle = curl_easy_init();
  if (command == BURROW_CMD_DELETE_QUEUES) {
    account = curl_easy_escape(chandle, cmd->account, 0);
    account_len = strlen(account);
  }

  filter_str = burrow_backend_http_filters_to_string(backend, filters,
						     &filter_str_len);
  if ((filter_str == NULL) && (filter_str_len != 0)) {
    burrow_error(burrow,
		 filter_str_len,
		 "Attempt to create URL type string from filters failed\n");
    return filter_str_len;
  }
  urllen = backend->baseurl_len +
    backend->proto_version_len +
    account_len +
    (size_t)filter_str_len +
    128;

  char *url = malloc(urllen);
  if (url == NULL) {
    burrow_error(backend->burrow,
		 ENOMEM,
		 "Failed to malloc space for URL\n");
    return ENOMEM;
  }
  size_t urllen_so_far;

  urllen_so_far = (size_t)snprintf(url, urllen, "%s/%s",
				backend->baseurl,
				backend->proto_version
				);

  if (command == BURROW_CMD_DELETE_QUEUES) {
    urllen_so_far +=
      (size_t)snprintf(url+urllen_so_far,
			      urllen-urllen_so_far,
			      "/%s",
			      account);
    curl_free(account); account = 0;
  }
  if (filter_str != 0) {
    urllen_so_far += 
      (size_t)snprintf(url+urllen_so_far, urllen-urllen_so_far, "?%s", filter_str);
    free(filter_str); filter_str_len = 0;
  }
  curl_easy_setopt(chandle, CURLOPT_URL, url);
  free(url); url = 0;

  /* Set random libcurl stuff */
  curl_easy_setopt(chandle, CURLOPT_UPLOAD, 0L);
  curl_easy_setopt(chandle, CURLOPT_CUSTOMREQUEST, "DELETE");

  /* Setup buffer for libcurl to use for response */
  user_buffer *buffer = user_buffer_create(0,0);
  curl_easy_setopt(chandle, CURLOPT_WRITEFUNCTION,
		   user_buffer_curl_write_function);
  curl_easy_setopt(chandle, CURLOPT_WRITEDATA, buffer);

  curl_easy_setopt(chandle, CURLOPT_DEBUGFUNCTION,
		   burrow_backend_http_curldebug);
  curl_easy_setopt(chandle, CURLOPT_DEBUGDATA, backend);
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

/**
 * delete accounts on a burrow server
 *
 * @param ptr pointer to a backend object
 * @param cmd pointer to a parsed command structure
 * @return 0 if successful, errno if not
 */
static int
burrow_backend_http_delete_accounts(void *ptr, const burrow_command_st *cmd)
{
  return burrow_backend_http_common_delete(ptr, cmd);
}

/**
 * Delete queues on a burrow server
 *
 * @param ptr pointer to a backend object
 * @param cmd pointer to a parsed command structure
 * @return 0 if successful, errno if not
 */
static int
burrow_backend_http_delete_queues(void *ptr, const burrow_command_st *cmd)
{
  return burrow_backend_http_common_delete(ptr, cmd);
}

/**
 * Common code for performing anything that can get a message(s).
 * That includes get_message, update_message and delete_message
 *
 * @param ptr pointer to a backend object
 * @param cmd pointer to a parsed command structure
 * @return 0 if successful, errno if not
 */
static int
burrow_backend_http_common_getting(void *ptr,
				   const burrow_command_st *cmd)
{  
  burrow_backend_t *backend = (burrow_backend_t *)ptr;
  char *account;
  size_t account_len = 0;
  char *queue;
  size_t queue_len = 0;
  char *message_id = 0;
  size_t message_id_len = 0;

  const burrow_attributes_st *attributes = cmd->attributes;
  const burrow_filters_st *filters = cmd->filters;
  burrow_st *burrow = backend->burrow;
  burrow_command_t command = burrow->cmd.command;
  size_t urllen = 0;
  char *filter_str = 0;
  int filter_str_len;
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
  account_len = strlen(account);
  queue = curl_easy_escape(chandle, cmd->queue, 0);
  queue_len = strlen(queue);

  if ((command == BURROW_CMD_UPDATE_MESSAGE) || (command == BURROW_CMD_DELETE_MESSAGE)||
      (command == BURROW_CMD_GET_MESSAGE))
    {
      message_id = curl_easy_escape(chandle, cmd->message_id, 0);
      message_id_len = strlen(message_id);
    }

  filter_str = burrow_backend_http_filters_to_string(backend, filters,
						     &filter_str_len);
  if ((filter_str == NULL) && (filter_str_len != 0)) {
    burrow_error(burrow,
		 filter_str_len,
		 "Attempt to create URL string from filters failed\n");
    return filter_str_len;
  }

  // If this is an update, attributes are also sent.
  int attribute_str_len = 0;
  if ((command == BURROW_CMD_UPDATE_MESSAGES) || (command == BURROW_CMD_UPDATE_MESSAGE))
    {
      attribute_str = burrow_backend_http_attributes_to_string(attributes,
							       &attribute_str_len
							       );
      if ((attribute_str == NULL) && (attribute_str_len != 0)) {
	burrow_error(burrow,
		     attribute_str_len,
		     "Call to create string from attributes failed\n");
	return attribute_str_len;
      }
    }
    
  urllen = backend->baseurl_len +
    backend->proto_version_len +
    account_len +
    queue_len +
    message_id_len +
    (size_t)filter_str_len +
    (size_t)attribute_str_len +
    + 128;
  char *url = malloc(urllen);
  if (url == NULL) {
    burrow_error(backend->burrow,
		 ENOMEM,
		 "Failed to malloc space for URL\n");
    return ENOMEM;
  }
  size_t urllen_so_far = 0;

  urllen_so_far +=
    (size_t)snprintf(url, urllen, "%s/%s/%s/%s",
		     backend->baseurl,
		     backend->proto_version,
		     account,
		     queue);
  curl_free(account); account = 0;
  curl_free(queue); queue = 0;
  if ((command == BURROW_CMD_UPDATE_MESSAGE) || (command == BURROW_CMD_DELETE_MESSAGE)||
      (command == BURROW_CMD_GET_MESSAGE))
    {
      urllen_so_far += 
	(size_t)snprintf(url + urllen_so_far, urllen-urllen_so_far, "/%s",
			 message_id);
      curl_free(message_id); message_id = 0;
    }
  
  if (filter_str_len != 0) {
    urllen_so_far += 
      (size_t)snprintf(url + urllen_so_far, urllen - urllen_so_far,
		       "?%s",
		       filter_str);
    // free filter_str later
  }
  if (attribute_str_len != 0) {
    if (filter_str != 0)
      urllen_so_far +=
	(size_t)snprintf(url+urllen_so_far, urllen-urllen_so_far, "&%s",
			 attribute_str);
    else
      urllen_so_far += 
	(size_t)snprintf(url+urllen_so_far, urllen-urllen_so_far, "?%s", attribute_str);
    free(attribute_str); attribute_str = 0;
  }
  if(filter_str != 0) {
    free(filter_str);
    filter_str = 0;
  }

  burrow_log_debug(backend->burrow,
		   "URL to send is \"%s\"\n",
		   url);

  curl_easy_setopt(chandle, CURLOPT_URL, url);
  free(url); url=0;

  /* set up libcurl command and related stuff... */
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

  /* setup buffer for libcurl to use for response */
  user_buffer *buffer = user_buffer_create(0,0);
  curl_easy_setopt(chandle, CURLOPT_WRITEFUNCTION,
		   user_buffer_curl_write_function);
  curl_easy_setopt(chandle, CURLOPT_WRITEDATA, buffer);

  if ((command == BURROW_CMD_UPDATE_MESSAGE) ||
      (command == BURROW_CMD_UPDATE_MESSAGES)) {
    curl_easy_setopt(chandle, CURLOPT_READFUNCTION,
		     user_buffer_curl_read_nothing_function);
  }

  curl_easy_setopt(chandle, CURLOPT_DEBUGFUNCTION,
		   burrow_backend_http_curldebug);
  curl_easy_setopt(chandle, CURLOPT_DEBUGDATA, backend);
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

/**
 * Delete a message
 *
 * @param ptr pointer to a backend object
 * @param cmd pointer to a parsed command structure
 * @return 0 if successful, errno if not
 */
static int
burrow_backend_http_delete_message(void *ptr, const burrow_command_st *cmd)
{
  return burrow_backend_http_common_getting(ptr, cmd);
}

/**
 * Delete multiple messages
 *
 * @param ptr pointer to a backend object
 * @param cmd pointer to a parsed command structure
 * @return 0 if successful, errno if not
 */
static int
burrow_backend_http_delete_messages(void *ptr, const burrow_command_st *cmd)
{
  return burrow_backend_http_common_getting(ptr, cmd);
}

/**
 * get a message from a burrow server
 *
 * @param ptr pointer to a backend object
 * @param cmd pointer to a parsed command structure
 * @return 0 if successful, errno if not
 */
static int
burrow_backend_http_get_message(void *ptr, const burrow_command_st *cmd)
{
  return burrow_backend_http_common_getting(ptr, cmd);
}

/**
 * Get messages from a burrow server
 * @param ptr pointer to a backend object
 * @param cmd pointer to a parsed command structure
 * @return 0 if successful, errno if not
 */
static int
burrow_backend_http_get_messages(void *ptr, const burrow_command_st *cmd)
{
  return burrow_backend_http_common_getting(ptr, cmd);
}

/**
 * Update a message on a burrow server
 * @param ptr pointer to a backend object
 * @param cmd pointer to a parsed command structure
 * @return 0 if successful, errno if not
 */
static int
burrow_backend_http_update_message(void *ptr, const burrow_command_st *cmd)
{
  return burrow_backend_http_common_getting(ptr, cmd);
}

/**
 * Update 0 or more messages on a burrow server
 *
 * @param ptr pointer to a backend object
 * @param cmd pointer to a parsed command structure
 * @return 0 if successful, errno if not
 */
static int
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

  .set_option = &burrow_backend_http_set_option,
  .set_option_int = NULL,

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
