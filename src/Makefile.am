diags = diag.txt
BUILT_SOURCES = diag.c diag.h
EXTRA_DIST = $(diags)
CLEANFILES = $(BUILT_SOURCES) *.core
#CLEANFILES += *.in

lib_LTLIBRARIES = libmrkapp.la

nobase_include_HEADERS = mrkapp.h mnhttp.h mnhttpc.h mncommand.h

noinst_HEADERS = mrkapp_private.h bytestream_ssl_helper.h

libmrkapp_la_SOURCES = mrkapp.c \
		       bytestream_ssl_helper.c \
		       mnhttp.c \
		       mnhttpc.c \
		       mncommand.c \
		       local_server.c \
		       daemonize.c

nodist_libmrkapp_la_SOURCES = diag.c

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
libmrkapp_la_LDFLAGS = -all-static
else
libmrkapp_la_LDFLAGS =
endif

libmrkapp_la_CFLAGS = $(DEBUG_CC_FLAGS) -Wall -Wextra -Werror -std=c99 @_GNU_SOURCE_MACRO@ @_XOPEN_SOURCE_MACRO@ -I$(top_srcdir)/src -I$(top_srcdir) -I$(includedir)

libmrkapp_la_LDFLAGS += $(DEBUG_LD_FLAGS) -version-info 0:0:0 -L$(libdir)
libmrkapp_la_LIBADD = -lmrkthr -lmrkcommon -lmndiag -lcrypto -lssl

diag.c diag.h: $(diags)
	$(AM_V_GEN) cat $(diags) | sort -u >diag.txt.tmp && mndiagen -v -S diag.txt.tmp -L mrkapp -H diag.h -C diag.c *.[ch]

run: all
	for i in $(bin_PROGRAMS); do if test -x ./$$i; then LD_LIBRARY_PATH=$(libdir) ./$$i; fi; done;

testrun:
	for i in $(SUBDIRS); do if test "$$i" != "."; then cd $$i && $(MAKE) testrun && cd ..; fi; done;
