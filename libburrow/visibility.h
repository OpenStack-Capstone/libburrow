/*
 * libburrow -- Burrow Client Library
 *
 * Copyright (C) 2011 Tony Wooster (twooster@gmail.com)
 * All rights reserved.
 *
 * Use and distribution licensed under the BSD license.  See
 * the COPYING file in this directory for full text.
 *
 * Implementation copied from libdrizzle, which was
 * drawn from visiblity.texi in gnulib. That implementation
 * copyright (c) 2008 Eric Day (eday@oddments.org).
 */

/**
 * @file
 * @brief Visibility Control Macros
 */
#ifndef __BURROW_VISIBILITY_H
#define __BURROW_VISIBILITY_H

/**
 *
 * BURROW_API is used for the public API symbols. It either DLL imports or
 * DLL exports (or does nothing for static build).
 *
 * BURROW_LOCAL is used for non-api symbols.
 */

#if defined(BUILDING_LIBBURROW)
# if defined(HAVE_VISIBILITY)
#  define BURROW_API __attribute__ ((visibility("default")))
#  define BURROW_LOCAL  __attribute__ ((visibility("hidden")))
# elif defined (__SUNPRO_C) && (__SUNPRO_C >= 0x550)
#  define BURROW_API __global
#  define BURROW_API __hidden
# elif defined(_MSC_VER)
#  define BURROW_API extern __declspec(dllexport) 
#  define BURROW_LOCAL
# endif /* defined(HAVE_VISIBILITY) */
#else  /* defined(BUILDING_LIBBURROW) */
# if defined(_MSC_VER)
#  define BURROW_API extern __declspec(dllimport) 
#  define BURROW_LOCAL
# else
#  define BURROW_API
#  define BURROW_LOCAL
# endif /* defined(_MSC_VER) */
#endif /* defined(BUILDING_LIBBURROW) */

#endif /* __BURROW_VISIBILITY_H */
