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


/*
 * libburrow -- burrow Client Library
 *
 * @brief this ridiculous little collection of functions for user_buffers
 * seems to be needed to deal with libcurl.  At least, I haven't found a
 * better way of dealing with it.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in this directory for full text.
 */

typedef struct user_buffer_st user_buffer;

user_buffer *user_buffer_create_sized(user_buffer *buffer, const uint8_t *data, size_t data_size);

user_buffer *user_buffer_create(user_buffer *buffer, const uint8_t *data);

void user_buffer_destroy(struct user_buffer_st *buffer);

size_t user_buffer_curl_write_function(char *data, size_t size, size_t nmemb, void *userdata);

size_t user_buffer_curl_read_function(char *data, size_t size, size_t nmemb, void *userdata);

size_t user_buffer_curl_read_nothing_function(char *data, size_t size, size_t nmemb, void *userdata);

char *user_buffer_get_text(const user_buffer *buffer);

size_t user_buffer_get_size(const user_buffer *buffer);
