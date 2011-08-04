/*
 * libburrow -- Burrow Client Library
 *
 * Copyright 2011 Tony Wooster 
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
 * @brief Library-wide constants
 */

#ifndef __BURROW_CONSTANTS_H
#define __BURROW_CONSTANTS_H

#ifdef __cplusplus
extern "C" {
#endif

#define BURROW_MAX_ERROR_SIZE 1024
#define BURROW_VERBOSE_DEFAULT BURROW_VERBOSE_ALL

typedef enum {
  BURROW_VERBOSE_ALL,
  BURROW_VERBOSE_DEBUG,
  BURROW_VERBOSE_INFO,
  BURROW_VERBOSE_WARN,
  BURROW_VERBOSE_ERROR,
  BURROW_VERBOSE_FATAL,
  BURROW_VERBOSE_NONE
} burrow_verbose_t;

/**
 * User options
 */
typedef enum {
  BURROW_OPT_AUTOPROCESS  = (1 << 0),
  BURROW_OPT_COPY_STRINGS = (1 << 1), /*!< Not yet implemented */
  BURROW_OPT_MAX          = (1 << 2)
} burrow_options_t;
 
/**
 * Detail levels burrow can request.
 */
typedef enum {
  BURROW_DETAIL_NONE,
  BURROW_DETAIL_ID,
  BURROW_DETAIL_ATTRIBUTES,
  BURROW_DETAIL_BODY,
  BURROW_DETAIL_ALL
} burrow_detail_t;

/**
 * IO events that Burrow may request waiting upon.
 */
typedef enum {
  BURROW_IOEVENT_NONE  = 0,
  BURROW_IOEVENT_WRITE = (1 << 0),
  BURROW_IOEVENT_READ  = (1 << 1),
  BURROW_IOEVENT_ALL   = BURROW_IOEVENT_WRITE | BURROW_IOEVENT_READ
} burrow_ioevent_t;

/* Used publicly */
typedef struct burrow_filters_st burrow_filters_st;
typedef struct burrow_attributes_st burrow_attributes_st;
typedef struct burrow_st burrow_st;

/* Function pointers for user callbacks */

/**
 * Signature for a message callback function.
 *
 * Called when Burrow receives a message and is to relay this to the user.
 *
 * See: burrow_set_message_fn
 *
 * @param burrow Burrow object that is invoking this callback
 * @param message_id Message id, may be NULL
 * @param body Message body, may be NULL
 * @param body_size Mesage body size, undefined if body NULL
 * @param atttributes Message attributes, may be NULL
 */
typedef void (burrow_message_fn)(burrow_st *burrow,
                                 const char *message_id,
                                 const void *body,
                                 size_t body_size,
                                 const burrow_attributes_st *attributes);

/**
 * Signature for a queue callback function.
 *
 * Called when Burrow receives information about a queue and is to relay
 * this to the user.
 *
 * See: burrow_set_queue_fn()
 *
 * @param burrow Burrow object that is invoking this callback
 * @param queue The name of the queue
 */
typedef void (burrow_queue_fn)(burrow_st *burrow, const char *queue);

/**
 * Signature for a account callback function.
 * 
 * Called when Burrow receives information about a account and is to relay
 * this to the user.
 *
 * See: burrow_set_account_fn()
 *
 * @param burrow Burrow object that is invoking this callback
 * @param account The name of the account
 */
typedef void (burrow_account_fn)(burrow_st *burrow, const char *account);

/**
 * Signature for a logging callback function. Called when Burrow wants to
 * log information. Messages will be pre-filtered for verbosity level before
 * being sent to this function. burrow_set_verbosity sets the verbosity level.
 * burrow_verbose_name can retrieve a descriptive string.
 *
 * See: burrow_set_log_fn()
 *
 * @param burrow Burrow object that is invoking this callback
 * @param verbose The verbosity level of this message
 * @param msg The message to log
 */
typedef void (burrow_log_fn)(burrow_st *burrow,
                             burrow_verbose_t verbose,
                             const char *msg);

/**
 * Signature for a command-complete callback function. Called when Burrow has
 * finished processing a command for whatever reason (success, error).
 * Indicates that burrow is ready to accept new commands.
 *
 * See: burrow_set_complete_fn()
 *
 * @param burrow Burrow object that is invoking this callback
 */
typedef void (burrow_complete_fn)(burrow_st *burrow);

/**
 * Signature for a file-descriptor watching callback function.
 *
 * Called when burrow needs the user to watch a specific fd for events
 * so that processing can continue.
 *
 * See: burrow_set_watch_fd_fn()
 *
 * @param burrow Burrow object that is invoking this callback
 * @param fd Which file descriptor to watch
 * @param event Bitmask of which event(s) to watch for
 */
typedef void (burrow_watch_fd_fn)(burrow_st *burrow,
                                  int fd,
                                  burrow_ioevent_t event);

/**
 * Signature for a user-overridable malloc function.
 *
 * Called when burrow or its backends need to allocate memory.
 *
 * See: burrow_set_malloc_fn()
 *
 * @param burrow Burrow object that is invoking this callback
 * @param size How much space to allocate
 */
typedef void *(burrow_malloc_fn)(burrow_st *burrow, size_t size);

/**
 * Signature for a user-overridable free function.
 *
 * Called when burrow or its backends need to free memory. Please note,
 * this and burrow_malloc_fn should be set immediately adjacent to eachother
 * to prevent potential conflicts.
 *
 * See: burrow_set_free_fn()
 *
 * @param burrow Burrow object that is invoking this callback
 * @param ptr Which memory to free
 */
typedef void (burrow_free_fn)(burrow_st *burrow, void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* __BURROW_CONSTANTS_H */
