#include <assert.h>
#include <stdlib.h>
#include <time.h>

#include <mncommon/unittest.h>
#define TRRET_DEBUG
#include <mncommon/dumpm.h>
#include <mncommon/util.h>

#include <mnapp.h>

#include "diag.h"

#ifndef NDEBUG
const char *_malloc_options = "AJ";
#endif

UNUSED static void
test0(void)
{
    struct {
        long rnd;
        int in;
        int expected;
    } data[] = {
        {0, 0, 0},
    };
    UNITTEST_PROLOG_RAND;

    FOREACHDATA {
        TRACE("in=%d expected=%d", CDATA.in, CDATA.expected);
        assert(CDATA.in == CDATA.expected);
    }
}


static int
_mycb(UNUSED int argc, void **argv)
{
    mnapp_tcp_server_t *srv;
    int fd;
    UNUSED void *udata;

    assert(argc == 3);
    srv = argv[0];
    fd = (int)(intptr_t)argv[1];
    udata = argv[2];

    char buf[32];
    ssize_t nread;

    if (srv->shutting_down) {
        return 1;
    }
    while ((nread = mnthr_read_allb(fd, buf, sizeof(buf))) > 0) {
        D8(buf, nread);
        if (*buf == 'q') {
            CTRACE("quit ...");
            break;
        }
    }
    close(fd);
    return 0;
}


static int
mycb(mnapp_tcp_server_t *srv, mnthr_socket_t *sock, UNUSED void *udata)
{
    (void)MNTHR_SPAWN(NULL, _mycb, srv, sock->fd, udata);
    return 0;
}


static int
test1(UNUSED int argc, void **argv)
{
    const char *addr;
    int res;
    mnapp_tcp_server_t srv;

    assert(argc == 1);
    addr = argv[0];

    mnapp_tcp_server_init(&srv, 1, addr, mycb, NULL);
    if ((res = mnapp_tcp_server_start(&srv)) != 0) {
        TR(res);
    }
    (void)mnapp_tcp_server_serve(&srv);
    mnapp_tcp_server_stop(&srv);
    mnapp_tcp_server_fini(&srv);
    return 0;
}


static int
test2(UNUSED int argc, UNUSED void **argv)
{
    while (true) {
        mnthr_sleep(1000);
        CTRACE();
    }

    return 0;
}


int
main(int argc, char **argv)
{
    if (argc != 2) {
        FAIL("main");
    }
    mnthr_init();
    (void)MNTHR_SPAWN("test1", test1, argv[1]);
    (void)MNTHR_SPAWN("test2", test2);
    mnthr_loop();
    mnthr_fini();
    return 0;
}
