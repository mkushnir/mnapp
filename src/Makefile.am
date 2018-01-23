AUTOMAKE_OPTIONS = foreign subdir-objects

diags = diag.txt
BUILT_SOURCES = diag.c diag.h
EXTRA_DIST = $(diags)
CLEANFILES = $(BUILT_SOURCES) *.core
#CLEANFILES += *.in

lib_LTLIBRARIES = libmrkapp.la

nobase_include_HEADERS = mrkapp.h mrkhttp.h mnhttpc.h

noinst_HEADERS = mrkapp_private.h bytestream_ssl_helper.h

libmrkapp_la_SOURCES = mrkapp.c \
		       bytestream_ssl_helper.c \
		       mrkhttp.c \
		       mnhttpc.c \
		       local_server.c \
		       daemonize.c

nodist_libmrkapp_la_SOURCES = diag.c

if DEBUG
    DEBUG_FLAGS = -g -O0 @CLANG_DEBUG@
else
DEBUG_FLAGS = -DNDEBUG -O3
endif

libmrkapp_la_CFLAGS = $(DEBUG_FLAGS) -Wall -Wextra -Werror -std=c99 @MRKCOMMON_LOCAL_CFLAGS@ @MRKTHR_LOCAL_CFLAGS@ @_GNU_SOURCE_MACRO@ @_XOPEN_SOURCE_MACRO@ -I$(includedir)

libmrkapp_la_LDFLAGS = -version-info 0:0:0 -lmrkthr @MRKTHR_LOCAL_LDFLAGS@ -lmrkcommon -lmndiag @MRKCOMMON_LOCAL_LDFLAGS@ -L$(libdir) -lcrypto -lssl -lz

diag.c diag.h: $(diags)
	$(AM_V_GEN) cat $(diags) | sort -u >diag.txt.tmp && mndiagen -v -S diag.txt.tmp -L mrkapp -H diag.h -C diag.c *.[ch]

run: all
	for i in $(bin_PROGRAMS); do if test -x ./$$i; then LD_LIBRARY_PATH=$(libdir) ./$$i; fi; done;

testrun:
	for i in $(SUBDIRS); do if test "$$i" != "."; then cd $$i && $(MAKE) testrun && cd ..; fi; done;
