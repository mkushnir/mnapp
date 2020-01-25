diags = diag.txt
BUILT_SOURCES = diag.c diag.h
EXTRA_DIST = $(diags)
CLEANFILES = $(BUILT_SOURCES) *.core
#CLEANFILES += *.in

lib_LTLIBRARIES = libmnapp.la

nobase_include_HEADERS = mnapp.h mnhttp.h mncommand.h

noinst_HEADERS = mnapp_private.h

libmnapp_la_SOURCES = mnapp.c mnhttp.c mncommand.c daemonize.c

if !THIN
libmnapp_la_SOURCES += local_server.c bytestream_ssl_helper.c mnhttpc.c
nobase_include_HEADERS += mntcps.h mnhttpc.h
noinst_HEADERS += bytestream_ssl_helper.h
endif

nodist_libmnapp_la_SOURCES = diag.c

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
libmnapp_la_LDFLAGS = -all-static
else
libmnapp_la_LDFLAGS =
endif

libmnapp_la_CFLAGS = $(DEBUG_CC_FLAGS) -Wall -Wextra -Werror -std=c99 @_GNU_SOURCE_MACRO@ @_XOPEN_SOURCE_MACRO@ -I$(top_srcdir)/src -I$(top_srcdir) -I$(includedir)

libmnapp_la_LDFLAGS += $(DEBUG_LD_FLAGS) -version-info 0:0:0 -L$(libdir)
libmnapp_la_LIBADD = -lmncommon -lmndiag

if !THIN
libmnapp_la_LIBADD += -lmnthr -lcrypto -lssl
endif

diag.c diag.h: $(diags)
	$(AM_V_GEN) cat $(diags) | sort -u >diag.txt.tmp && mndiagen -v -S diag.txt.tmp -L mnapp -H diag.h -C diag.c *.[ch]

run: all
	for i in $(bin_PROGRAMS); do if test -x ./$$i; then LD_LIBRARY_PATH=$(libdir) ./$$i; fi; done;

testrun:
	for i in $(SUBDIRS); do if test "$$i" != "."; then cd $$i && $(MAKE) testrun && cd ..; fi; done;
