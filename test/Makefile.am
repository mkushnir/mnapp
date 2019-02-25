CLEANFILES = *.core
#CLEANFILES += *.in

noinst_PROGRAMS=testfoo testdaemonize testhttp testhttpuri testmnhttpc

noinst_HEADERS = unittest.h

DEBUG_LD_FLAGS =
if DEBUG
DEBUG_CC_FLAGS = -g -O0 @CC_DEBUG@
DEBUG_LD_FLAGS += -no-install
else
DEBUG_CC_FLAGS = -DNDEBUG -O3 -fomit-frame-pointer
if LTO
DEBUG_CC_FLAGS += @CC_LTO@
DEBUG_LD_FLAGS += @LD_LTO@
endif
endif

if ALLSTATIC
testfoo_LDFLAGS = -all-static
testdaemonize_LDFLAGS = -all-static
testhttp_LDFLAGS = -all-static
testhttpuri_LDFLAGS = -all-static
testmnhttpc_LDFLAGS = -all-static
else
testfoo_LDFLAGS =
testdaemonize_LDFLAGS =
testhttp_LDFLAGS =
testhttpuri_LDFLAGS =
testmnhttpc_LDFLAGS =
endif

common_sources = ../src/bytestream_ssl_helper.c \
		  ../src/daemonize.c \
		  ../src/diag.c \
		  ../src/diag.h \
		  ../src/local_server.c \
		  ../src/mncommand.c \
		  ../src/mnhttp.c \
		  ../src/mnhttpc.c \
		  ../src/mrkapp.c

common_cflags = @_GNU_SOURCE_MACRO@ @_XOPEN_SOURCE_MACRO@ $(DEBUG_CC_FLAGS) -Wall -Wextra -Werror -std=c99 -I$(top_srcdir)/src -I$(top_srcdir) -I$(includedir)

testfoo_SOURCES = $(common_sources) testfoo.c

testfoo_CFLAGS = $(common_cflags)
testfoo_LDFLAGS += $(DEBUG_LD_FLAGS) -L$(top_srcdir)/src/.libs
testfoo_LDADD = -lmrkapp

testdaemonize_SOURCES = $(common_sources) testdaemonize.c

testdaemonize_CFLAGS = $(common_cflags)
testdaemonize_LDFLAGS += $(DEBUG_LD_FLAGS) -L$(top_srcdir)/src/.libs
testdaemonize_LDADD = -lmrkapp

testhttp_SOURCES = $(common_sources) testhttp.c

testhttp_CFLAGS = $(common_cflags)
testhttp_LDFLAGS += $(DEBUG_LD_FLAGS) -L$(top_srcdir)/src/.libs
testhttp_LDADD = -lmrkapp

testhttpuri_SOURCES = $(common_sources) testhttpuri.c
testhttpuri_CFLAGS = $(common_cflags)
testhttpuri_LDFLAGS += $(DEBUG_LD_FLAGS) -L$(top_srcdir)/src/.libs
testhttpuri_LDADD = -lmrkapp

testmnhttpc_SOURCES = $(common_sources) testmnhttpc.c
testmnhttpc_CFLAGS = $(common_cflags)
testmnhttpc_LDFLAGS += $(DEBUG_LD_FLAGS) -L$(top_srcdir)/src/.libs
testmnhttpc_LDADD = -lmrkapp -lssl

testrun: all
	for i in $(noinst_PROGRAMS); do if test -x ./$$i; then LD_LIBRARY_PATH=$(libdir) ./$$i; fi; done;
