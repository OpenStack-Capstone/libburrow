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

#include "common.h"

burrow_filters_st *burrow_filters_create(burrow_filters_st *dest, burrow_st *burrow)
{
  if (!burrow || dest) {
    if (!dest) {
      dest = malloc(sizeof(burrow_filters_st));
      if (!dest)
        return NULL;
    }

    dest->burrow = burrow; /* for the free_fn, if burrow is defined */
    dest->next = dest; /* convention for non-managed */
    dest->prev = dest;
    
  } else if (!dest) {
    dest = burrow->malloc_fn(burrow, sizeof(burrow_filters_st));
    if (!dest)
      return NULL;

    dest->burrow = burrow;
    dest->prev = NULL;
    dest->next = burrow->filters_list;
    burrow->filters_list = dest;

    if (dest->next != NULL)
      dest->next->prev = dest;
  }
  
  dest->wait = -1;
  dest->limit = -1;
  dest->marker = NULL;
  dest->detail = BURROW_DETAIL_UNSET;
  dest->match_hidden = BURROW_MAYBE;
  
  return dest;
}

size_t burrow_filters_size(void)
{
  return sizeof(burrow_filters_st);
}

burrow_filters_st *burrow_filters_clone(burrow_filters_st *dest, const burrow_filters_st *src)
{
  dest = burrow_filters_create(dest, src->burrow);
  if (!dest)
    return NULL;
  /* TODO: if copy strings is implemented, we need to copy the marker */
  dest->wait = src->wait;
  dest->limit = src->limit;
  dest->marker = src->marker;
  dest->detail = src->detail;
  dest->match_hidden = src->match_hidden;
  return dest;
}

void burrow_filters_free(burrow_filters_st *filters)
{
  /* TODO: if copy strings is implemented, we need to free marker */
  if (filters->burrow != NULL) { 
    if (filters->next != filters) { /* a managed attribute object */

      if (filters->next != NULL) /* not at end */
        filters->next->prev = filters->prev;
      if (filters->prev != NULL) /* not at front */
        filters->prev->next = filters->next;
      else /* at front -- update burrow */
        filters->burrow->filters_list = filters->next;

    }
    filters->burrow->free_fn(filters->burrow, filters);
  } else {
    free(filters);
  }
}

void burrow_filters_set_match_hidden(burrow_filters_st *filters, burrow_tribool_t match_hidden)
{
  filters->match_hidden = match_hidden;
}

void burrow_filters_set_marker(burrow_filters_st *filters, const char *marker_id)
{
  /* TODO: if copy strings is implemented, we need to copy the user input */
  filters->marker = marker_id;
}

void burrow_filters_set_limit(burrow_filters_st *filters, int32_t limit)
{
  if (limit < 0)
    limit = -1;
  filters->limit = limit;
}

void burrow_filters_set_wait(burrow_filters_st *filters, int32_t wait_time)
{
  if (wait_time < 0)
    wait_time = -1;
  filters->wait = wait_time;
}


void burrow_filters_set_detail(burrow_filters_st *filters, burrow_detail_t detail)
{
  filters->detail = detail;
}


burrow_tribool_t burrow_filters_get_match_hidden(burrow_filters_st *filters)
{
  return filters->match_hidden;
}

const char *burrow_filters_get_marker(burrow_filters_st *filters)
{
  return filters->marker;
}

int32_t burrow_filters_get_limit(burrow_filters_st *filters)
{
  return filters->limit;
}

int32_t burrow_filters_get_wait(burrow_filters_st *filters)
{
  return filters->wait;
}

burrow_detail_t burrow_filters_get_detail(burrow_filters_st *filters)
{
  return filters->detail;
}
