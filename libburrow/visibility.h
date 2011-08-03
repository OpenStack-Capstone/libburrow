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
 * @brief API visibility macro definitions
 */

#ifndef __BURROW_VISIBILITY_H
#define __BURROW_VISIBILITY_H

/**
 * Be sure to put BURROW_API in front of all public API symbols, or one of
 * the other macros as appropriate. The default visibility without a macro is
 * to be hidden (BURROW_LOCAL).
 */

#if defined(BUILDING_LIBBURROW) && defined(HAVE_VISIBILITY)
# if defined(__GNUC__)
#  define BURROW_API __attribute__ ((visibility("default")))
#  define BURROW_INTERNAL_API __attribute__ ((visibility("hidden")))
#  define BURROW_API_DEPRECATED __attribute__ ((deprecated,visibility("default")))
#  define BURROW_LOCAL  __attribute__ ((visibility("hidden")))
# elif (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x550)) || (defined(__SUNPRO_CC) && (__SUNPRO_CC >= 0x550))
#  define BURROW_API __global
#  define BURROW_INTERNAL_API __hidden
#  define BURROW_API_DEPRECATED __global
#  define BURROW_LOCAL __hidden
# elif defined(_MSC_VER)
#  define BURROW_API extern __declspec(dllexport)
#  define BURROW_INTERNAL_API extern __declspec(dllexport)
#  define BURROW_DEPRECATED_API extern __declspec(dllexport)
#  define BURROW_LOCAL
# endif
#else  /* defined(BUILDING_LIBBURROW) && defined(HAVE_VISIBILITY) */
# if defined(_MSC_VER)
#  define SCALESTACK_API extern __declspec(dllimport)
#  define BURROW_INTERNAL_API extern __declspec(dllimport)
#  define BURROW_API_DEPRECATED extern __declspec(dllimport)
#  define BURROW_LOCAL
# else
#  define BURROW_API
#  define BURROW_INTERNAL_API
#  define BURROW_API_DEPRECATED
#  define BURROW_LOCAL
# endif /* defined(_MSC_VER) */
#endif  /* defined(BUILDING_BURROW) && defined(HAVE_VISIBILITY) */


#endif /* __BURROW_VISIBILITY_H */
