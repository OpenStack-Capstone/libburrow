/*
 * libburrow -- burrow Client Library
 *
 * this ridiculous little collection of functions for user_buffers
 * seems to be needed to deal with libcurl.  At least, I haven't found a
 * better way of dealing with it.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in this directory for full text.
 */

typedef struct user_buffer_st user_buffer;

user_buffer *user_buffer_create(user_buffer *buffer, const uint8_t *data);

void user_buffer_destroy(struct user_buffer_st *buffer);

size_t user_buffer_curl_write_function(char *data, size_t size, size_t nmemb, void *userdata);

size_t user_buffer_curl_read_function(char *data, size_t size, size_t nmemb, void *userdata);

size_t user_buffer_curl_read_nothing_function(char *data, size_t size, size_t nmemb, void *userdata);

char *user_buffer_get_text(const user_buffer *buffer);

size_t user_buffer_get_size(const user_buffer *buffer);
