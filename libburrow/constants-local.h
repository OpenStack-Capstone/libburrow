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
 * @brief Private constants/typedefs
 */


#ifndef __BURROW_CONSTANTS_LOCAL_H
#define __BURROW_CONSTANTS_LOCAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Used internally */
typedef struct burrow_command_st burrow_command_st;
typedef struct burrow_backend_functions_st burrow_backend_functions_st;

/* Function pointers used for backend communication */
typedef void *(burrow_backend_create_fn)(void *dest, burrow_st *burrow);
typedef void (burrow_backend_destroy_fn)(void *backend);
typedef size_t (burrow_backend_size_fn)(void);

typedef int (burrow_backend_set_option_fn)(void *backend,
                                           const char *key,
                                           const char *value);
typedef int (burrow_backend_set_option_int_fn)(void *backend,
                                               const char *key,
                                               int32_t value);

typedef void (burrow_backend_cancel_fn)(void *backend);
typedef int (burrow_backend_process_fn)(void *backend);
typedef int (burrow_backend_event_raised_fn)(void *backend,
                                             int fd,
                                             burrow_ioevent_t event);

typedef int (burrow_backend_command_fn)(void *backend,
                                        const burrow_command_st *command);

typedef enum {
  BURROW_STATE_IDLE,
  BURROW_STATE_START,
  BURROW_STATE_WAITING,
  BURROW_STATE_READY,
  BURROW_STATE_FINISH
} burrow_state_t;

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
  BURROW_FILTERS_DETAIL       = (1 << 2),
  BURROW_FILTERS_WAIT         = (1 << 3),
  BURROW_FILTERS_ALL = ~BURROW_FILTERS_NONE
} burrow_filters_set_t;

/* Internal flags */
typedef enum {
  BURROW_FLAG_SELFALLOCATED = (1 << 0),
  BURROW_FLAG_PROCESSING    = (1 << 1),
  BURROW_FLAG_MAX           = (1 << 2)
} burrow_flags_t; 

#ifdef __cplusplus
}
#endif

#endif /* __BURROW_CONSTANTS_LOCAL_H */