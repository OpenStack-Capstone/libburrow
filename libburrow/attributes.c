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
 * @brief Attribute function declarations
 */

#include "common.h"

burrow_attributes_st *burrow_attributes_create(burrow_attributes_st *dest, burrow_st *burrow)
{
  if (!burrow || dest) {
    if (!dest) {
      dest = malloc(sizeof(burrow_attributes_st));
      if (!dest)
        return NULL;
    }

    dest->burrow = burrow; /* for the free_fn, if burrow is defined */
    dest->next = dest; /* convention for non-managed */
    dest->prev = dest;
    
  } else if (!dest) {
    dest = burrow->malloc_fn(burrow, sizeof(burrow_attributes_st));
    if (!dest)
      return NULL;

    dest->burrow = burrow;
    dest->prev = NULL;
    if (burrow->attributes_list != NULL) {
      burrow->attributes_list->prev = dest;
      dest->next = burrow->attributes_list;
    } else
      dest->next = NULL;

    burrow->attributes_list = dest;
  }
  
  dest->ttl = -1;
  dest->hide = -1;
  
  return dest;
}

size_t burrow_attributes_size(void)
{
  return sizeof(burrow_attributes_st);
}

burrow_attributes_st *burrow_attributes_clone(burrow_attributes_st *dest, const burrow_attributes_st *src)
{
  burrow_attributes_create(dest, src->burrow);
  dest->ttl = src->ttl;
  dest->hide = src->hide;
  return dest;
}

void burrow_attributes_free(burrow_attributes_st *attributes)
{
  if (attributes->burrow != NULL) { 
    if (attributes->next != attributes) { /* a managed attribute object */
      if (attributes->prev != NULL) /* not at front */
        attributes->prev->next = attributes->next;
      if (attributes->next != NULL) /* not at end */
        attributes->next->prev = attributes->prev;

      if (attributes->next == NULL &&
          attributes->prev == NULL)
        attributes->burrow->attributes_list = NULL;
      else
        attributes->burrow->attributes_list = attributes->next;
    }
    attributes->burrow->free_fn(attributes->burrow, attributes);
  } else {
    free(attributes);
  }
}

void burrow_attributes_set_ttl(burrow_attributes_st *attributes, int32_t ttl)
{
  attributes->ttl = ttl;
}

void burrow_attributes_set_hide(burrow_attributes_st *attributes, int32_t hide)
{
  attributes->hide = hide;
}

int32_t burrow_attributes_get_ttl(burrow_attributes_st *attributes)
{
  return attributes->ttl;
}

int32_t burrow_attributes_get_hide(burrow_attributes_st *attributes)
{
  return attributes->hide;
}
