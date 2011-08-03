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

/**
 * Internal logging routing function. If the user has set a logging function,
 * that function will be called with an internally-buffered error. Otherwise,
 * errors will be written to stdout. Does not validate verbose level.
 *
 * @param burrow Burrow object
 * @param verbose Error level of this message
 * @param msg Message/printf-style format string of logged message
 * @param event Arguments for printf in va_list form
 */
BURROW_LOCAL
void burrow_internal_log(burrow_st *burrow, burrow_verbose_t verbose, const char *msg, va_list args);

/**
 * Default watch-fd function utilized if the user hasn't set watch_fd_fn.
 * Adds fds and events to an internal fd/event list for later use.
 *
 * @param burrow Burrow object
 * @param fd Which FD to watch
 * @param events Which events to watch for
 * @return 0 on success, ENOMEM on memory error
 */
BURROW_LOCAL
int burrow_internal_watch_fd(burrow_st *burrow, int fd, burrow_ioevent_t events);

/**
 * Kicks off an blocking for any fds currently being watched internal
 * to the burrow structure.
 *
 * @param burrow Burrow object
 * @return 0 on success, appropriate errno on error
 */
BURROW_LOCAL
int burrow_internal_poll_fds(burrow_st *burrow);

#endif /* __BURROW_INTERNAL_H */