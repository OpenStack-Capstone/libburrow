/*
 * libburrow -- Burrow Client Library
 *
 * Copyright (C) 2011 Adrian Miranda (ade@psg.com)
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in this directory for full text.
 */

/**
 * @file
 * @brief Http backend using libcurl
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
