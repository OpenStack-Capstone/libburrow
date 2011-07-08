#!/bin/sh
#
# libburrow -- Burrow Client Library
#
# Copyright (C) 2010 Eric Day (eday@oddments.org)
# All rights reserved.
#
# Use and distribution licensed under the BSD license. See the
# COPYING file in the root project directory for full text.
#

# Get filename we want to run without path

# On MAC OS X, GNU libtool is named 'glibtool':
case `uname -s` in 
'Darwin')
  LIBTOOL_BIN="glibtool"
  ;;
*)
  LIBTOOL_BIN="libtool"
  ;;
esac
  LIBTOOL_BIN="libtool"


name=`echo $1 | sed 's/.*\/\(libburrow\/.*[^\/]*\)$/\1/'`

ext=`echo $name | sed 's/.*\.\([^.]*$\)/\1/'`
if [ "x$ext" = "x$name" ]
then
  ext=""
fi

if [ ! "x$ext" = "xsh" ]
then
  libtool_prefix="$LIBTOOL_BIN --mode=execute"
fi

# Set prefix if it was given through environment
if [ -n "$LIBBURROW_TEST_PREFIX" ]
then
  if [ -n "$LIBBURROW_TEST_FILTER" ]
  then
    # If filter variable is set, only apply prefix to those that match
    for x in $LIBBURROW_TEST_FILTER
    do
      if [ "x$x" = "x$name" ]
      then
        prefix="$libtool_prefix $LIBBURROW_TEST_PREFIX"
        with=" (with prefix after filter)"
        break
      fi
    done
  else
    prefix="$libtool_prefix $LIBBURROW_TEST_PREFIX"
    with=" (with prefix)"
  fi
fi

# Set this to fix broken libtool test
ECHO=`which echo`
export ECHO

# This needs to be set because of broken libtool on OSX
DYLD_LIBRARY_PATH=libburrow/.libs
export DYLD_LIBRARY_PATH

$prefix $1 $LIBBURROW_TEST_ARGS
