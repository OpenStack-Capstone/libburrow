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
 * @brief Macros and inline functions
 */


#ifndef __BURROW_MACROS_H
#define __BURROW_MACROS_H

static inline void burrow_callback_message(burrow_st *burrow,
                                 const char *message_id,
                                 const void *body,
                                 size_t body_size,
                                 const burrow_attributes_st *attributes)
{
  if (burrow->message_fn)
    burrow->message_fn(burrow, message_id, body, body_size, attributes);
}

static inline void burrow_callback_queue(burrow_st *burrow, const char *queue)
{
  if (burrow->queue_fn)
    burrow->queue_fn(burrow, queue);
}

static inline void burrow_callback_account(burrow_st *burrow, const char *account)
{
  if (burrow->account_fn)
    burrow->account_fn(burrow, account);
}

static inline void burrow_callback_complete(burrow_st *burrow)
{
  if (burrow->complete_fn)
    burrow->complete_fn(burrow);
}

static inline void burrow_watch_fd(burrow_st *burrow, int fd, burrow_ioevent_t events)
{
  if (burrow->watch_fd_fn)
    burrow->watch_fd_fn(burrow, fd, events);
  else
    burrow_internal_watch_fd(burrow, fd, events);
}

static inline void *burrow_malloc(burrow_st *burrow, size_t size)
{
  if (burrow->malloc_fn)
    return burrow->malloc_fn(burrow, size);
  else
    return malloc(size);
}

static inline void burrow_free(burrow_st *burrow, void *ptr)
{
  if (burrow->free_fn)
    burrow->free_fn(burrow, ptr);
  else
    free(ptr);
}

static inline void burrow_error(burrow_st *burrow, int err, const char *msg, ...)
{
  va_list args;
  (void) err;

  if (burrow->verbose <= BURROW_VERBOSE_ERROR) {
    va_start(args, msg);
    burrow_internal_log(burrow, BURROW_VERBOSE_ERROR, msg, args);
    va_end(args);
  }
}

static inline void burrow_log_fatal(burrow_st *burrow, const char *msg, ...)
{
  va_list args;

  if (burrow->verbose <= BURROW_VERBOSE_FATAL) {
    va_start(args, msg);
    burrow_internal_log(burrow, BURROW_VERBOSE_FATAL, msg, args);
    va_end(args);
  }
}

static inline void burrow_log_error(burrow_st *burrow, const char *msg, ...)
{
  va_list args;

  if (burrow->verbose <= BURROW_VERBOSE_ERROR) {
    va_start(args, msg);
    burrow_internal_log(burrow, BURROW_VERBOSE_ERROR, msg, args);
    va_end(args);
  }
}

static inline void burrow_log_warn(burrow_st *burrow, const char *msg, ...)
{
  va_list args;

  if (burrow->verbose <= BURROW_VERBOSE_WARN) {
    va_start(args, msg);
    burrow_internal_log(burrow, BURROW_VERBOSE_WARN, msg, args);
    va_end(args);
  }
}

static inline void burrow_log_info(burrow_st *burrow, const char *msg, ...)
{
  va_list args;

  if (burrow->verbose <= BURROW_VERBOSE_INFO) {
    va_start(args, msg);
    burrow_internal_log(burrow, BURROW_VERBOSE_INFO, msg, args);
    va_end(args);
  }
}

static inline void burrow_log_debug(burrow_st *burrow, const char *msg, ...)
{
  va_list args;

  if (burrow->verbose <= BURROW_VERBOSE_DEBUG) {
    va_start(args, msg);
    burrow_internal_log(burrow, BURROW_VERBOSE_DEBUG, msg, args);
    va_end(args);
  }
}

#endif