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
/**
 * @file
 * @brief this ridiculous little collection of functions for user_buffers
 * seems to be needed to deal with libcurl.  At least, I haven't found a
 * better way of dealing with it.
 *

#include <sys/types.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct user_buffer_st {
  size_t size;
  size_t where;
  char *buf;
};


#include "user_buffer.h"

user_buffer *
user_buffer_create(user_buffer *buffer, const uint8_t *data) {
  return user_buffer_create_sized(buffer, data,
				  data ? strlen((const char *)data) : 0);
}

user_buffer *
user_buffer_create_sized(user_buffer *buffer, const uint8_t *data, size_t data_size){
  if (buffer == 0) {
    buffer = (user_buffer *)malloc(sizeof(user_buffer));
  }
  buffer->where = 0;
  if (data == 0) {
    buffer->buf = 0;
    buffer->size = 0;
  } else {
    buffer->buf = malloc(data_size);
    memcpy(buffer->buf, data, data_size);
    buffer->size = data_size;
  }
  return buffer;
}  

void
user_buffer_destroy(user_buffer *buffer) {
  if (buffer->buf != 0)
    free(buffer->buf);
  free(buffer);
}

char *
user_buffer_get_text(const user_buffer *buffer) {
  return buffer->buf;
}

size_t
user_buffer_get_size(const user_buffer *buffer){
  return buffer->size;
}

/**
 * when libcurl receives data it will call this function to write
 * it somewhere (a buffer).
 */
size_t
user_buffer_curl_write_function(char *data, size_t size, size_t nmemb, void *userdata)
{
  user_buffer *userd = (user_buffer *)userdata;
  size_t len = size * nmemb;
  if (userd->size != 0) {
    if (userd->buf != 0) {
      userd->buf = (char *)realloc(userd->buf, userd->size + len);
      userd->size += len;
    }
  } else {
    userd->size = len;
    userd->buf = malloc(len);
    userd->where = 0;
  }
  memcpy(userd->buf + userd->where, data, len);
  userd->where += len;
  return len;
}

/**
 * when libcurl needs to read some data to send, it will call this function
 * to read the data.
 */
size_t
user_buffer_curl_read_function(char *data, size_t size, size_t nmemb, void *userdata)
{
  user_buffer *userd = (user_buffer *)userdata;
  size_t len = size * nmemb;

  size_t how_much_left = userd->size - userd->where;
  if (how_much_left == 0)
    return 0;

  if (how_much_left <= len){
    memcpy(data, userd->buf + userd->where, how_much_left);
    userd->where += how_much_left;
    return how_much_left;
  } else {
    memcpy(data, userd->buf + userd->where, len);
    userd->where += len;
    return len;
  }
}

size_t
user_buffer_curl_read_nothing_function(char *data, size_t size, size_t nmemb,
				       void *userdata)
{
  (void)data;
  (void)size;
  (void)nmemb;
  (void)userdata;
  return 0;
}

