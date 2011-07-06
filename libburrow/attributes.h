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
 * @brief Attributes functions
 */
#ifndef __BURROW_ATTRIBUTES_H
#define __BURROW_ATTRIBUTES_H

#ifdef __cplusplus
extern "C" {
#endif

burrow_attributes_st *burrow_attributes_create(burrow_attributes_st *dest);
size_t burrow_attributes_size();
burrow_attributes_st *burrow_attributes_clone(burrow_attributes_st *dest, const burrow_attributes_st *src);
void burrow_attributes_free(burrow_attributes_st *attributes);

void burrow_attributes_set_ttl(burrow_attributes_st *attributes, int32_t ttl);
void burrow_attributes_set_hide(burrow_attributes_st *attributes, int32_t hide);

int32_t burrow_attributes_get_ttl(burrow_attributes_st *attributes);
int32_t burrow_attributes_get_hide(burrow_attributes_st *attributes);

#ifdef __cplusplus
}
#endif

#endif /* __BURROW_ATTRIBUTES_H */