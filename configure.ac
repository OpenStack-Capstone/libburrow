#
# libburrow -- Burrow Client Library
#
# Copyright 2011 Tony Wooster 
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

AC_INIT([libburrow],[0.1],[https://github.com/OpenStack-Capstone/libburrow])
AC_CONFIG_SRCDIR([libburrow/burrow.h])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR([m4])

PANDORA_CANONICAL_TARGET(ignore-shared-ptr)

# Increment if interfaces have been added, removed, or changed.
LIBBURROW_VERSION_CURRENT=0

# Increment if source code has changed, set to zero if current is incremented.
LIBBURROW_VERSION_REVISION=1

# Increment if interfaces have been added, set to zero if interfaces have been
# removed or changed.
LIBBURROW_VERSION_AGE=0

AC_SUBST(LIBBURROW_VERSION_CURRENT)
AC_SUBST(LIBBURROW_VERSION_REVISION)
AC_SUBST(LIBBURROW_VERSION_AGE)

AC_DEFINE_UNQUOTED([LIBBURROW_VERSION_CURRENT],
                   [$LIBBURROW_VERSION_CURRENT],
                   [Current portion of libburrow version])
AC_DEFINE_UNQUOTED([LIBBURROW_VERSION_REVISION],
                   [$LIBBURROW_VERSION_REVISION],
                   [Revision portion of libburrow version])
AC_DEFINE_UNQUOTED([LIBBURROW_VERSION_AGE],
                   [$LIBBURROW_VERSION_AGE],
                   [Age portion of libburrow version])

LIBBURROW_VERSION=$LIBBURROW_VERSION_CURRENT:$LIBBURROW_VERSION_REVISION:$LIBBURROW_VERSION_AGE

AC_SUBST(LIBBURROW_VERSION)

AC_LANG_PUSH(C)
PANDORA_REQUIRE_LIBCURL
#PANDORA_REQUIRE_PTHREAD
#PANDORA_REQUIRE_LIBDL
AC_LANG_POP

PANDORA_HAVE_LIBUUID

PANDORA_PLUGINS

PKG_CHECK_EXISTS([libcurl >= 7.21.2],[],[AC_DEFINE([CURL_DISABLE_TYPECHECK],[1],[old library])])

AC_DEFINE_UNQUOTED([BURROW_MODULE_EXT], ["$acl_cv_shlibext"],
                   [Extension to use for modules.])

AC_CHECK_HEADERS([stdarg.h stdio.h stdlib.h string.h poll.h errno.h])

AC_CONFIG_FILES(Makefile docs/doxygen/header.html)
AC_CONFIG_FILES(support/libburrow.pc support/libburrow.spec)

AC_OUTPUT

echo "---"
echo "Configuration summary for $PACKAGE_NAME version $VERSION"
echo ""
echo "   * Installation prefix:       $prefix"
echo "   * System type:               $host_vendor-$host_os"
echo "   * Host CPU:                  $host_cpu"
echo "   * C Compiler:                $CC_VERSION"
echo "   * Assertions enabled:        $ac_cv_assert"
echo "   * Debug enabled:             $with_debug"
echo "   * Profiling enabled:         $ac_profiling"
echo "   * Coverage enabled:          $ac_coverage"
echo "   * Warnings as errors:        $ac_cv_warnings_as_errors"
echo "   * Configured modules:"
echo ""

for module in `echo $PANDORA_PLUGIN_LIST | sed 's/_/::/g' | tr ',' '\n' | sort`
do
  echo "     - $module"
done

echo ""
echo "---"




