/**
 * json_processing.h
 * Declarations of functions to assist with JSON processing.
 */

typedef struct json_processing_st json_processing_t;

int burrow_backend_http_parse_json(burrow_backend_t *backend,
				   char *jsontext,
				   size_t jsonsize);
