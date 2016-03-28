#include <assert.h>
#include <stdlib.h>
#include <time.h>

#include "unittest.h"
#include <mrkcommon/dumpm.h>
#include <mrkcommon/util.h>
#include <mrkapp.h>

#ifndef NDEBUG
const char *_malloc_options = "AJ";
#endif

static void
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
    fprintf(stdout, "qweqweqwe\n");
    fflush(stdout);
    fprintf(stderr, "asdasdasd\n");
    fflush(stderr);
}

int
main(void)
{
    daemonize("qwe.pid", "qwe.stdout", "qwe.stdout");
    while (1) {
        test0();
        sleep(1);
    }
    return 0;
}
