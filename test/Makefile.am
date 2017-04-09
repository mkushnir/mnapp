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

testfoo_SOURCES = testfoo.c
testfoo_CFLAGS = @_GNU_SOURCE_MACRO@ $(DEBUG_FLAGS) -Wall -Wextra -Werror -std=c99 -I$(top_srcdir)/src -I$(top_srcdir) -I$(includedir)
testfoo_LDFLAGS = -L$(top_srcdir)/src/.libs -lmrkapp

testdaemonize_SOURCES = testdaemonize.c
testdaemonize_CFLAGS = @_GNU_SOURCE_MACRO@ $(DEBUG_FLAGS) -Wall -Wextra -Werror -std=c99 -I$(top_srcdir)/src -I$(top_srcdir) -I$(includedir)
testdaemonize_LDFLAGS = -L$(top_srcdir)/src/.libs -lmrkapp

testhttp_SOURCES = testhttp.c
testhttp_CFLAGS = @_GNU_SOURCE_MACRO@ $(DEBUG_FLAGS) -Wall -Wextra -Werror -std=c99 -I$(top_srcdir)/src -I$(top_srcdir) -I$(includedir)
testhttp_LDFLAGS = -L$(top_srcdir)/src/.libs -lmrkapp

testhttpuri_SOURCES = testhttpuri.c
testhttpuri_CFLAGS = @_GNU_SOURCE_MACRO@ $(DEBUG_FLAGS) -Wall -Wextra -Werror -std=c99 -I$(top_srcdir)/src -I$(top_srcdir) -I$(includedir)
testhttpuri_LDFLAGS = -L$(top_srcdir)/src/.libs -lmrkapp

testmnhttpc_SOURCES = testmnhttpc.c
testmnhttpc_CFLAGS = @_GNU_SOURCE_MACRO@ $(DEBUG_FLAGS) -Wall -Wextra -Werror -std=c99 -I$(top_srcdir)/src -I$(top_srcdir) -I$(includedir)
testmnhttpc_LDFLAGS = -L$(top_srcdir)/src/.libs -lmrkapp -lssl

testrun: all
	for i in $(noinst_PROGRAMS); do if test -x ./$$i; then LD_LIBRARY_PATH=$(libdir) ./$$i; fi; done;
