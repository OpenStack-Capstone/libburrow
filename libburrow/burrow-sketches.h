/* For my own notes, function pointers!
 typedef return_type (typedef_name_w_optional_asterisk_for_pointer_only)(param_type param_name, param_type param_name2);
*/

/* One way of doing things:

typedef void (burrow_get_accounts_fn)(void *backend, burrow_filters_st *filters);
typedef void (burrow_delete_accounts_fn)(void *backend, burrow_filters_st *filters);

typedef void (burrow_get_queues_fn)(void *backend, void *backend, char *account, burrow_filters_st *filters);
typedef void (burrow_delete_queues_fn)(void *backend, char *account, burrow_filters_st *filters);

typedef void (burrow_get_messages_fn)(void *backend, char *account, char *queue, burrow_filters_st *filters);
typedef void (burrow_delete_messages_fn)(void *backend, char *account, char *queue, burrow_filters_st *filters);

typedef void (burrow_get_message_fn)(void *backend, char *account, char *queue, char *id);
typedef void (burrow_delete_message_fn)(void *backend, char *account, char *queue, char *id);

typedef void (burrow_create_message_fn)(void *backend, char *account, char *queue, char *id, uint8_t *body, size_t *body_len, burrow_attributes_t *attributes);

typedef void (burrow_update_message_fn)(void *backend, char *account, char *queue, char *id, burrow_attributes_t *attributes);

typedef void (burrow_update_messages_fn)(void *backend, char *account, char *queue, burrow_attributes_t *attributes, burrow_filters_t *filters);

typedef struct {
  burrow_get_accounts_fn    *get_accounts_fn;
  burrow_delete_accounts_fn *delete_accounts_fn;
  burrow_get_queues_fn      *get_queues_fn;
  burrow_delete_queues_fn   *delete_queues_fn;
  burrow_get_messages_fn    *get_messages_fn;
  burrow_delete_messages_fn *delete_messages_fn;
  burrow_get_message_fn     *get_message_fn;
  burrow_delete_message_fn  *delete_message_fn;
  burrow_create_message_fn  *create_message_fn;
  burrow_update_message_fn  *update_message_fn;
  burrow_update_messages_fn *update_messages_fn;
} burrow_backend_st;

*/

/* or */

/*
typedef char *(burrow_backend_get_option_fn)(void *backend, char *option);
typedef void (burrow_backend_set_option_int_fn)(void *backend, char *option, int32_t value);
*/

typedef enum {
  BURROW_EVENT_SUCCESS,
  BURROW_EVENT_TIMEOUT,
  BURROW_EVENT_COMMAND_WOULDBLOCK,
  BURROW_EVENT_COMMAND_COMPLETED,
  /* etc, tbd as programming continues */
}

typedef enum {
  BURROW_VERBOSE_NONE
  BURROW_VERBOSE_FATAL,
  BURROW_VERBOSE_ERROR,
  BURROW_VERBOSE_WARNING,
  BURROW_VERBOSE_INFO,
  BURROW_VERBOSE_DEBUG,
  
  BURROW_VERBOSE_DEFAULT = BURROW_VERBOSE_ERROR
} burrow_verbose_t;


typedef void (burrow_message_fn)(char *message_id,
                                 uint8_t *body,
                                 size_t body_size,
                                 burrow_attributes_st *attributes,
                                 void *context);

/* TODO: size_t necessary? or null terminated array works? */
typedef void (burrow_queues_fn)(char **queues, void *context);

typedef void (burrow_accounts_fn)(char **accounts, void *context);

typedef void (burrow_log_fn)(burrow_verbose_t verbose, burrow_event_t occurred, char *msg, void *context);

typedef void (burrow_complete_fn)(void *context);

typedef void (burrow_event_wait_fn)(burrow_fd_t fd, burrow_ioevent_t event, void *context);

/* I've let these dictate where void *context is -- at the end of the formals
   list. This may or may not be a good idea. */
typedef void *(burrow_malloc_fn)(size_t size, void *context);
typedef void (burrow_free_fn)(void *ptr, void *context);

