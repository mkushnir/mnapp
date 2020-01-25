#ifndef MNTCPS_H_DEFINED
#define MNTCPS_H_DEFINED

#include <stdbool.h>

#include <mnthr.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _mnapp_tcp_server;
typedef int (*mnapp_tcp_server_cb_t)(struct _mnapp_tcp_server *,
                                      mnthr_socket_t *,
                                      void *);

typedef struct _mnapp_tcp_server {
    mnthr_ctx_t *thread;
    mnapp_tcp_server_cb_t cb;
    void *udata;
    char *hostname;
    char *servname;
    int listen_backlog;
    int family;
    int socktype;
    int fd;
    bool shutting_down;
} mnapp_tcp_server_t;

const char *mnapp_diag_str(int);

void mnapp_tcp_server_init(mnapp_tcp_server_t *,
                            int,
                            const char *,
                            mnapp_tcp_server_cb_t,
                            void *);
void mnapp_tcp_server_fini(mnapp_tcp_server_t *);
int mnapp_tcp_server_start(mnapp_tcp_server_t *);
int mnapp_tcp_server_serve(mnapp_tcp_server_t *);
void mnapp_tcp_server_stop(mnapp_tcp_server_t *);

int local_server(int, void **);
void local_server_shutdown(void);

#ifdef __cplusplus
}
#endif
#endif /* MNTCPS_H_DEFINED */
