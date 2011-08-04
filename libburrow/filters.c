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
 * @brief Filters function definitions
 */

#include "common.h"

burrow_filters_st *burrow_filters_create(burrow_filters_st *dest,
                                         burrow_st *burrow)
{
  if (!burrow || dest)
  {
    if (!dest)
    {
      dest = malloc(sizeof(burrow_filters_st));
      if (!dest)
        return NULL;
    }

    dest->burrow = burrow; /* for the free_fn, if burrow is defined */
    dest->next = dest; /* convention for non-managed */
    dest->prev = dest;
  }
  else if (!dest)
  {
    dest = burrow_malloc(burrow, sizeof(burrow_filters_st));
    if (!dest)
      return NULL;

    dest->burrow = burrow;
    dest->prev = NULL;
    dest->next = burrow->filters_list;
    burrow->filters_list = dest;

    if (dest->next != NULL)
      dest->next->prev = dest;
  }
  
  dest->wait = 0;
  dest->limit = 0;
  dest->marker = NULL;
  dest->detail = BURROW_DETAIL_NONE;
  dest->match_hidden = false;
  dest->set = BURROW_FILTERS_NONE;
  
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
  dest->set = src->set;
  return dest;
}

void burrow_filters_destroy(burrow_filters_st *filters)
{
  /* TODO: if copy strings is implemented, we need to free marker */
  if (filters->burrow != NULL)
  { 
    if (filters->next != filters) /* a managed attribute object */
    { 
      if (filters->next != NULL) /* not at end */
        filters->next->prev = filters->prev;
      if (filters->prev != NULL) /* not at front */
        filters->prev->next = filters->next;
      else /* at front -- update burrow */
        filters->burrow->filters_list = filters->next;
    }
    burrow_free(filters->burrow, filters);
  }
  else
    free(filters);
}

void burrow_filters_unset_all(burrow_filters_st *filters)
{
  /* TODO: if copystrings, then we need to dealloc our marker */
  filters->set = BURROW_FILTERS_NONE;
  filters->marker = NULL;
}



void burrow_filters_set_match_hidden(burrow_filters_st *filters, bool match_hidden)
{
  filters->match_hidden = match_hidden;
  filters->set |= BURROW_FILTERS_MATCH_HIDDEN;
}

bool burrow_filters_get_match_hidden(const burrow_filters_st *filters)
{
  return filters->match_hidden;
}

bool burrow_filters_isset_match_hidden(const burrow_filters_st *filters)
{
  return (filters->set & BURROW_FILTERS_MATCH_HIDDEN);
}

void burrow_filters_unset_match_hidden(burrow_filters_st *filters)
{
  filters->set &= ~BURROW_FILTERS_MATCH_HIDDEN;
}



void burrow_filters_set_limit(burrow_filters_st *filters, uint32_t limit)
{
  filters->limit = limit;
  filters->set |= BURROW_FILTERS_LIMIT;
}

uint32_t burrow_filters_get_limit(const burrow_filters_st *filters)
{
  return filters->limit;
}

bool burrow_filters_isset_limit(const burrow_filters_st *filters)
{
  return (filters->set & BURROW_FILTERS_LIMIT);
}

void burrow_filters_unset_limit(burrow_filters_st *filters)
{
  filters->set &= ~BURROW_FILTERS_LIMIT;
}



void burrow_filters_set_wait(burrow_filters_st *filters, uint32_t wait_time)
{
  filters->wait = wait_time;
  filters->set |= BURROW_FILTERS_WAIT;
}

uint32_t burrow_filters_get_wait(const burrow_filters_st *filters)
{
  return filters->wait;
}

bool burrow_filters_isset_wait(const burrow_filters_st *filters)
{
  return (filters->set & BURROW_FILTERS_WAIT);
}

void burrow_filters_unset_wait(burrow_filters_st *filters)
{
  filters->set &= ~BURROW_FILTERS_WAIT;
}



void burrow_filters_set_detail(burrow_filters_st *filters, burrow_detail_t detail)
{
  filters->detail = detail;
  filters->set |= BURROW_FILTERS_DETAIL;
}

burrow_detail_t burrow_filters_get_detail(const burrow_filters_st *filters)
{
  return filters->detail;
}

bool burrow_filters_isset_detail(const burrow_filters_st *filters)
{
  return (filters->set & BURROW_FILTERS_DETAIL);
}

void burrow_filters_unset_detail(burrow_filters_st *filters)
{
  filters->set &= ~BURROW_FILTERS_DETAIL;
}



void burrow_filters_set_marker(burrow_filters_st *filters, const char *marker_id)
{
  /* TODO: if copy strings is implemented, we need to copy the user input */
  /* Special case: if the user passes in NULL, we treat it as an unset command */
  filters->marker = marker_id;
}

const char *burrow_filters_get_marker(const burrow_filters_st *filters)
{
  return filters->marker;
}
