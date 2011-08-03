/*
 * libburrow -- Burrow Client Library
 *
 * Copyright 2011 Tony Wooster 
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
 * @brief Internal-use functions only
 */


#ifndef __BURROW_INTERNAL_H
#define __BURROW_INTERNAL_H

BURROW_LOCAL
void burrow_internal_log(burrow_st *burrow, burrow_verbose_t verbose, const char *msg, va_list args);

BURROW_LOCAL
void burrow_internal_watch_fd(burrow_st *burrow, int fd, burrow_ioevent_t events);

#endif /* __BURROW_INTERNAL_H */