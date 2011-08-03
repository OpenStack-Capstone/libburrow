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
 *     note: DO NOT use this to re-initialize an previously initialized
 *     filters structure!
 *   destination is NULL, burrow is NULL: a new filters structure will
 *     be malloced and initialized
 *   destination is NULL, burrow is non-NULL: a new filters structure
 *     will be allocated (using burrow's malloc), initialized, and
 *     will be treated as managed by burrow. See burrow_filters_destroy()
 *
 * @param dest destination structure, or NULL
 * @param burrow associated burrow structure, or NULL
 * @return pointer to the created filters structure, or NULL on error
 */
BURROW_API
burrow_filters_st *burrow_filters_create(burrow_filters_st *dest, burrow_st *burrow);

/**
 * Returns the size of a filters structure.
 *
 * @return size of a filters structure.
 */
BURROW_API
size_t burrow_filters_size(void);

/**
 * Clones the source filters structure into the destination. Follows
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
burrow_filters_st *burrow_filters_clone(burrow_filters_st *dest, const burrow_filters_st *src);

/**
 * Destroys/frees a given filter struct, properly unassociating it from any
 * burrow struct and freeing
 *
 * @param filters filters struct to free
 */
BURROW_API
void burrow_filters_destroy(burrow_filters_st *filters);

/**
 * Unsets all fields in the filters structure.
 *
 * @param filters filters structure
 */
BURROW_API
void burrow_filters_unset_all(burrow_filters_st *filters);



/**
 * Sets the match_hidden filter value.
 *
 * @param filters filters struct
 * @param match_hidden boolean value
 */
BURROW_API
void burrow_filters_set_match_hidden(burrow_filters_st *filters, bool match_hidden);

/**
 * Gets the match_hidden filter value. Behavior is undefined if
 * match_hidden is not set.
 *
 * @param filters filters struct
 * @return the value of the match_hidden filter
 */
BURROW_API
bool burrow_filters_get_match_hidden(const burrow_filters_st *filters);

/**
 * Checks if match hidden is set.
 *
 * @param filters filters struct
 * @return true if field is set, false otherwise
 */
BURROW_API
bool burrow_filters_isset_match_hidden(const burrow_filters_st *filters);

/**
 * Unsets the match hidden filter parameter.
 *
 * @param filters filters struct
 */
BURROW_API
void burrow_filters_unset_match_hidden(burrow_filters_st *filters);



/**
 * Sets the limit filter.
 *
 * @param filters filters struct
 * @param limit limit value
 */
BURROW_API
void burrow_filters_set_limit(burrow_filters_st *filters, uint32_t limit);

/**
 * Gets the limit filter value. Behavior is undefined if
 * limit is not set.
 *
 * @param filters filters struct
 * @return the value of the filter
 */
BURROW_API
uint32_t burrow_filters_get_limit(const burrow_filters_st *filters);

/**
 * Checks if limit is set.
 *
 * @param filters filters struct
 * @return true if field is set, false otherwise
 */
BURROW_API
bool burrow_filters_isset_limit(const burrow_filters_st *filters);

/**
 * Unsets the limit filter parameter.
 *
 * @param filters filters struct
 */
BURROW_API
void burrow_filters_unset_limit(burrow_filters_st *filters);



/**
 * Sets the wait filter.
 *
 * @param filters filters struct
 * @param wait wait value
 */
BURROW_API
void burrow_filters_set_wait(burrow_filters_st *filters, uint32_t wait_time);

/**
 * Gets the wait filter value. Behavior is undefined if
 * wait is not set.
 *
 * @param filters filters struct
 * @return the value of the filter
 */
BURROW_API
uint32_t burrow_filters_get_wait(const burrow_filters_st *filters);

/**
 * Checks if wait is set.
 *
 * @param filters filters struct
 * @return true if field is set, false otherwise
 */
BURROW_API
bool burrow_filters_isset_wait(const burrow_filters_st *filters);

/**
 * Unsets the wait filter parameter.
 *
 * @param filters filters struct
 */
BURROW_API
void burrow_filters_unset_wait(burrow_filters_st *filters);



/**
 * Sets the detail filter.
 *
 * @param filters filters struct
 * @param detail detail level
 */
BURROW_API
void burrow_filters_set_detail(burrow_filters_st *filters, burrow_detail_t detail);

/**
 * Gets the detail filter value. Behavior is undefined if
 * detail is not set.
 *
 * @param filters filters struct
 * @return the value of the filter
 */
BURROW_API
burrow_detail_t burrow_filters_get_detail(const burrow_filters_st *filters);

/**
 * Checks if detail is set.
 *
 * @param filters filters struct
 * @return true if field is set, false otherwise
 */
BURROW_API
bool burrow_filters_isset_detail(const burrow_filters_st *filters);

/**
 * Unsets the detail filter parameter.
 *
 * @param filters filters struct
 */
BURROW_API
void burrow_filters_unset_detail(burrow_filters_st *filters);



/**
 * Sets the marker filter. Note: If marker_id is NULL, then marker will
 * be unset.
 *
 * @param filters filters struct
 * @param marker_id null-terminated marker-id
 */
BURROW_API
void burrow_filters_set_marker(burrow_filters_st *filters, const char *marker_id);

/**
 * Gets the marker filter value. If the marker value is not set, NULL
 * is returned.
 *
 * @param filters filters struct
 * @return the value of the filter
 */
BURROW_API
const char *burrow_filters_get_marker(const burrow_filters_st *filters);

#ifdef __cplusplus
}
#endif

#endif /* __BURROW_FILTERS_H */