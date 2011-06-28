/* Burrow Object Functions */

/**
 * Creates a new burrow object with specified backend at (optinally)
 * specified memory location. Malloc will be used in certain cases.
 *
 * @param burrow Optional destination. If NULL, a new location will be malloc'd
 * @param backend String indicating which backend to load and use
 * @return A handle to an initialized burrow_st *, or NULL on error
 */
burrow_st *burrow_create(burrow_st *burrow, char *backend);

/**
 * Frees a burrow object, utilizing the free function set in the burrow
 * object (if set).
 *
 * @param burrow Burrow object to be freed
 */
void burrow_free(burrow_st *burrow);

/**
 * Sets an associated context pointer. This will be passed to all
 * callback functions.
 *
 * @param burrow Burrow object
 * @param context Pointer to context
 */
void burrow_set_context(burrow_st *burrow, void *context);

/**
 * Gets the associated context pointer.
 *
 * @param burrow Burrow object
 */
void *burrow_get_context(burrow_st *burrow);

/**
 * Sets burrow options
 *
 * @param burrow Burrow object
 * @param options Options to be set
 */
void burrow_set_options(burrow_options_t options);

/**
 * Enables specified options.
 *
 * @param burrow Burrow object
 * @param options Options to be enabled
 */
void burrow_add_options(burrow_options_t options);

/**
 * Disables specified options.
 *
 * @param burrow Burrow object
 * @param options Options to be disabled
 */
void burrow_remove_options(burrow_options_t options_to_remove);

/**
 * Returns current burrow options.
 *
 * @param burrow Burrow object
 * @return Current burrow options
 */
burrow_options_t burrow_get_options();

/**
 * Sets a string backend option.
 *
 * @param burrow Burrow object
 * @param option Option name
 * @param value  Option value, string
 */
void burrow_backend_set_option(burrow_st *burrow, char *option, char *value);

/**
 * Sets an integer backend option.
 *
 * @param burrow Burrow object
 * @param option Option name
 * @param value  Option value, int32
 */
void burrow_backend_set_option_int(burrow_st *burrow, char *option, int32_t value);


/**
 * Sets the message-received callback. Called once per message received.
 * If a list of messages is received, multiple consecutive calls may occur
 * before the command-complete function is called.
 *
 * @param burrow Burrow object
 * @param callback Pointer to message-received function
 */
void burrow_set_message_fn(burrow_st *burrow, burrow_message_fn *callback);

/**
 * Sets the accounts-list-received callback. Called when a list of
 * accounts is received.
 *
 * @param burrow Burrow object
 * @param callback Pointer to accounts-list-received function
 */
void burrow_set_accounts_fn(burrow_st *burrow, burrow_accounts_fn *callback);

/**
 * Sets the queue-list-received callback. Called when a list of
 * queues is received.
 *
 * @param burrow Burrow object
 * @param callback Pointer to queue-list-received function
 */
void burrow_set_queues_fn(burrow_st *burrow, burrow_queues_fn *callback);

/**
 * Sets the logging function. Note that this function doubles as an error
 * notification function for the user. Note also that errors may occur
 * midway through processing a command result, such that any of the other
 * 'received' callbacks may be called in advance of certain errors.
 *
 * @param burrow Burrow object
 * @param callback Pointer to event-log function
 */
void burrow_set_log_fn(burrow_st *burrow, burrow_log_fn *callback);

/**
 * Sets the command-complete function, called when a command completes,
 * regardless of whether it was succesful or not.
 *
 * @param burrow Burrow object
 * @param callback Pointer to command-complete function
 */
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
void burrow_set_event_wait_fn(burrow_st *burrow, burrow_event_wait_fn *callback);

/**
 * Sets the malloc function burrow uses to allocate memory. Defaults to malloc.
 *
 * @param burrow Burrow object
 * @param func Pointer to malloc function
 */
void burrow_set_malloc_fn(burrow_st *burrow, burrow_malloc_fn *func);

/**
 * Sets the free function burrow uses to deallocate memory, and itself.
 * Defaults to free.
 *
 * @param burrow Burrow object
 * @param func Pointer to free function
 */
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
 * @param attributes Message attributes (ignored if NULL)
 */
