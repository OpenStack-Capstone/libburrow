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
 * @brief Defines the JSON processing related functions for libburrow
 */

#include "config.h"
#include <libburrow/common.h>

#include "curl_backend.h"
#include "user_buffer.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include "json_processing.h"
#include <libburrow/backends/http/parser/contrib/JSON_PARSER/JSON_parser.h>

struct json_processing_st {
  burrow_backend_t* backend;
  char *body;
  size_t body_size;
  char *message_id;
  int is_key;
  char *key;
  burrow_attributes_st *attributes;

};

static json_processing_t*
burrow_easy_json_st_create(burrow_backend_t* backend)
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
    burrow_attributes_destroy(jproc->attributes);
  free(jproc);
}

/**
 * Called by the JSON_parser when it has parsed something new.  This will
 * take whatever has been parsed, and try to store it up, or if it has
 * something complete (like a complete burrow message), it will call the
 * appropriate function to deal with it.
 *
 * @param ctx the json_processing_t struct it uses to keep track of things
 * @param type the type of whatever the JSON parser has just parsed
 * @param value the value of whatever the JSON parser has just parsed
 * @return 1 if good, 0 if bad
 */
 
static int burrow_backend_http_json_callback(void *ctx,
					     int type,
					     const JSON_value* value)
{
  json_processing_t* jproc = (json_processing_t *)ctx;

  // determine what command is being processed.
  burrow_command_t bcommand = burrow_backend_http_get_command(jproc->backend);

  /* This section is for commands that return burrow messages, whether they
     return one message or a list of messages.
  */
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
	/*
	 * we got the end of an object.  We take this to mean we have
	 * a complete message at this point, which we should pass off
	 * to the appropriate callback.
	 */
	burrow_callback_message(burrow_backend_http_get_burrow(jproc->backend),
				jproc->message_id,
				(uint8_t *)jproc->body,
				jproc->body_size,
				jproc->attributes);
	/* clean up for the next message, in case there is one */
	if (jproc->message_id) {
	  free(jproc->message_id);
	  jproc->message_id = 0;
	}
	if (jproc->body) {
	  free(jproc->body);
	  jproc->body = 0;
	}
	jproc->body_size = 0;
	/* Make sure we have a clean set of attributes */
	if (jproc->attributes) {
	  burrow_attributes_unset_all(jproc->attributes);
	} else {
	  jproc->attributes = burrow_attributes_create(0,0);
	}

	break;
      case JSON_T_KEY:
	// We don't check if the key is valid until later,
	// when we have the value
	jproc->is_key = 1;
	if (jproc->key)
	  free(jproc->key);
	jproc->key = strdup(value->vu.str.value);
	break;
      case JSON_T_STRING:
	// Now check if key is valid
	if(jproc->is_key) {
	  jproc->is_key = 0;
	  if (strcmp(jproc->key, "id") == 0) {
	    char *message_id =
	      curl_easy_unescape(burrow_backend_http_get_curl_easy_handle(jproc->backend),
				 value->vu.str.value,
				 0, 0);
	    jproc->message_id = strdup(message_id);
	    curl_free(message_id);
	  } else if (strcmp(jproc->key, "body") == 0) {
	    jproc->body = strdup(value->vu.str.value);
	    jproc->body_size = strlen(value->vu.str.value);
	  } else {
	    burrow_error(burrow_backend_http_get_burrow(jproc->backend),
			 EINVAL,
			 "ERROR!  unrecognized string valued key \"%s\"=\"%s\"\n", jproc->key, value->vu.str.value);
	    return 0;
	  }
	} else {
	  // We never got a key?
	  // is this even possible without the json parser yakking?
	  return 0;
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
	  } else {
	     burrow_error(burrow_backend_http_get_burrow(jproc->backend),
			  EINVAL,
			  "WARNING! JSON parsing found unrecognized integer key \"%s\"=%d\n",
			  jproc->key, value->vu.integer_value);
	    return 0;
	  }
	}
	break;
      default:
	burrow_error(burrow_backend_http_get_burrow(jproc->backend),
		     EINVAL,
		     "WARNING! JSON parsing found unexpected type = %d\n",
		     type);
	return 0;
	break;
      }
    } else if ((bcommand == BURROW_CMD_GET_QUEUES) ||
	       (bcommand == BURROW_CMD_GET_ACCOUNTS))
    {
      /* This section is for commands that return lists of strings, such as when
       * you list the queues or the accounts
       */
      switch(type) {
      case JSON_T_ARRAY_BEGIN:
	break;
      case JSON_T_ARRAY_END:
	/* Does not seem to be anything to do here. */
	break;
      case JSON_T_STRING:
	if (bcommand == BURROW_CMD_GET_ACCOUNTS) {
	  char *account = curl_easy_unescape(burrow_backend_http_get_curl_easy_handle(jproc->backend),
					     value->vu.str.value, 0,0);
	  burrow_callback_account(burrow_backend_http_get_burrow(jproc->backend),
				  account);
	  curl_free(account);
	} else if (bcommand == BURROW_CMD_GET_QUEUES) {
	  char *queue = curl_easy_unescape(burrow_backend_http_get_curl_easy_handle(jproc->backend),
					   value->vu.str.value, 0,0);
	  burrow_callback_queue(burrow_backend_http_get_burrow(jproc->backend),
				queue);
	  curl_free(queue);
	}
	break;
      default:
	burrow_error(burrow_backend_http_get_burrow(jproc->backend),
		     EINVAL,
		     "WARNING! JSON parsing found unexpected type = %d\n",
		     type);

	return 0;
	break;
      }
    }

  return 1;
}

/**
 * Process a JSON object that we got from the burrow server.j
 * Should normally only be called on a valid JSON object, I think.
 *
 * @param backend the http backend
 * @param jsontext the actual JSON text we got from the burrow server
 * @param jsonsize the size of the JSON text we got from the burrow server
 * @return 0 if successful, negative if parsing fails
 */
int
burrow_backend_http_parse_json(burrow_backend_t *backend,
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

  int i;
  for (i = 0; i < (int)jsonsize; ++i) {
    int retval;
    int nextchar = jsontext[i];
    if ((retval = JSON_parser_char(jc, nextchar)) <= 0) {
      burrow_error(burrow_backend_http_get_burrow(backend),
		   EINVAL,
		   "WARNING! JSON_parser_char (%d) at byte %d (%d = '%c')\n",
		   retval, i, (int)nextchar, nextchar);
      return -1;
    }
  }    
  if (!JSON_parser_done(jc)) {
    burrow_error(burrow_backend_http_get_burrow(backend),
		 EINVAL,
		 "WARNING! JSON_parser_end indicates JSON syntax error\n");
    delete_JSON_parser(jc);
    burrow_easy_json_st_destroy(json_processing);
    return -1;
  }
  delete_JSON_parser(jc);
  burrow_easy_json_st_destroy(json_processing);
  return 0;
}

