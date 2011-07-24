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

/**
 * Creates a filters structure. Three cases:
 *   destination is non-NULL: the destination will be initialized
 *   destination is NULL, burrow is NULL: a new filters structure will
 *     be malloced and initialized
 *   destination is NULL, burrow is non-NULL: a new filters structure
 *     will be allocated (using burrow's malloc), initialized, and
 *     will be treated as managed by burrow. See burrow_filters_destroy()
 *
 * @param dest Destination structure, or NULL
 * @param burrow Associated burrow structure, or NULL
 * @return A pointer to the created filters structure, or NULL on error
 */
BURROW_API
burrow_filters_st *burrow_filters_create(burrow_filters_st *dest, burrow_st *burrow);

/**
 * Returns the size of a filters structure.
 *
 * @return The size of a filters structure.
 */
BURROW_API
size_t burrow_filters_size(void);

/**
 * Clones the source filters structure into the destination. Follows
 * similar conventions to create:
 *   if destination is non-NULL, then the source will merely be copied to
 *     the destination
 *   if the destination is NULL and the source is associated with a burrow
 *     object (a burrow structure was supplied during its creation process)
 *     then the destination will be created in the same manner as the source
 *   if the destination is NULL and the source has no associated burrow
 *     object, then the destination will merely be malloc'd 
 *
 * @param dest The destination object, may be NULL
 * @param src The source object
 * @return The destination object
 */
BURROW_API
burrow_filters_st *burrow_filters_clone(burrow_filters_st *dest, const burrow_filters_st *src);

/**
 * Destroys/frees a given filter object, properly unassociating it from any
 * burrow object and freeing
 *
 * @param filters Filters object to free
 */
BURROW_API
void burrow_filters_destroy(burrow_filters_st *filters);

/**
 * "Unsets" fields in the provided filters object. See burrow_filters_set_t typedef.
 *
 * @param filters Filters object
 * @param unset Which filters to unset
 */
BURROW_API
void burrow_filters_unset(burrow_filters_st *filters, burrow_filters_set_t unset);

/**
 * Checks if certain fields have been set in the filters structure. Calls
 * to burrow_filters_get functions may return erroneous/uninitialized values
 * otherwise, and so should be checked first.
 *
 * @param filters Filters object
 * @param set Which filters to check
 * @return true if any of the specified filters are set, false otherwise
 */
BURROW_API
bool burrow_filters_check(const burrow_filters_st *filters, burrow_filters_set_t set);

/**
 * Sets the match_hidden filter value.
 *
 * @param filters filters object
 * @param match_hidden boolean value
 */
BURROW_API
void burrow_filters_set_match_hidden(burrow_filters_st *filters, bool match_hidden);

/**
 * Sets the marker filter. Note: If marker_id is NULL, then calling this function
 * is the same as unsetting the marker.
 *
 * @param filters filters object
 * @param marker_id null-terminated marker-id
 */
BURROW_API
void burrow_filters_set_marker(burrow_filters_st *filters, const char *marker_id);

/**
 * Sets the limit filter.
 *
 * @param filters filters object
 * @param limit limit value
 */
BURROW_API
void burrow_filters_set_limit(burrow_filters_st *filters, uint32_t limit);

/**
 * Sets the wait filter.
 *
 * @param filters filters object
 * @param wait wait value
 */
BURROW_API
void burrow_filters_set_wait(burrow_filters_st *filters, uint32_t wait_time);

/**
 * Sets the detail filter.
 *
 * @param filters filters object
 * @param detail detail level
 */
BURROW_API
void burrow_filters_set_detail(burrow_filters_st *filters, burrow_detail_t detail);

/**
 * Gets the match_hidden filter value. See burrow_filters_check for more
 * information on retrieving filters values.
 *
 * @param filters filters object
 * @return the value of the match_hidden filter
 */
BURROW_API
bool burrow_filters_get_match_hidden(const burrow_filters_st *filters);

/**
 * Gets the marker filter value. See burrow_filters_check for more information
 * on retrieving filters values.
 *
 * @param filters filters object
 * @return the value of the marker filter
 */
BURROW_API
const char *burrow_filters_get_marker(const burrow_filters_st *filters);

/**
 * Gets the limit filter value. See burrow_filters_check for more information
 * on retrieving filters values.
 *
 * @param filters filters object
 * @return the value of the limit filter
 */
BURROW_API
uint32_t burrow_filters_get_limit(const burrow_filters_st *filters);

/**
 * Gets the wait filter value. See burrow_filters_check for more information
 * on retrieving filters values.
 *
 * @param filters filters object
 * @return the value of the wait filter
 */
BURROW_API
uint32_t burrow_filters_get_wait(const burrow_filters_st *filters);

/**
 * Gets the detail filter value. See burrow_filters_check for more information
 * on retrieving filters values.
 *
 * @param filters filters object
 * @return the value of the detail filter
 */
BURROW_API
burrow_detail_t burrow_filters_get_detail(const burrow_filters_st *filters);


#ifdef __cplusplus
}
#endif

#endif /* __BURROW_FILTERS_H */