AUTOMAKE_OPTIONS = foreign subdir-objects
CLEANFILES = *.core
#CLEANFILES += *.in

noinst_PROGRAMS=testfoo testdaemonize

distdir = $(top_srcdir)/$(PACKAGE)-$(VERSION)/test
dist_HEADERS = unittest.h

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

testrun: all
	for i in $(noinst_PROGRAMS); do if test -x ./$$i; then LD_LIBRARY_PATH=$(libdir) ./$$i; fi; done;
