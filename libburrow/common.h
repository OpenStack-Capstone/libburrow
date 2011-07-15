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

#include <stdio.h>

#include <poll.h>


#include "burrow.h"
#include "structs-local.h"
#include "internal.h"

#include "backends.h"
#include "macros.h"

#endif
