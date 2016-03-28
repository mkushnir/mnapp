#include <assert.h>

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>

#include <mrkthr.h>
#include <mrkcommon/dumpm.h>

#include "diag.h"

int _shutdown = 0;

void
local_server_shutdown(void)
{
    _shutdown = 1;
}

int
local_server(UNUSED int argc, void **argv)
{
    int listen_backlog;
    const char *path;
    int(*cb)(int, void **);
    void *udata;

    int s = -1;
    struct sockaddr_un sa;
    size_t sz;
    struct stat sb;

    assert(argc == 4);

    listen_backlog = (int)(intptr_t)(argv[0]);
    path = argv[1];
    cb = argv[2];
    udata = argv[3];

    if (listen_backlog <= 0) {
        listen_backlog = 1;
    }

    if (stat(path, &sb) == 0) {
        if (S_ISSOCK(sb.st_mode)) {
            CTRACE("unlinking: %s", path);
            unlink(path);
        }
    }

    memset(&sa, '\0', sizeof(struct sockaddr_un));

    sz = MIN(strlen(path) + 1, sizeof(sa.sun_path));
    sa.sun_family = PF_LOCAL;
    strncpy(sa.sun_path, path, sz);
    sa.sun_path[sz] = '\0';
#ifdef HAVE_SUN_LEN
    sa.sun_len = strlen(sa.sun_path);
#endif

    if ((s = socket(PF_LOCAL, SOCK_STREAM, 0)) < 0) {
        FAIL("socket");
    }

    if (bind(s, (struct sockaddr *)&sa, SUN_LEN(&sa)) != 0) {
        FAIL("bind");
    }

    if (listen(s, listen_backlog) != 0) {
        FAIL("listen");
    }

    while (!_shutdown) {
        mrkthr_socket_t *sockets = NULL;
        off_t sz = 0, i;

        if (mrkthr_accept_all(s, &sockets, &sz) != 0) {
            break;
        }
        for (i = 0; i < sz; ++i) {
            mrkthr_socket_t *psock;
            char buf[32];

            psock = sockets + i;
            CTRACE("accepted fd=%d", psock->fd);
            snprintf(buf, sizeof(buf), "local_server @%d", psock->fd);

            mrkthr_spawn(buf, cb, 2, psock->fd, udata);
        }

        if (sockets != NULL) {
            free(sockets);
            sockets = NULL;
        }
        sz = 0;

    }
    if (s != -1) {
        close(s);
    }
    unlink(path);
    CTRACE("exiting ...");
    return 0;
}



