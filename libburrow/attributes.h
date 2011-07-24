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

/**
 * Creates an attributes structure. Three cases:
 *   destination is non-NULL: the destination will be initialized
 *     note: DO NOT use this to re-initialize an previously initialized
 *     attributes structure!
 *   destination is NULL, burrow is NULL: a new attributes structure will
 *     be malloced and initialized
 *   destination is NULL, burrow is non-NULL: a new attributes structure
 *     will be allocated (using burrow's malloc), initialized, and
 *     will be treated as managed by burrow.
 *
 * @param dest destination structure, or NULL
 * @param burrow associated burrow structure, or NULL
 * @return pointer to the initialzied attributes structure, or NULL on error
 */
BURROW_API
burrow_attributes_st *burrow_attributes_create(burrow_attributes_st *dest, burrow_st *burrow);

/**
 * Returns the size of an attributes structure.
 *
 * @return size of a attributes structure.
 */
BURROW_API
size_t burrow_attributes_size(void);

/**
 * Clones the source attributes structure into the destination. Follows
 * similar conventions to create:
 *   if destination is non-NULL, then the source will merely be copied to
 *     the destination
 *   if the destination is NULL and the source is associated with a burrow
 *     struct (a burrow structure was supplied during its creation process)
 *     then the destination will be created in the same manner as the source
 *   if the destination is NULL and the source has no associated burrow
 *     struct, then the destination will merely be malloc'd 
 *
 * @param dest destination struct, may be NULL
 * @param src source struct
 * @return destination struct
 */
BURROW_API
burrow_attributes_st *burrow_attributes_clone(burrow_attributes_st *dest, const burrow_attributes_st *src);

/**
 * Destroys/frees a given attributes structure, properly unassociating it from
 * any burrow struct and freeing
 *
 * @param attributes attributes structure to free
 */
BURROW_API
void burrow_attributes_destroy(burrow_attributes_st *attributes);

/**
 * "Unsets" fields in the provided attributes struct.
 * See burrow_attributes_set_t typedef.
 *
 * @param filters attributes structure
 * @param unset bitfield of which attributes to unset
 */
BURROW_API
void burrow_attributes_unset(burrow_attributes_st *attributes, burrow_attributes_set_t set);

/**
 * Checks if certain fields have been set in the attributes structure. Calls
 * to burrow_attributes_get functions may return erroneous/uninitialized values
 * otherwise, and so should be checked first.
 *
 * @param attributes attributes struct
 * @param set bitfield of which attributes to check
 * @return true if any of the specified attributes are set, false otherwise
 */
BURROW_API
bool burrow_attributes_check(const burrow_attributes_st *attributes, burrow_attributes_set_t set);

/**
 * Sets the ttl attribute value.
 *
 * @param filters attributes struct
 * @param ttl ttl value
 */
BURROW_API
void burrow_attributes_set_ttl(burrow_attributes_st *attributes, uint32_t ttl);

/**
 * Sets the hide attribute value.
 *
 * @param filters attributes struct
 * @param hide hide value
 */
BURROW_API
void burrow_attributes_set_hide(burrow_attributes_st *attributes, uint32_t hide);

/**
 * Gets the ttl value. See burrow_attributes_check for more information
 * on retrieving attributes values.
 *
 * @param attributes attributes struct
 * @return the value of the ttl attribute
 */
BURROW_API
uint32_t burrow_attributes_get_ttl(const burrow_attributes_st *attributes);

/**
 * Gets the hide value. See burrow_attributes_check for more information
 * on retrieving attributes values.
 *
 * @param attributes attributes struct
 * @return the value of the hide attribute
 */
BURROW_API
uint32_t burrow_attributes_get_hide(const burrow_attributes_st *attributes);

#ifdef __cplusplus
}
#endif

#endif /* __BURROW_ATTRIBUTES_H */
