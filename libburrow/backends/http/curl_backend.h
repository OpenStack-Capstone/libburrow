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
 * @brief declarations for Http backend using libcurl
 */
#ifndef __BURROW_BACKEND_CURL_H
#define __BURROW_BACKEND_CURL_H

#include <curl/curl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern burrow_backend_functions_st burrow_backend_http_functions;

typedef struct burrow_backend_st burrow_backend_t;

burrow_command_t  burrow_backend_http_get_command(burrow_backend_t *backend);

burrow_st *burrow_backend_http_get_burrow(burrow_backend_t *backend);

CURL *burrow_backend_http_get_curl_easy_handle(burrow_backend_t *backend);

#ifdef __cplusplus
}
#endif
#endif /* __BURROW_BACKEND_CURL_H */
