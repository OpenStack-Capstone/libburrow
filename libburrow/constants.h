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
  BURROW_STATE_IDLE,
  BURROW_STATE_START,
  BURROW_STATE_WAITING,
  BURROW_STATE_READY,
  BURROW_STATE_FINISH
} burrow_state_t;

typedef enum {
  BURROW_VERBOSE_ALL,
  BURROW_VERBOSE_DEBUG,
  BURROW_VERBOSE_INFO,
  BURROW_VERBOSE_WARN,
  BURROW_VERBOSE_ERROR,
  BURROW_VERBOSE_FATAL,
  BURROW_VERBOSE_NONE
} burrow_verbose_t;

typedef enum {
  BURROW_CMD_GET_ACCOUNTS,
  BURROW_CMD_DELETE_ACCOUNTS,

  BURROW_CMD_GET_QUEUES,
  BURROW_CMD_DELETE_QUEUES,

  BURROW_CMD_GET_MESSAGES,
  BURROW_CMD_UPDATE_MESSAGES,
  BURROW_CMD_DELETE_MESSAGES,
  
  BURROW_CMD_GET_MESSAGE,
  BURROW_CMD_UPDATE_MESSAGE,
  BURROW_CMD_DELETE_MESSAGE,
  BURROW_CMD_CREATE_MESSAGE,

  BURROW_CMD_MAX,

  BURROW_CMD_NONE = BURROW_CMD_MAX
} burrow_command_t;

/* User options */
typedef enum {
  BURROW_OPT_AUTOPROCESS  = (1 << 0),
  BURROW_OPT_COPY_STRINGS = (1 << 1),
  BURROW_OPT_MAX          = (1 << 2)
} burrow_options_t;
  
/* Internal flags */
typedef enum {
  BURROW_FLAG_SELFALLOCATED = (1 << 0),
  BURROW_FLAG_PROCESSING    = (1 << 1),
  BURROW_FLAG_MAX           = (1 << 2)
} burrow_flags_t; 
  
typedef enum {
  BURROW_DETAIL_NONE,
  BURROW_DETAIL_ID,
  BURROW_DETAIL_ATTRIBUTES,
  BURROW_DETAIL_BODY,
  BURROW_DETAIL_ALL
} burrow_detail_t;

typedef enum {
  BURROW_IOEVENT_NONE  = 0,
  BURROW_IOEVENT_WRITE = (1 << 0),
  BURROW_IOEVENT_READ  = (1 << 1),
  BURROW_IOEVENT_ALL   = BURROW_IOEVENT_WRITE | BURROW_IOEVENT_READ
} burrow_ioevent_t;

typedef enum {
  BURROW_OK = 0,
  BURROW_OK_WAITING,
  BURROW_ERROR_NOT_READY,
  BURROW_ERROR_MEMORY,
  BURROW_ERROR_CONNECTION,
  BURROW_ERROR_BAD_ARGS,
  BURROW_ERROR_SERVER,
  BURROW_ERROR_INTERNAL,
  BURROW_ERROR_UNSUPPORTED,
  BURROW_FATAL
} burrow_result_t;

typedef enum {
  BURROW_ATTRIBUTES_NONE = 0,
  BURROW_ATTRIBUTES_TTL  = (1 << 0),
  BURROW_ATTRIBUTES_HIDE = (1 << 1),
  BURROW_ATTRIBUTES_ALL = ~BURROW_ATTRIBUTES_NONE
} burrow_attributes_set_t;

typedef enum {
  BURROW_FILTERS_NONE         = 0,
  BURROW_FILTERS_MATCH_HIDDEN = (1 << 0),
  BURROW_FILTERS_LIMIT        = (1 << 1),
  BURROW_FILTERS_MARKER       = (1 << 2),
  BURROW_FILTERS_DETAIL       = (1 << 3),
  BURROW_FILTERS_WAIT         = (1 << 4),
  BURROW_FILTERS_ALL = ~BURROW_FILTERS_NONE
} burrow_filters_set_t;

/* Used publicly */
typedef struct burrow_filters_st burrow_filters_st;
typedef struct burrow_attributes_st burrow_attributes_st;
typedef struct burrow_st burrow_st;

/* Used internally */
typedef struct burrow_command_st burrow_command_st;
typedef struct burrow_backend_functions_st burrow_backend_functions_st;

/* Function pointers for user callbacks */
typedef void (burrow_message_fn)(burrow_st *burrow,
                                 const char *message_id,
                                 const void *body,
                                 size_t body_size,
                                 const burrow_attributes_st *attributes);

typedef void (burrow_queue_fn)(burrow_st *burrow, const char *queue);

typedef void (burrow_account_fn)(burrow_st *burrow, const char *account);

typedef void (burrow_log_fn)(burrow_st *burrow, burrow_verbose_t verbose, const char *msg);

typedef void (burrow_complete_fn)(burrow_st *burrow);

typedef void (burrow_watch_fd_fn)(burrow_st *burrow, int fd, burrow_ioevent_t event);

typedef void *(burrow_malloc_fn)(burrow_st *burrow, size_t size);

typedef void (burrow_free_fn)(burrow_st *burrow, void *ptr);

/* Function pointers used for backend communication */
typedef void *(burrow_backend_create_fn)(void *dest, burrow_st *burrow);
typedef void (burrow_backend_destroy_fn)(void *backend);
typedef size_t (burrow_backend_size_fn)(void);
typedef void *(burrow_backend_clone_fn)(void *dst, void *src);

typedef burrow_result_t (burrow_backend_set_option_fn)(void *backend, const char *key, const char *value);

typedef void (burrow_backend_cancel_fn)(void *backend);
typedef burrow_result_t (burrow_backend_process_fn)(void *backend);
typedef burrow_result_t (burrow_backend_event_raised_fn)(void *backend, int fd, burrow_ioevent_t event);

typedef burrow_result_t (burrow_backend_command_fn)(void *backend, const burrow_command_st *command);

#ifdef __cplusplus
}
#endif

#endif /* __BURROW_CONSTANTS_H */
