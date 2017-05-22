AUTOMAKE_OPTIONS = foreign subdir-objects
CLEANFILES = *.core
#CLEANFILES += *.in

noinst_PROGRAMS=testfoo testdaemonize testhttp testhttpuri testmnhttpc

noinst_HEADERS = unittest.h

if DEBUG
DEBUG_FLAGS = -g -O0 @CLANG_DEBUG@
else
DEBUG_FLAGS = -DNDEBUG -O3
endif

common_cflags = @_GNU_SOURCE_MACRO@ @_XOPEN_SOURCE_MACRO@ $(DEBUG_FLAGS) -Wall -Wextra -Werror -std=c99 -I$(top_srcdir)/src -I$(top_srcdir) -I$(includedir)
testfoo_SOURCES = testfoo.c
testfoo_CFLAGS = $(common_cflags)
testfoo_LDFLAGS = -L$(top_srcdir)/src/.libs -lmrkapp

testdaemonize_SOURCES = testdaemonize.c
testdaemonize_CFLAGS = $(common_cflags)
testdaemonize_LDFLAGS = -L$(top_srcdir)/src/.libs -lmrkapp

testhttp_SOURCES = testhttp.c
testhttp_CFLAGS = $(common_cflags)
testhttp_LDFLAGS = -L$(top_srcdir)/src/.libs -lmrkapp

testhttpuri_SOURCES = testhttpuri.c
testhttpuri_CFLAGS = $(common_cflags)
testhttpuri_LDFLAGS = -L$(top_srcdir)/src/.libs -lmrkapp

testmnhttpc_SOURCES = testmnhttpc.c
testmnhttpc_CFLAGS = $(common_cflags)
testmnhttpc_LDFLAGS = -L$(top_srcdir)/src/.libs -lmrkapp -lssl

testrun: all
	for i in $(noinst_PROGRAMS); do if test -x ./$$i; then LD_LIBRARY_PATH=$(libdir) ./$$i; fi; done;
