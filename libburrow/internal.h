/*
 * libburrow -- Burrow Client Library
 *
 * Copyright (C) 2011 Tony Wooster (twooster@gmail.com)
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in this directory for full text.
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