typedef int burrow_fd_t;

typedef void *(burrow_backend_create_fn)(void *dest, burrow_st *burrow);
typedef void (burrow_backend_free_fn)(void *backend);
typedef size_t (burrow_backend_size_fn)();
typedef void (burrow_backend_clone_fn)(void *dst, void *src);

typedef void (burrow_backend_set_option_fn)(void *backend, char *key, char *value);

typedef void (burrow_command_fn)(void *backend, burrow_command_st *command);

typedef void (burrow_backend_process_fn)(void *backend);
typedef void (burrow_backend_event_raised_fn)(void *backend, burrow_fd_t fd, burrow_ioevent_t event);

/* Backend-public */
typedef struct {
  burrow_backend_create_fn *create;
  burrow_backend_free *free;
  burrow_backend_size *size;
  burrow_backend_clone *clone;

  burrow_backend_set_option_fn *set_option;

  burrow_backend_process_fn *process;
  burrow_backend_event_raised_fn *event_raised;

  burrow_command_fn *get_accounts;
  burrow_command_fn *delete_accounts;
  burrow_command_fn *get_queues;
  burrow_command_fn *delete_queues;
  burrow_command_fn *get_messages;
  burrow_command_fn *delete_messages;
  burrow_command_fn *get_message;
  burrow_command_fn *delete_message;
  burrow_command_fn *create_message;
  burrow_command_fn *update_message;
  burrow_command_fn *update_messages;
} burrow_backend_functions_st; 

/* Backend-public */
typedef enum {
  BURROW_CMD_GET_ACCOUNTS,
  BURROW_CMD_DELETE_ACCOUNTS,

  BURROW_CMD_GET_QUEUES,
  BURROW_CMD_DELETE_QUEUES,

  BURROW_CMD_GET_MESSAGE,
  BURROW_CMD_UPDATE_MESSAGE,
  BURROW_CMD_DELETE_MESSAGE,

  BURROW_CMD_GET_MESSAGES,
  BURROW_CMD_UPDATE_MESSAGES,
  BURROW_CMD_DELETE_MESSAGES,
  
  BURROW_CMD_MAX,
  BURROW_CMD_NONE = BURROW_CMD_MAX
} burrow_command_t;

typedef enum {
  BURROW_OPT_NONE         = 0,
  BURROW_OPT_AUTOALLOC    = (1 << 0),
  BURROW_OPT_COPY_STRINGS = (1 << 1)
} burrow_options_t;

/* Public */
typedef enum {
  BURROW_DETAIL_UNSET,
  BURROW_DETAIL_NONE,
  BURROW_DETAIL_ID,
  BURROW_DETAIL_ATTRIBUTES,
  BURROW_DETAIL_BODY,
  BURROW_DETAIL_ALL
} burrow_detail_t;

/* Public */
typedef enum {
  BURROW_IOEVENT_NONE    = 0,
  BURROW_IOEVENT_WRITE   = 1,
  BURROW_IOEVENT_READ    = 2,
  BURROW_IOEVENT_TIMEOUT = 3
} burrow_ioevent_t;

/* Necessary? Public */
typedef enum {
  BURROW_MAYBE = -1,
  BURROW_FALSE = 0,
  BURROW_TRUE = 1
} burrow_tribool_t;

/* Existence public, internals not */
typedef struct {
  int32_t ttl; /* -1: not set */
  int32_t hide; /* -1: not set */
} burrow_attributes_st;

/* Existence public, internals not */
typedef struct {
  burrow_tribool_t match_hidden; /* MAYBE -- not set */
  int32_t limit; /* -1, not set */
  char *marker;  /* NULL, not set */
  burrow_detail_t detail; /* BURROW_DETAIL_UNSET -- not set */
  int32_t wait;
} burrow_filters_st;

/* Private */
typedef struct {
  burrow_command_t command;
  char *account;
  char *queue;
  char *message;
  uint8_t *body;
  size_t *body_len;
  burrow_filters_st filters;
  burrow_attributes_st attributes;
} burrow_command_st;

