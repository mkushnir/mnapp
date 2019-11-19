#ifndef MNHTTPC_H
#define MNHTTPC_H

#include <stdarg.h>
#include <time.h> /* time_t */

#include <openssl/ssl.h>

#include <mrkcommon/bytes.h>
#include <mrkcommon/bytestream.h>
#include <mrkcommon/hash.h>
#include <mrkcommon/stqueue.h>

#include <mnhttp.h>
#include <mrkthr.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef union _mnhttpc_message {
    /* request */
    struct {
        /* MRKHTTP_METHOD_ */
        const char *method; /* weak */
        mnhttp_uri_t uri;
        mnhash_t headers;
        mnbytes_t *content_type;
        size_t content_length;
        struct {
            int keepalive:1;
        } flags;
    } out;

    /* response */
    struct {
        mnhttp_ctx_t ctx;
        mnbytes_t *content_type;
        mnhash_t headers;
        struct {
            int keepalive:1;
        } flags;
    } in;

} mnhttpc_message_t;


struct _mnhttpc_connection;
struct _mnhttpc_request;
typedef ssize_t (*mnhttpc_request_body_cb_t)(mnbytestream_t *, void *);
typedef int (*mnhttpc_response_body_cb_t)(mnhttp_ctx_t *, mnbytestream_t *, struct _mnhttpc_request *);

typedef struct _mnhttpc_request {
    STQUEUE_ENTRY(_mnhttpc_request, link);
    struct _mnhttpc_connection *connection;
    mrkthr_signal_t recv_signal;
    mnhttpc_request_body_cb_t out_body_cb;
    void *out_body_cb_udata;
    mnhttpc_response_body_cb_t in_body_cb;
    mnhttpc_message_t request;
    mnhttpc_message_t response;
} mnhttpc_request_t;


typedef struct _mnhttpc_connection {
    uint64_t hash;
    mnbytes_t *proxy_host;
    mnbytes_t *proxy_port;
    mnbytes_t *host;
    mnbytes_t *port;
    int fd; /* connect socket */
    void *fp; /* connect socket */
    SSL_CTX *sslctx;
    SSL *ssl;

    mrkthr_ctx_t *send_thread;
    mrkthr_sema_t reqfin_sema;
    mrkthr_signal_t send_signal;
    mrkthr_ctx_t *recv_thread;
    mrkthr_cond_t recv_cond;
    mnbytestream_t in;
    mnbytestream_t out;
    STQUEUE(_mnhttpc_request, requests);
} mnhttpc_connection_t;


typedef struct _mnhttpc {
    /*
     * strong mnhttpc_connection_t *, NULL
     */
    mnhash_t connections;

} mnhttpc_t;


void mnhttpc_request_destroy(mnhttpc_request_t **req);
mnhttpc_request_t *mnhttpc_new(mnhttpc_t *,
                               const mnbytes_t *,
                               const mnbytes_t *,
                               const mnbytes_t *,
                               const char *,
                               mnhttpc_request_body_cb_t,
                               void *,
                               mnhttpc_response_body_cb_t);

#define mnhttpc_get_new(cli, proxy_host, proxy_port, uri, in_body_cb)  \
    mnhttpc_new(cli,                                                   \
                proxy_host,                                            \
                proxy_port,                                            \
                uri,                                                   \
                MRKHTTP_METHOD_GET,                                    \
                NULL,                                                  \
                NULL,                                                  \
                in_body_cb)                                            \


#define mnhttpc_post_new(cli,                  \
                         proxy_host,           \
                         proxy_port,           \
                         uri,                  \
                         out_body_cb,          \
                         out_body_cb_udata,    \
                         in_body_cb)           \
    mnhttpc_new(cli,                           \
                proxy_host,                    \
                proxy_port,                    \
                uri,                           \
                MRKHTTP_METHOD_POST,           \
                out_body_cb,                   \
                out_body_cb_udata,             \
                in_body_cb)                    \


int mnhttpc_request_out_field_addb(mnhttpc_request_t *,
                                   mnbytes_t *,
                                   mnbytes_t *);
int mnhttpc_request_out_field_addt(mnhttpc_request_t *,
                                   mnbytes_t *,
                                   time_t);
int PRINTFLIKE(3, 4) mnhttpc_request_out_field_addf(mnhttpc_request_t *,
                                                    mnbytes_t *,
                                                    const char *,
                                                    ...);
void mnhttpc_request_out_qterm_addb(mnhttpc_request_t *, mnbytes_t *, mnbytes_t *);
mnbytes_t *mnhttpc_request_out_header(mnhttpc_request_t *, mnbytes_t *);
mnbytes_t *mnhttpc_request_in_header(mnhttpc_request_t *, mnbytes_t *);
int mnhttpc_request_finalize(mnhttpc_request_t *);


int mnhttpc_get_response(mnhttpc_request_t *);

void mnhttpc_init(mnhttpc_t *);
void mnhttpc_fini(mnhttpc_t *); /* MRKTHR_CPOINT */
void mnhttpc_destroy(mnhttpc_t **); /* MRKTHR_CPOINT */
void mnhttpc_gc(mnhttpc_t *);


#ifdef __cplusplus
}
#endif

#endif
