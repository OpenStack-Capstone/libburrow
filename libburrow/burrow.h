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
 * @brief Burrow frontend declarations
 */

#ifndef __BURROW_BURROW_H
#define __BURROW_BURROW_H

#ifndef __cplusplus
# include <stdbool.h>
#endif

#include <inttypes.h> /* for uint32_t, etc */
#include <sys/types.h> /* for size_t, etc */

#include <libburrow/visibility.h>
#include <libburrow/constants.h>
#include <libburrow/structs.h>
#include <libburrow/attributes.h>
#include <libburrow/filters.h>

#ifdef  __cplusplus
extern "C" {
#endif

/* Burrow Object Functions */

/**
 * Creates a new burrow object with specified backend at (optinally)
 * specified memory location. Malloc will be used in certain cases.
 *
 * @param burrow Optional destination. If NULL, a new location will be malloc'd
 * @param backend String indicating which backend to load and use
 * @return A handle to an initialized burrow_st *, or NULL on error
 */
BURROW_API
burrow_st *burrow_create(burrow_st *burrow, const char *backend);

/**
 * Frees a burrow object, utilizing the free function set in the burrow
 * object (if set).
 *
 * @param burrow Burrow object to be freed
 */
BURROW_API
void burrow_destroy(burrow_st *burrow);

/**
 * Returns the size required to allocate a burrow object with the
 * specified backend.
 *
 * @param backend Backend descriptor for size calculation
 * @return The size of a complete burrow structure w/ backend; if the specified
 *         backend is NULL or does not exist, then 0 is returned.
 */
BURROW_API
size_t burrow_size(const char *backend);

/**
 * Sets an associated context pointer. This will be passed to all
 * callback functions.
 *
 * @param burrow Burrow object
 * @param context Pointer to context
 */
BURROW_API
void burrow_set_context(burrow_st *burrow, void *context);

/**
 * Gets the associated context pointer.
 *
 * @param burrow Burrow object
 */
BURROW_API
void *burrow_get_context(burrow_st *burrow);

/**
 * Sets burrow options
 *
 * @param burrow Burrow object
 * @param options Options to be set
 */
BURROW_API
void burrow_set_options(burrow_st *burrow, burrow_options_t options);

/**
 * Enables specified options.
 *
 * @param burrow Burrow object
 * @param options Options to be enabled
 */
BURROW_API
void burrow_add_options(burrow_st *burrow, burrow_options_t options);

/**
 * Disables specified options.
 *
 * @param burrow Burrow object
 * @param options Options to be disabled
 */
BURROW_API
void burrow_remove_options(burrow_st *burrow, burrow_options_t options);

/**
 * Returns current burrow options.
 *
 * @param burrow Burrow object
 * @return Current burrow options
 */
BURROW_API
burrow_options_t burrow_get_options(burrow_st *burrow);

/**
 * Sets a string backend option.
 *
 * @param burrow Burrow object
 * @param option Option name
 * @param value  Option value, string
 * @return 0 on success, errno result otherwise (e.g., EINVAL)
 */
BURROW_API
int burrow_set_backend_option(burrow_st *burrow,
                              const char *option,
                              const char *value);

/**
 * Sets an integer backend option. If the specified option doesn't exist,
 * EINVAL will be returned.
 *
 * @param burrow Burrow object
 * @param option Option name
 * @param value  Option value, int32
 * @return 0 on success, errno result otherwise (e.g., EINVAL)
 */
BURROW_API
int burrow_set_backend_option_int(burrow_st *burrow,
                                  const char *option,
                                  int32_t value);

/**
 * Sets the message-received callback. Called once per message received.
 * If a list of messages is received, multiple consecutive calls may occur
 * before the command-complete function is called.
 *
 * @param burrow Burrow object
 * @param callback Pointer to message-received function
 */
BURROW_API
void burrow_set_message_fn(burrow_st *burrow, burrow_message_fn *callback);

/**
 * Sets the account-received callback. Called when account detail is received.
 *
 * @param burrow Burrow object
 * @param callback Pointer to account-received function
 */
BURROW_API
void burrow_set_account_fn(burrow_st *burrow, burrow_account_fn *callback);

/**
 * Sets the queue-received callback. Called when queue detail is received.
 *
 * @param burrow Burrow object
 * @param callback Pointer to queue-received function
 */
BURROW_API
void burrow_set_queue_fn(burrow_st *burrow, burrow_queue_fn *callback);

/**
 * Sets the logging function. Note that this function doubles as an error
 * notification function for the user. Note also that errors may occur
 * midway through processing a command result, such that any of the other
 * 'received' callbacks may be called in advance of certain errors.
 *
 * @param burrow Burrow object
 * @param callback Pointer to event-log function
 */
BURROW_API
void burrow_set_log_fn(burrow_st *burrow, burrow_log_fn *callback);

/**
 * Sets the command-complete function, called when a command completes,
 * regardless of whether it was succesful or not.
 *
 * @param burrow Burrow object
 * @param callback Pointer to command-complete function
 */
BURROW_API
void burrow_set_complete_fn(burrow_st *burrow, burrow_complete_fn *callback);

/**
 * Sets the event-wait function. This function is called when a burrow
 * command would block; it should either block over the given file descriptor
 * or add it to a list of fds to be selected over. If this function is
 * not set, an internal blocking function will be used.
 *
 * @param burrow Burrow object
 * @param callback Pointer to event-wait function
 */
BURROW_API
void burrow_set_watch_fd_fn(burrow_st *burrow, burrow_watch_fd_fn *callback);

/**
 * Sets the malloc function burrow uses to allocate memory. Defaults to malloc.
 *
 * @param burrow Burrow object
 * @param func Pointer to malloc function
 */
BURROW_API
void burrow_set_malloc_fn(burrow_st *burrow, burrow_malloc_fn *func);

/**
 * Sets the free function burrow uses to deallocate memory, and itself.
 * Defaults to free.
 *
 * @param burrow Burrow object
 * @param func Pointer to free function
 */
BURROW_API
void burrow_set_free_fn(burrow_st *burrow, burrow_free_fn *func);


/* Burrow Command Functions */

/**
 * Sets up burrow to issue a create_message command.
 *
 * If burrow is already issuing a command, this will fail and trigger
 * a warning.
 *
 * @param burrow Burrow object
 * @param account Account name
 * @param queue Queue name
 * @param message_id Message id
 * @param body The message body
 * @param body_size The size of the message body in bytes
 * @param attributes Message attributes, may be NULL
 * @return 0 on command completion or an errno value on error, such as
 *         EINPROGRESS or EINVAL
 */
BURROW_API
int burrow_create_message(burrow_st *burrow,
                          const char *account, 
                          const char *queue, 
                          const char *message_id,
                          const void *body,
                          size_t body_size,
                          const burrow_attributes_st *attributes);


/**
 * Sets up burrow to issue an update_message command.
 *
 * If burrow is already issuing a command, this will fail and trigger
 * a warning.
 *
 * @param burrow Burrow object
 * @param account Account name
 * @param queue Queue name
 * @param message_id Message id
 * @param attributes Message attributes
 * @param filters Filters, may be NULL
 * @return 0 on command completion or an errno value on error, such as
 *         EINPROGRESS or EINVAL
 */
BURROW_API
int burrow_update_message(burrow_st *burrow,
                          const char *account,
                          const char *queue,
                          const char *message_id,
                          const burrow_attributes_st *attributes,
                          const burrow_filters_st *filters);

/**
 * Sets up burrow to issue a get_message command.
 *
 * If burrow is already issuing a command, this will fail and trigger
 * a warning.
 *
 * @param burrow Burrow object
 * @param account Account name
 * @param queue Queue name
 * @param message_id Message id
 * @param filters Filters, may be NULL
 * @return 0 on command completion or an errno value on error, such as
 *         EINPROGRESS or EINVAL
 */
BURROW_API
int burrow_get_message(burrow_st *burrow,
                       const char *account,
                       const char *queue,
                       const char *message_id,
                       const burrow_filters_st *filters);

/**
 * Sets up burrow to issue a delete_message command.
 *
 * If burrow is already issuing a command, this will fail and trigger
 * a warning.
 *
 * @param burrow Burrow object
 * @param account Account name
 * @param queue Queue name
 * @param message_id Message id
 * @param filters Filters, may be NULL
 * @return 0 on command completion or an errno value on error, such as
 *         EINPROGRESS or EINVAL
 */
BURROW_API
int burrow_delete_message(burrow_st *burrow,
                          const char *account,
                          const char *queue,
                          const char *message_id,
                          const burrow_filters_st *filters);

/**
 * Sets up burrow to issue a get_messages command.
 *
 * If burrow is already issuing a command, this will fail and trigger
 * a warning.
 *
 * @param burrow Burrow object
 * @param account Account name
 * @param queue Queue name
 * @param filters Filters to apply, may be NULL
 * @return 0 on command completion or an errno value on error, such as
 *         EINPROGRESS or EINVAL
 */
BURROW_API
int burrow_get_messages(burrow_st *burrow,
                        const char *account,
                        const char *queue,
                        const burrow_filters_st *filters);

/**
 * Sets up burrow to issue a delete_messages command.
 *
 * If burrow is already issuing a command, this will fail and trigger
 * a warning.
 *
 * @param burrow Burrow object, non-NULL
 * @param account Account name, non-NULL
 * @param queue Queue name, non-NULL
 * @param filters Filters to apply, may be NULL
 * @return 0 on command completion or an errno value on error, such as
 *         EINPROGRESS or EINVAL
 */
BURROW_API
int burrow_delete_messages(burrow_st *burrow,
                           const char *account,
                           const char *queue,
                           const burrow_filters_st *filters);

/**
 * Sets up burrow to issue an update_messages command.
 *
 * If burrow is already issuing a command, this will fail and trigger
 * a warning.
 *
 * @param burrow Burrow object
 * @param account Account name
 * @param queue Queue name
 * @param attributes Attributes to set
 * @param filters Filters to apply, may be NULL
 * @return 0 on command completion or an errno value on error, such as
 *         EINPROGRESS or EINVAL
 */
BURROW_API
int burrow_update_messages(burrow_st *burrow,
                           const char *account,
                           const char *queue,
                           const burrow_attributes_st *attributes,
                           const burrow_filters_st *filters);

/**
 * Sets up burrow to issue a get_queues command.
 *
 * If burrow is already issuing a command, this will fail and trigger
 * a warning.
 *
 * @param burrow Burrow object
 * @param account Account name
 * @param filters Filters to apply, may be NULL
 * @return 0 on command completion or an errno value on error, such as
 *         EINPROGRESS or EINVAL
 */
BURROW_API
int burrow_get_queues(burrow_st *burrow,
                      const char *account,
                      const burrow_filters_st *filters);

/**
 * Sets up burrow to issue a delete_queues command.
 *
 * If burrow is already issuing a command, this will fail and trigger
 * a warning.
 *
 * @param burrow Burrow object
 * @param account Account name
 * @param filters Filters to apply, may be NULL
 * @return 0 on command completion or an errno value on error, such as
 *         EINPROGRESS or EINVAL
 */
BURROW_API
int burrow_delete_queues(burrow_st *burrow,
                         const char *account,
                         const burrow_filters_st *filters);

/**
 * Sets up burrow to issue a get_accounts command.
 *
 * If burrow is already issuing a command, this will fail and trigger
 * a warning.
 *
 * @param burrow Burrow object
 * @param filters Filters to apply, may be NULL
 * @return 0 on command completion or an errno value on error, such as
 *         EINPROGRESS or EINVAL
 */
BURROW_API
int burrow_get_accounts(burrow_st *burrow, const burrow_filters_st *filters);

/**
 * Sets up burrow to issue a delete_accounts command.
 *
 * If burrow is already issuing a command, this will fail and trigger
 * a warning.
 *
 * @param burrow Burrow object
 * @param filters Filters to apply, may be NULL
 * @return 0 on command completion or an errno value on error, such as
 *         EINPROGRESS or EINVAL
 */
BURROW_API
int burrow_delete_accounts(burrow_st *burrow, const burrow_filters_st *filters);

/**
 * Cancels an ongoing command. 
 * Will trigger a command-complete callback if any command is pending.
 *
 * @param burrow Burrow object
 */
BURROW_API
void burrow_cancel(burrow_st *burrow);

/**
 * Begins or continues processing of the current burrow command.
 * If there is no command, returns immediately. If a command completes
 * inside of burrow_process, the command-complete callback will be
 * invoked, after which point burrow will see if any new command has
 * been issues which it can continue processing.
 *
 * @param burrow Burrow object
 * @return 0 on command completion, EAGAIN if burrow would block,
 *         or another error code if an error occurred
 */
BURROW_API
int burrow_process(burrow_st *burrow);

/**
 * Notify burrow that an event has occurred on a given file descriptor.
 * Behavior is undefined if this event was not requested to be waited
 * upon by the event-wait callback above.
 *
 * @param burrow Burrow object
 * @param fd Which file descriptor
 * @param event Which event(s) occurred
 */
BURROW_API
int burrow_event_raised(burrow_st *burrow, int fd, burrow_ioevent_t event);

/**
 * Returns a string describing the verbosity level. Useful for logging.
 *
 * @param verbose Verbosity level.
 * @return A string describing the passed in verbosity level.
 */
BURROW_API
const char *burrow_verbose_name(burrow_verbose_t verbose);

/**
 * Sets the verbosity level of a burrow instance.
 *
 * @param burrow Burrow object
 * @param verbosity Verbosity level
 */
BURROW_API
void burrow_set_verbosity(burrow_st *burrow, burrow_verbose_t verbosity);


#ifdef  __cplusplus
}
#endif

#endif /* __BURROW_BURROW_H */
