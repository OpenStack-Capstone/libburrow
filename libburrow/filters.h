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
 * @brief Filters functions
 */
#ifndef __BURROW_FILTERS_H
#define __BURROW_FILTERS_H

#ifdef __cplusplus
extern "C" {
#endif

burrow_filters_st *burrow_filters_create(burrow_filters_st *dest);
size_t burrow_filters_size();
burrow_filters_st *burrow_filters_clone(burrow_filters_st *dest, const burrow_filters_st *src);
void burrow_filters_free(burrow_filters_st *filters);

void burrow_filters_set_match_hidden(burrow_filters_st *filters, bool match_hidden);
void burrow_filters_set_limit(burrow_filters_st *filters, uint32_t limit);
void burrow_filters_set_marker(burrow_filters_st *filters, const char *marker);
void burrow_filters_set_detail(burrow_filters_st *filters, burrow_detail_t detail);

#ifdef __cplusplus
}
#endif

#endif /* __BURROW_FILTERS_H */