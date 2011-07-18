#ifndef __BURROW_COMMON_H
#define __BURROW_COMMON_H

#include "config.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STDARG_H   
#include <stdarg.h>
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include "burrow.h"
#include "structs-local.h"
#include "internal.h"

#include "backends.h"
#include "macros.h"

#endif
