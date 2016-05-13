#ifndef MRKAPP_H_DEFINED
#define MRKAPP_H_DEFINED

#include <stdbool.h>

#include <mrkthr.h>

#ifdef __cplusplus
extern "C" {
#endif


struct _mrkapp_tcp_server;
typedef int (*mrkapp_tcp_server_cb_t)(struct _mrkapp_tcp_server *,
                                      mrkthr_socket_t *,
                                      void *);

typedef struct _mrkapp_tcp_server {
    mrkthr_ctx_t *thread;
    mrkapp_tcp_server_cb_t cb;
    void *udata;
    char *hostname;
    char *servname;
    int listen_backlog;
    int family;
    int socktype;
    int fd;
    bool shutting_down;
} mrkapp_tcp_server_t;

const char *mrkapp_diag_str(int);

void mrkapp_tcp_server_init(mrkapp_tcp_server_t *,
                            int,
                            const char *,
                            mrkapp_tcp_server_cb_t,
                            void *);
void mrkapp_tcp_server_fini(mrkapp_tcp_server_t *);
int mrkapp_tcp_server_start(mrkapp_tcp_server_t *);
int mrkapp_tcp_server_serve(mrkapp_tcp_server_t *);
void mrkapp_tcp_server_stop(mrkapp_tcp_server_t *);

int local_server(int, void **);
void local_server_shutdown(void);
void daemonize(const char *, const char *, const char *);

#ifdef __cplusplus
}
#endif
#endif /* MRKAPP_H_DEFINED */
