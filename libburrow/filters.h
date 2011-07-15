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
 * @brief Filters function declarations
 */
#ifndef __BURROW_FILTERS_H
#define __BURROW_FILTERS_H

#ifdef __cplusplus
extern "C" {
#endif

/* TODO: doxygen this sucker! */

/* TODO: should these be reversed? burrow first? */
BURROW_API
burrow_filters_st *burrow_filters_create(burrow_filters_st *dest, burrow_st *burrow);

BURROW_API
size_t burrow_filters_size(void);

BURROW_API
burrow_filters_st *burrow_filters_clone(burrow_filters_st *dest, const burrow_filters_st *src);

BURROW_API
void burrow_filters_free(burrow_filters_st *filters);

BURROW_API
void burrow_filters_unset(burrow_filters_st *filters, burrow_filters_set_t set);

BURROW_API
bool burrow_filters_check(burrow_filters_st *filters, burrow_filters_set_t set);

BURROW_API
void burrow_filters_set_match_hidden(burrow_filters_st *filters, bool match_hidden);

BURROW_API
void burrow_filters_set_marker(burrow_filters_st *filters, const char *marker_id);

BURROW_API
void burrow_filters_set_limit(burrow_filters_st *filters, uint32_t limit);

BURROW_API
void burrow_filters_set_wait(burrow_filters_st *filters, uint32_t wait_time);

BURROW_API
void burrow_filters_set_detail(burrow_filters_st *filters, burrow_detail_t detail);

BURROW_API
bool burrow_filters_get_match_hidden(burrow_filters_st *filters);

BURROW_API
const char *burrow_filters_get_marker(burrow_filters_st *filters);

BURROW_API
uint32_t burrow_filters_get_limit(burrow_filters_st *filters);

BURROW_API
uint32_t burrow_filters_get_wait(burrow_filters_st *filters);

BURROW_API
burrow_detail_t burrow_filters_get_detail(burrow_filters_st *filters);


#ifdef __cplusplus
}
#endif

#endif /* __BURROW_FILTERS_H */