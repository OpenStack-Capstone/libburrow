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

ACLOCAL_AMFLAGS = -I m4
LDADD = libburrow.la ${LTLIBCURL}
LCOV_REMOVE = '*/contrib/*'

bin_PROGRAMS =
lib_LTLIBRARIES =
noinst_LTLIBRARIES = 
nobase_include_HEADERS =
pkgplugin_LTLIBRARIES =
CLEANFILES =

DISTCLEANFILES = \
  config/top.h

noinst_PROGRAMS = \
  examples/complete_loop

if HAVE_LIBUUID
  noinst_PROGRAMS += examples/demo
  noinst_PROGRAMS += examples/oneshot
  examples_demo_LDFLAGS= ${LTLIBUUID}
  examples_oneshot_LDFLAGS= ${LTLIBUUID}
endif

EXTRA_DIST = \
  docs/doxygen/Doxyfile \
  docs/doxygen/header.html.in \
  tests/run.sh

MAINTAINERCLEANFILES = \
	m4/libtool.m4 \
	m4/ltoptions.m4 \
	m4/ltsugar.m4 \
	m4/ltversion.m4 \
	m4/lt~obsolete.m4

#
# libburrow
#

lib_LTLIBRARIES += libburrow.la
libburrow_la_SOURCES = \
	libburrow/burrow.c \
	libburrow/attributes.c \
	libburrow/filters.c \
	libburrow/backends.c \
	libburrow/backends/memory/memory.c \
	libburrow/backends/memory/dictionary.c \
	libburrow/backends/http/curl_backend.c \
	libburrow/backends/http/user_buffer.c \
	libburrow/backends/http/json_processing.c \
	libburrow/backends/http/parser/contrib/JSON_PARSER/JSON_parser.c \
	libburrow/backends/dummy/dummy.c


nobase_include_HEADERS += \
	libburrow/burrow.h \
	libburrow/constants.h \
	libburrow/structs.h \
	libburrow/backends.h \
	libburrow/attributes.h \
	libburrow/filters.h \
	libburrow/visibility.h

noinst_HEADERS = \
	libburrow/common.h \
	libburrow/constants-local.h \
	libburrow/structs-local.h \
	libburrow/internal.h \
	libburrow/macros.h \
	libburrow/backends/http/parser/contrib/JSON_PARSER/JSON_parser.h \
	libburrow/backends/memory/dictionary.h \
	libburrow/backends/http/curl_backend.h \
	libburrow/backends/http/user_buffer.h \
	libburrow/backends/http/json_processing.h \
	libburrow/backends/memory/memory.h \
	libburrow/backends/dummy/dummy.h \
	tests/common.h

libburrow_la_CFLAGS = \
	${AM_CFLAGS} \
	-DBUILDING_LIBBURROW

libburrow_la_LIBADD = $(LIBDL_LIBS)
libburrow_la_LDFLAGS = $(AM_LDFLAGS) -version-info $(LIBBURROW_VERSION)

#
# Simple libburrow application
#

include config/pandora-plugin.am

check_PROGRAMS = \
	tests/burrow_st \
	tests/burrow_filters_st \
	tests/burrow_attributes_st \
	tests/burrow_backend_memory \
	tests/burrow_backend_http

tests_burrow_backend_http_SOURCES = \
    tests/burrow_backend_http.c \
    tests/burrow_generic_tests.c

tests_burrow_backend_memory_SOURCES = \
    tests/burrow_backend_memory.c \
    tests/burrow_generic_tests.c

check_HEADERS = \
	tests/common.h \
	tests/burrow_generic_tests.h

TESTS = $(check_PROGRAMS)

TESTS_ENVIRONMENT = ${top_srcdir}/tests/run.sh

check-verbose:
	@LIBBURROW_TEST_ARGS="v" make check

check-debug:
	@LIBBURROW_TEST_ARGS="vvv" make check

check-valgrind:
	@LIBBURROW_TEST_PREFIX="valgrind -q --leak-check=full --show-reachable=yes --suppressions=valgrind.suppressions" make check

check-valgrind-debug:
	@LIBBURROW_TEST_PREFIX="valgrind -q --leak-check=full --show-reachable=yes --suppressions=valgrind.suppressions" LIBBURROW_TEST_ARGS="vvv" make check

check-gdb:
	@LIBBURROW_TEST_PREFIX="gdb -q" make check

check-time:
	@LIBBURROW_TEST_PREFIX="/usr/bin/time" make check

check-strace:
	@LIBBURROW_TEST_PREFIX="strace -c" make check

check-truss:
	@LIBBURROW_TEST_PREFIX="truss -c" make check

docs: all
	@echo "/**" > docs/doxygen/mainpage.h
	@echo "@mainpage README" >> docs/doxygen/mainpage.h
	@echo "<pre>" >> docs/doxygen/mainpage.h
	@cat ${top_srcdir}/README >> docs/doxygen/mainpage.h
	@echo "</pre>" >> docs/doxygen/mainpage.h
	@echo "*/" >> docs/doxygen/mainpage.h
	${DOXYGEN} ${top_srcdir}/docs/doxygen/Doxyfile
	@rm docs/doxygen/mainpage.h

lcov: lcov-clean check
	@echo
	@echo "------------------------------------------------------"
	@echo "Make sure ./configure was run with '--enable-coverage'"
	@echo "------------------------------------------------------"
	@echo
	cd libburrow && lcov --directory . --base-directory .. --capture --output-file lcov.out
	cd libburrow && lcov --directory . --base-directory .. --remove lcov.out ${LCOV_REMOVE} --output-file lcov_remove.out
	cd libburrow && lcov --directory . --base-directory .. --extract lcov_remove.out `pwd`/\* --output-file lcov_extract.out
	genhtml -o lcov -t libburrow libburrow/lcov_extract.out

lcov-clean: clean
	find . -name '*.gcno' -exec rm {} \;
	find . -name '*.gcda' -exec rm {} \;
	find . -name 'lcov*.out' -exec rm {} \;
