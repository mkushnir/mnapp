CLEANFILES = *.core
#CLEANFILES += *.in

noinst_PROGRAMS=testdaemonize testhttpuri

if !THIN
noinst_PROGRAMS += testfoo testhttp testmnhttpc
endif

noinst_HEADERS = unittest.h

DEBUG_LD_FLAGS =
if DEBUG
DEBUG_CC_FLAGS = -g -O0 @CC_DEBUG@
DEBUG_LD_FLAGS += @LIBTOOL_NO_INSTALL@
else
DEBUG_CC_FLAGS = -DNDEBUG -O3 -fomit-frame-pointer
if LTO
DEBUG_CC_FLAGS += @CC_LTO@
DEBUG_LD_FLAGS += @LD_LTO@
endif
endif

if ALLSTATIC
testdaemonize_LDFLAGS = -all-static
testhttpuri_LDFLAGS = -all-static

if !THIN
testfoo_LDFLAGS = -all-static
testhttp_LDFLAGS = -all-static
testmnhttpc_LDFLAGS = -all-static
endif
else
testdaemonize_LDFLAGS =
testhttpuri_LDFLAGS =

if !THIN
testfoo_LDFLAGS =
testhttp_LDFLAGS =
testmnhttpc_LDFLAGS =
endif
endif

common_cflags = @_GNU_SOURCE_MACRO@ @_XOPEN_SOURCE_MACRO@ $(DEBUG_CC_FLAGS) -Wall -Wextra -Werror -std=c99 -I$(top_srcdir)/src -I$(top_srcdir) -I$(includedir)

testdaemonize_SOURCES = testdaemonize.c
testdaemonize_CFLAGS = $(common_cflags)
testdaemonize_LDFLAGS += $(DEBUG_LD_FLAGS) -L$(top_srcdir)/src/.libs
testdaemonize_LDADD = -lmnapp

testhttpuri_SOURCES = testhttpuri.c
testhttpuri_CFLAGS = $(common_cflags)
testhttpuri_LDFLAGS += $(DEBUG_LD_FLAGS) -L$(top_srcdir)/src/.libs
testhttpuri_LDADD = -lmnapp

if !THIN
testfoo_SOURCES = testfoo.c
testfoo_CFLAGS = $(common_cflags)
testfoo_LDFLAGS += $(DEBUG_LD_FLAGS) -L$(top_srcdir)/src/.libs
testfoo_LDADD = -lmnapp

testhttp_SOURCES = testhttp.c
testhttp_CFLAGS = $(common_cflags)
testhttp_LDFLAGS += $(DEBUG_LD_FLAGS) -L$(top_srcdir)/src/.libs
testhttp_LDADD = -lmnapp

testmnhttpc_SOURCES = testmnhttpc.c
testmnhttpc_CFLAGS = $(common_cflags)
testmnhttpc_LDFLAGS += $(DEBUG_LD_FLAGS) -L$(top_srcdir)/src/.libs
testmnhttpc_LDADD = -lmnapp -lssl
endif

testrun: all
	for i in $(noinst_PROGRAMS); do if test -x ./$$i; then LD_LIBRARY_PATH=$(libdir) ./$$i; fi; done;
