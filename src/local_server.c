#include <assert.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>

//#define TRRET_DEBUG
#include <mrkcommon/dumpm.h>

#include <mrkthr.h>

#include <mrkapp.h>

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



void
mrkapp_tcp_server_init(mrkapp_tcp_server_t *srv,
                       int listen_backlog,
                       const char *addr,
                       mrkapp_tcp_server_cb_t cb,
                       void *udata)
{
    char *c;

    srv->thread = NULL;
    srv->cb = cb;
    srv->udata = udata;
    srv->listen_backlog = listen_backlog;
    if ((c = strchr(addr, ':')) != NULL) {
        size_t sz;

        sz = c - addr;
        if ((srv->hostname = malloc(sz + 1)) == NULL) {
            FAIL("malloc");
        }
        memcpy(srv->hostname, addr, sz);
        srv->hostname[sz] = '\0';
        srv->servname = strdup(c + 1);
        srv->family = PF_INET;
    } else {
        srv->hostname = strdup(addr);
        srv->servname = NULL;
        srv->family = PF_LOCAL;
    }
    srv->socktype = SOCK_STREAM;
    srv->fd = -1;
    srv->shutting_down = false;
}


void
mrkapp_tcp_server_fini(mrkapp_tcp_server_t *srv)
{
    srv->shutting_down = true;
    if (srv->thread != NULL) {
        mrkthr_set_interrupt_and_join(srv->thread);
        srv->thread = NULL;
    }
    if (srv->hostname != NULL) {
        free(srv->hostname);
        srv->hostname = NULL;
    }
    if (srv->servname != NULL) {
        free(srv->servname);
        srv->servname = NULL;
    }
    srv->fd = -1;
}


static int
mrkapp_tcp_server_worker(int argc, void **argv)
{
    int res;
    mrkapp_tcp_server_t *srv;

    assert(argc == 1);
    srv = argv[0];

    res = 0;
    while (!srv->shutting_down) {
        mrkthr_socket_t *sockets;
        off_t sz, i;

        sockets = NULL;
        sz = 0;
        if ((res = mrkthr_accept_all2(srv->fd, &sockets, &sz)) != 0) {
            break;
        }
        for (i = 0; i < sz; ++i) {
            mrkthr_socket_t *psock;

            psock = sockets + i;

            if ((res = srv->cb(srv, psock, srv->udata)) != 0) {
                break;
            }
        }

        if (sockets != NULL) {
            free(sockets);
            sockets = NULL;
        }
    }

    MRKTHRET(res);
}


int
mrkapp_tcp_server_start(mrkapp_tcp_server_t *srv)
{
    if (srv->family == PF_INET) {
        if ((srv->fd = mrkthr_socket_bind(srv->hostname,
                                          srv->servname,
                                          srv->family)) < 0) {
            TRRET(MRKAPP_TCP_SERVER_START + 1);
        }

    } else {
        struct stat sb;
        struct sockaddr_un sa;
        size_t sz;

        assert(srv->family = PF_LOCAL);


        if (stat(srv->hostname, &sb) == 0) {
            if (S_ISSOCK(sb.st_mode)) {
                CTRACE("unlinking: %s", srv->hostname);
                (void)unlink(srv->hostname);
            }
        }

        memset(&sa, '\0', sizeof(struct sockaddr_un));

        sz = MIN(strlen(srv->hostname) + 1, sizeof(sa.sun_path));
        sa.sun_family = srv->family;
        strncpy(sa.sun_path, srv->hostname, sz);
        sa.sun_path[sz] = '\0';
#ifdef HAVE_SUN_LEN
        sa.sun_len = strlen(sa.sun_path);
#endif

        if ((srv->fd = socket(srv->family, srv->socktype, 0)) < 0) {
            TRRET(MRKAPP_TCP_SERVER_START + 2);
        }

        if (fcntl(srv->fd, F_SETFL, O_NONBLOCK) == -1) {
            perror("fcntl");
            close(srv->fd);
            srv->fd = -1;
            TRRET(MRKAPP_TCP_SERVER_START + 3);
        }

        if (bind(srv->fd, (struct sockaddr *)&sa, SUN_LEN(&sa)) != 0) {
            perror("bind");
            close(srv->fd);
            srv->fd = -1;
            TRRET(MRKAPP_TCP_SERVER_START + 4);
        }
    }


    if (listen(srv->fd, srv->listen_backlog) != 0) {
        perror("listen");
        close(srv->fd);
        srv->fd = -1;
        TRRET(MRKAPP_TCP_SERVER_START + 5);
    }

    srv->thread = mrkthr_spawn(srv->hostname,
                               mrkapp_tcp_server_worker,
                               1,
                               srv);

    return 0;
}


int
mrkapp_tcp_server_serve(mrkapp_tcp_server_t *srv)
{
    assert(srv->thread != NULL);
    return mrkthr_join(srv->thread);
}

void
mrkapp_tcp_server_stop(mrkapp_tcp_server_t *srv)
{
    srv->shutting_down = true;
    if (srv->thread != NULL) {
        mrkthr_set_interrupt_and_join(srv->thread);
        srv->thread  = NULL;
    }
}
