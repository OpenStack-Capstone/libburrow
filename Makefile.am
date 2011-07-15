#
# libburrow -- Burrow Client Library
#
# Copyright (C) 2011 Tony Wooster (twooster@gmail.com)
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in this directory for full text.
#

ACLOCAL_AMFLAGS = -I m4
LDADD = libburrow.la $(LIBCURL_LIBS)

#bin_PROGRAMS =
lib_LTLIBRARIES =
#noinst_LTLIBRARIES =
nobase_include_HEADERS =
#pkgplugin_LTLIBRARIES =
CLEANFILES =


noinst_PROGRAMS = \
  examples/complete_loop

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
	libburrow/backends/dummy/dummy.c \
	libburrow/attributes.c \
	libburrow/filters.c \
	libburrow/backends.c \
	libburrow/backends/http/curl_backend.c \
	libburrow/backends/http/parser/contrib/JSON_PARSER/JSON_parser.c


nobase_include_HEADERS += \
	libburrow/burrow.h \
	libburrow/visibility.h \
	libburrow/attributes.h

noinst_HEADERS = \
	libburrow/common.h \
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
	tests/burrow_filters_st \
	tests/burrow_st \
	tests/burrow_backend_dummy \
	tests/burrow_attributes_st

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
	cd libburrow && lcov --directory . --base-directory .. --extract lcov.out `pwd`/\* --output-file lcov_extract.out
	genhtml -o lcov -t libburrow libburrow/lcov_extract.out

lcov-clean: clean
	find . -name '*.gcno' -exec rm {} \;
	find . -name '*.gcda' -exec rm {} \;
	find . -name 'lcov*.out' -exec rm {} \;
