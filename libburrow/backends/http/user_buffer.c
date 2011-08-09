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
 * @brief this silly collection of functions for user_buffers
 * seems to be needed to deal with libcurl.  At least, I haven't found a
 * better way of dealing with it.
 *
 */
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

/**
 * Creates a user_buffer, optionally containing data
 *
 * @param buffer pointer to a previously allocated buffer, if null a
 * new one will be allocated.
 * @param data pointer to character data.  If null, an empty buffer
 * will be created
 * @return a pointer to user_buffer.  If NULL, indicates a malloc failure.
 */
user_buffer *
user_buffer_create(user_buffer *buffer, const uint8_t *data) {
  return user_buffer_create_sized(buffer, data,
				  data ? strlen((const char *)data) : 0);
}

/**
 * Creates a user_buffer, optionally containing data, with a size given
 *
 * @param buffer pointer to a previously allocated buffer, if null a
 * new one will be allocated.
 * @param data pointer to character data.  If null, an empty buffer
 * will be created
 * @param data_size size in bytes of string pointed to by data
 * @return a pointer to user_buffer.  If NULL, indicates a malloc failure.
 */
user_buffer *
user_buffer_create_sized(user_buffer *buffer, const uint8_t *data, size_t data_size){
  if (buffer == 0) {
    buffer = (user_buffer *)malloc(sizeof(user_buffer));
    if (buffer == NULL)
      return 0;
  }
  buffer->where = 0;
  if (data == 0) {
    buffer->buf = 0;
    buffer->size = 0;
  } else {
    buffer->buf = malloc(data_size);
    if (buffer->buf == NULL){
      free(buffer);
      return 0;
    }
    memcpy(buffer->buf, data, data_size);
    buffer->size = data_size;
  }
  return buffer;
}  

/**
 * Destroys a previously allocated user_buffer.
 *
 * @param buffer pointer to a previously allocated user_buffer
 */
void
user_buffer_destroy(user_buffer *buffer) {
  if (buffer->buf != 0)
    free(buffer->buf);
  free(buffer);
}

/**
 * Returns a character pointer to the data stored in the user_buffer
 *
 * @param buffer pointer to a user_buffer
 * @return pointer to a char* containing the data within the user_buffer.
 */
char *
user_buffer_get_text(const user_buffer *buffer) {
  return buffer->buf;
}

/**
 * Returns the amount of data within a user_buffer
 *
 * @param buffer pointer to a user_buffer
 * @return size (size_t) of the data stored within the user_buffer
 */
size_t
user_buffer_get_size(const user_buffer *buffer){
  return buffer->size;
}

/**
 * when libcurl receives data it will call this function to write
 * it somewhere (a user_buffer).
 *
 * @param data where the new data libcurl has read is presently stored
 * @param size the size of the data libcurl has read
 * @param nmemb the size of each member that libcurl has read
 * @param userdata pointer to a user_buffer, in which we will write the new data
 * @return size of the data read.  Should be the size offered to us.
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
 *
 * @param data pointer to where we should write the data
 * @param size number of items of storage available for us to write data
 * @param nmemb size of each item.  (Thus size*nmemb = total size)
 * @param userdata pointer to a user_buffer containing whatever we want libcurl to send
 * @return number of bytes copied to data, for libcurl to send
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

/**
 * test function for reading nothing.  If libcurl calls this, it will always
 * be told nothing is available.
 *
 */
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

