#include <assert.h>
#include <stdlib.h>
#include <time.h>

#include "unittest.h"
#define TRRET_DEBUG
#include <mrkcommon/dumpm.h>
#include <mrkcommon/util.h>

#include <mrkapp.h>

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
_mycb(int argc, void **argv)
{
    mrkapp_tcp_server_t *srv;
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
    while ((nread = mrkthr_read_allb(fd, buf, sizeof(buf))) > 0) {
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
mycb(mrkapp_tcp_server_t *srv, mrkthr_socket_t *sock, UNUSED void *udata)
{
    mrkthr_spawn(NULL, _mycb, 3, srv, sock->fd, udata);
    return 0;
}

static int
test1(int argc, void **argv)
{
    const char *addr;
    int res;
    mrkapp_tcp_server_t srv;

    assert(argc == 1);
    addr = argv[0];

    mrkapp_tcp_server_init(&srv, 1, addr, mycb, NULL);
    if ((res = mrkapp_tcp_server_start(&srv)) != 0) {
        TR(res);
    }
    (void)mrkapp_tcp_server_serve(&srv);
    mrkapp_tcp_server_stop(&srv);
    mrkapp_tcp_server_fini(&srv);
    return 0;
}


static int
test2(UNUSED int argc, UNUSED void **argv)
{
    while (true) {
        mrkthr_sleep(1000);
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
    mrkthr_init();
    mrkthr_spawn("test1", test1, 1, argv[1]);
    mrkthr_spawn("test2", test2, 0);
    mrkthr_loop();
    mrkthr_fini();
    return 0;
}
