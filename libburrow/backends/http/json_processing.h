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
 * @file json_processing.h
 * @brief Declarations of functions to assist with JSON processing.
 */

typedef struct json_processing_st json_processing_t;

int burrow_backend_http_parse_json(burrow_backend_t *backend,
				   char *jsontext,
				   size_t jsonsize);
