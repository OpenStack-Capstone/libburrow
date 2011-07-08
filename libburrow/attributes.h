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
#ifndef __BURROW_ATTRIBUTES_H
#define __BURROW_ATTRIBUTES_H

#ifdef __cplusplus
extern "C" {
#endif

/* TODO: doxygen this sucker! */

/* TODO: should these be reversed? burrow first? */
BURROW_API
burrow_attributes_st *burrow_attributes_create(burrow_attributes_st *dest, burrow_st *burrow);

BURROW_API
size_t burrow_attributes_size(void);

BURROW_API
burrow_attributes_st *burrow_attributes_clone(burrow_attributes_st *dest, const burrow_attributes_st *src);

BURROW_API
void burrow_attributes_free(burrow_attributes_st *attributes);

BURROW_API
void burrow_attributes_set_ttl(burrow_attributes_st *attributes, int32_t ttl);

BURROW_API
void burrow_attributes_set_hide(burrow_attributes_st *attributes, int32_t hide);

BURROW_API
int32_t burrow_attributes_get_ttl(const burrow_attributes_st *attributes);

BURROW_API
int32_t burrow_attributes_get_hide(const burrow_attributes_st *attributes);

#ifdef __cplusplus
}
#endif

#endif /* __BURROW_ATTRIBUTES_H */