void burrow_create_message(burrow_st *burrow,
                           char *account, char *queue, char *message_id,
                           uint8_t *body, size_t body_size
                           burrow_attributes_st *attributes);


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
 * @param attributes Message attributes; must not be NULL
 */
void burrow_update_message(burrow_st *burrow,
                           char *account, char *queue, char *message_id,
                           burrow_attributes_st *attributes);

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
 */
void burrow_get_message(burrow_st *burrow,
                        char *account, char *queue, char *message_id);

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
 */
void burrow_delete_message(burrow_st *burrow,
                           char *account, char *queue, char *message_id);

/**
 * Sets up burrow to issue a get_messages command.
 *
 * If burrow is already issuing a command, this will fail and trigger
 * a warning.
 *
 * @param burrow Burrow object
 * @param account Account name
 * @param queue Queue name
 * @param filters Filters to apply (may be NULL)
 */
void burrow_get_messages(burrow_st *burrow,
                         char *account, char *queue, burrow_filters_st *filters);

/**
 * Sets up burrow to issue a delete_messages command.
 *
 * If burrow is already issuing a command, this will fail and trigger
 * a warning.
 *
 * @param burrow Burrow object
 * @param account Account name
 * @param queue Queue name
 * @param filters Filters to apply (may be NULL)
 */
void burrow_delete_messages(burrow_st *burrow,
                            char *account, char *queue, burrow_filters_st *filters);

/**
 * Sets up burrow to issue an update_messages command.
 *
 * If burrow is already issuing a command, this will fail and trigger
 * a warning.
 *
 * @param burrow Burrow object
 * @param account Account name
 * @param queue Queue name
 * @param attributes Attributes to set; must not be null
 * @param filters Filters to apply (may be NULL)
 */
void burrow_update_messages(burrow_st *burrow,
                            char *account, char *queue, burrow_attributes_st *attributes,
                            burrow_filters_st *filters);

/**
 * Sets up burrow to issue a get_queues command.
 *
 * If burrow is already issuing a command, this will fail and trigger
 * a warning.
 *
 * @param burrow Burrow object
 * @param account Account name
 * @param filters Filters to apply (may be NULL)
 */
void burrow_get_queues(burrow_st *burrow,
                       char *account, burrow_filters_st *filters);

/**
 * Sets up burrow to issue a delete_queues command.
 *
 * If burrow is already issuing a command, this will fail and trigger
 * a warning.
 *
 * @param burrow Burrow object
 * @param account Account name
 * @param filters Filters to apply (may be NULL)
 */
void burrow_delete_queues(burrow_st *burrow,
                          char *account, burrow_filters_st *filters);

/**
 * Sets up burrow to issue a get_accounts command.
 *
 * If burrow is already issuing a command, this will fail and trigger
 * a warning.
 *
 * @param burrow Burrow object
 * @param filters Filters to apply (may be NULL)
 */
void burrow_get_accounts(burrow_st *burrow, burrow_filters_st *filters);

/**
 * Sets up burrow to issue a delete_accounts command.
 *
 * If burrow is already issuing a command, this will fail and trigger
 * a warning.
 *
 * @param burrow Burrow object
 * @param filters Filters to apply (may be NULL)
 */
void burrow_delete_accounts(burrow_st *burrow, burrow_filters_st *filters);

/**
 * Cancels an ongoing command. 
 * Will trigger a command-complete callback if any command is pending.
 * TODO: Should trigger a command complete call always?
 *
 * @param burrow Burrow object
 */
void burrow_cancel(burrow_st *burrow);

/**
 * Begins or continues processing of the current burrow command.
 * If there is no command, returns immediately. If a command completes
 * inside of burrow_process, the command-complete callback will be
 * invoked, after which point burrow will see if any new command has
 * been issues which it can continue processing.
 *
 * @param burrow Burrow object
 */
void burrow_process(burrow_st *burrow);

/**
 * Notify burrow that an event has occurred on a given file descriptor.
 * Behavior is undefined if this event was not requested to be waited
 * upon by the event-wait callback above.
 *
 * @param burrow Burrow object
 * @param fd Which file descriptor
 * @param event Which event(s) occurred
 */
void burrow_event_raised(burrow_st *burrow, burrow_fd_t fd, burrow_event_t event);

