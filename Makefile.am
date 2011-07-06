#
# libburrow -- Burrow Client Library
#
# Copyright (C) 2011 Tony Wooster (twooster@gmail.com)
# All rights reserved.
#
# Use and distribution licensed under the BSD license.  See
# the COPYING file in this directory for full text.
#

ACLOCAL_AMFLAGS= -I m4
LDADD=libburrow.la

bin_PROGRAMS =
check_PROGRAMS =
noinst_PROGRAMS =
lib_LTLIBRARIES =
noinst_LTLIBRARIES =
nobase_include_HEADERS =
noinst_HEADERS =
pkgplugin_LTLIBRARIES =
CLEANFILES =
EXTRA_DIST = \
  docs/doxygen/Doxyfile \
  docs/doxygen/header.html.in \
  tests/run.sh

#
# libburrow
#

lib_LTLIBRARIES += libburrow.la
libburrow_la_SOURCES = \
	libburrow/burrow.c

nobase_include_HEADERS += \
	libburrow/burrow.h \
	libburrow/visibility.h

libburrow_la_CFLAGS = \
	${AM_CFLAGS} \
	-DBUILDING_LIBBURROW

libburrow_la_LIBADD = $(LIBDL_LIBS)
libburrow_la_LDFLAGS = $(AM_LDFLAGS) -version-info $(LIBBURROW_LIBRARY_VERSION)

#
# Simple libburrow application
#

include config/pandora-plugin.am

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
	cd project_name && lcov --directory . --base-directory .. --capture --output-file lcov.out
	cd project_name && lcov --directory . --base-directory .. --extract lcov.out `pwd`/\* --output-file lcov_extract.out
	genhtml -o lcov -t project_name project_name/lcov_extract.out

lcov-clean: clean
	find . -name '*.gcno' -exec rm {} \;
	find . -name '*.gcda' -exec rm {} \;
	find . -name 'lcov*.out' -exec rm {} \;