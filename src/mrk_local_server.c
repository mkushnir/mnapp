#include <assert.h>

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
mrk_local_server_shutdown(void)
{
    _shutdown = 1;
}

int
mrk_local_server(int argc, void **argv)
{
    const char *path;
    int(*cb)(int, void **);
    void *udata;

    int s = -1;
    struct sockaddr_un sa;
    size_t sz;
    struct stat sb;

    assert(argc == 3);

    path = argv[0];
    cb = argv[1];
    udata = argv[2];

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
    sa.sun_len = strlen(sa.sun_path);

    if ((s = socket(PF_LOCAL, SOCK_STREAM, 0)) < 0) {
        FAIL("socket");
    }

    if (bind(s, (struct sockaddr *)&sa, SUN_LEN(&sa)) != 0) {
        FAIL("bind");
    }

    if (listen(s, 1) != 0) {
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
            mrkthr_ctx_t *thr;

            psock = sockets + i;
            TRACE("accepted fd=%d", psock->fd);

            if ((thr = mrkthr_new(NULL, cb, 2, psock->fd, udata)) == NULL) {
                FAIL("mrkthr_new");
            }
            mrkthr_run(thr);
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



