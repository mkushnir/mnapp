#include <stdbool.h>
#include <time.h> /* time_t */

//#define TRRET_DEBUG
#include <mncommon/dumpm.h>
#include <mncommon/fasthash.h>
#include <mncommon/util.h>

#include "bytestream_ssl_helper.h"
#include <mnhttpc.h>

#include "diag.h"
#include "config.h" /* PACKAGE, VERSION */


static int
mnhttpc_request_header_item_fini(void *k, void *v)
{
    mnbytes_t *name = k, *value = v;
    BYTES_DECREF(&name);
    BYTES_DECREF(&value);
    return 0;
}


static uint64_t
_bytes_hash (void const *o)
{
    mnbytes_t const *b = o;
    return bytes_hash(b);
}


static int
_bytes_cmp (void const *oa, void const *ob)
{
    mnbytes_t const *a = oa, *b = ob;
    return bytes_cmp(a, b);
}

static void
mnhttpc_message_init_out(mnhttpc_message_t *msg)
{
    msg->out.method = NULL;
    mnhttp_uri_init(&msg->out.uri);
    msg->out.content_type = NULL;
    hash_init(&msg->out.headers, 17,
              _bytes_hash,
              _bytes_cmp,
              mnhttpc_request_header_item_fini);
    msg->out.content_length = 0;
    msg->out.flags.keepalive = -1;
}


static void
mnhttpc_message_fini_out(mnhttpc_message_t *msg)
{
    msg->out.method = NULL;
    mnhttp_uri_fini(&msg->out.uri);
    BYTES_DECREF(&msg->out.content_type);
    hash_fini(&msg->out.headers);
    msg->out.content_length = 0;
    msg->out.flags.keepalive = -1;
}


static void
mnhttpc_message_init_in(mnhttpc_message_t *msg)
{
    http_ctx_init(&msg->in.ctx);
    msg->in.content_type = NULL;
    hash_init(&msg->in.headers, 17,
              _bytes_hash,
              _bytes_cmp,
              mnhttpc_request_header_item_fini);
    msg->in.flags.keepalive = -1;
}


static void
mnhttpc_message_fini_in(mnhttpc_message_t *msg)
{
    http_ctx_fini(&msg->in.ctx);
    BYTES_DECREF(&msg->in.content_type);
    hash_fini(&msg->in.headers);
    msg->in.flags.keepalive = -1;
}


static void
mnhttpc_request_init(mnhttpc_request_t *req)
{
    STQUEUE_ENTRY_INIT(link, req);
    req->connection = NULL;
    MNTHR_SIGNAL_INIT(&req->recv_signal);
    req->out_body_cb = NULL;
    req->out_body_cb_udata = NULL;
    req->in_body_cb = NULL;
    mnhttpc_message_init_out(&req->request);
    mnhttpc_message_init_in(&req->response);
}


static mnhttpc_request_t *
mnhttpc_request_new(void)
{
    mnhttpc_request_t *req;

    if (MNUNLIKELY((req = malloc(sizeof(mnhttpc_request_t))) == NULL)) {
        FAIL("malloc");
    }
    mnhttpc_request_init(req);

    return req;
}


static void
mnhttpc_request_fini(mnhttpc_request_t *req)
{
    mnhttpc_message_fini_out(&req->request);
    mnhttpc_message_fini_in(&req->response);
    req->connection = NULL;
    req->out_body_cb = NULL;
    req->out_body_cb_udata = NULL;
    req->in_body_cb = NULL;
    STQUEUE_ENTRY_FINI(link, req);
}


void
mnhttpc_request_destroy(mnhttpc_request_t **req)
{
    if (*req != NULL) {
        mnhttpc_request_fini(*req);
        free(*req);
        *req = NULL;
    }
}


int
mnhttpc_request_out_field_addb(mnhttpc_request_t *req,
                               mnbytes_t *name,
                               mnbytes_t *value)
{
    assert(name != NULL);
    assert(value != NULL);

    hash_set_item(&req->request.out.headers, name, value);
    BYTES_INCREF(name);
    BYTES_INCREF(value);
    return 0;
}


int
mnhttpc_request_out_field_addt(mnhttpc_request_t *req,
                               mnbytes_t *name,
                               time_t t)
{
    mnbytes_t *value;
    size_t n;
    char buf[64];
    struct tm *tv;

    assert(name != NULL);

    tv = gmtime(&t);
    n = strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT",
                 tv);
    value = bytes_new_from_str_len(buf, n);

    hash_set_item(&req->request.out.headers, name, value);
    BYTES_INCREF(name);
    BYTES_INCREF(value);
    return 0;
}


int PRINTFLIKE(3, 4)
mnhttpc_request_out_field_addf(mnhttpc_request_t *req,
                               mnbytes_t *name,
                               const char *fmt,
                           ...)
{
    mnbytes_t *value;
    assert(name != NULL);
    va_list ap;

    va_start(ap, fmt);
    value = bytes_vprintf(fmt, ap);
    va_end(ap);

    hash_set_item(&req->request.out.headers, name, value);
    BYTES_INCREF(name);
    BYTES_INCREF(value);
    return 0;
}


mnbytes_t *
mnhttpc_request_out_header(mnhttpc_request_t *req, mnbytes_t *name)
{
    mnhash_item_t *hit;
    mnbytes_t *value;
    value = NULL;
    if ((hit = hash_get_item(&req->request.out.headers, name)) != NULL) {
        value = hit->value;
    }
    return value;
}


mnbytes_t *
mnhttpc_request_in_header(mnhttpc_request_t *req, mnbytes_t *name)
{
    mnhash_item_t *hit;
    mnbytes_t *value;
    value = NULL;
    if ((hit = hash_get_item(&req->response.in.headers, name)) != NULL) {
        value = hit->value;
    }
    return value;
}


static uint64_t
mnhttpc_connection_hash(void const *o)
{
    mnhttpc_connection_t *conn = (mnhttpc_connection_t *)o;
    if (conn->hash == 0) {
        if (conn->proxy_host != NULL) {
            conn->hash = bytes_hash(conn->proxy_host);
        }

        if (conn->proxy_port != NULL) {
            union {
                uint64_t i;
                unsigned char c;
            } u;
            u.i = bytes_hash(conn->proxy_port);
            conn->hash = fasthash(conn->hash, &u.c, sizeof(uint64_t));
        }

        if (conn->host != NULL) {
            union {
                uint64_t i;
                unsigned char c;
            } u;
            u.i = bytes_hash(conn->host);
            conn->hash = fasthash(conn->hash, &u.c, sizeof(uint64_t));
        }

        if (conn->port != NULL) {
            union {
                uint64_t i;
                unsigned char c;
            } u;

            u.i = bytes_hash(conn->port);
            conn->hash = fasthash(conn->hash, &u.c, sizeof(uint64_t));
        }
    }

    return conn->hash;
}


static int
mnhttpc_connection_cmp(void const *oa, void const *ob)
{
    mnhttpc_connection_t *a = (mnhttpc_connection_t *)oa, *b = (mnhttpc_connection_t *)ob;
    uint64_t ha, hb;

    ha = mnhttpc_connection_hash(a);
    hb = mnhttpc_connection_hash(b);
    return MNCMP(ha, hb);
}


static void
mnhttpc_connection_close(mnhttpc_connection_t *conn)
{
    if (conn->send_thread != NULL) {
        mnthr_set_interrupt_and_join(conn->send_thread);
        conn->send_thread = NULL;
    }

    if (conn->recv_thread != NULL) {
        mnthr_set_interrupt_and_join(conn->recv_thread);
        conn->recv_thread = NULL;
    }

    if (conn->ssl != NULL) {
        SSL_shutdown(conn->ssl);
        SSL_free(conn->ssl);
        conn->ssl = NULL;
    }
    if (conn->fd != -1) {
        close(conn->fd);
        conn->fd = -1;
    }
    conn->fp = (void *)(-1);
}


static void
mnhttpc_connection_fini(mnhttpc_connection_t *conn)
{
    mnhttpc_request_t *req;

    conn->hash = 0;
    BYTES_DECREF(&conn->proxy_host);
    BYTES_DECREF(&conn->proxy_port);
    BYTES_DECREF(&conn->host);
    BYTES_DECREF(&conn->port);

    while ((req = STQUEUE_HEAD(&conn->requests)) != NULL) {
        STQUEUE_DEQUEUE(&conn->requests, link);
        STQUEUE_ENTRY_FINI(link, req);
        mnhttpc_request_destroy(&req);
    }

    mnhttpc_connection_close(conn);

    if (conn->sslctx != NULL) {
        SSL_CTX_free(conn->sslctx);
        conn->sslctx = NULL;
    }
    mnthr_sema_fini(&conn->reqfin_sema);

    bytestream_fini(&conn->in);
    bytestream_fini(&conn->out);
}


static void
mnhttpc_connection_destroy(mnhttpc_connection_t **conn)
{
    if (*conn != NULL) {
        mnhttpc_connection_fini(*conn);
        free(*conn);
        *conn = NULL;
    }
}


static int
mnhttpc_connection_item_fini(void *k, UNUSED void *value)
{
    mnhttpc_connection_t *conn = k;
    mnhttpc_connection_destroy(&conn);
    return 0;
}

static void
mnhttpc_connection_init(mnhttpc_connection_t *conn, int scheme)
{
    conn->hash = 0;
    conn->proxy_host = NULL;
    conn->proxy_port = NULL;
    conn->host = NULL;
    conn->port = NULL;
    conn->fd = -1;
    conn->fp = (void *)(-1);
    conn->send_thread = NULL;
    conn->recv_thread = NULL;
    mnthr_sema_init(&conn->reqfin_sema, 1);
    if (MNUNLIKELY(bytestream_init(&conn->in, 4096) != 0)) {
        FAIL("bytestream_init");
    }
    MNTHR_SIGNAL_INIT(&conn->send_signal);
    if (MNUNLIKELY(bytestream_init(&conn->out, 4096) != 0)) {
        FAIL("bytestream_init");
    }

    if (scheme == MNHTTPC_MESSAGE_SCHEME_HTTPS) {
        if (MNUNLIKELY((conn->sslctx = SSL_CTX_new(SSLv23_client_method())) == NULL)) {
            FAIL("SSL_CTX_new");
        }
        conn->ssl = NULL;
        conn->fp = NULL;
        conn->in.read_more = bytestream_ssl_recv_more;
        conn->out.write = bytestream_ssl_send;
    } else {
        conn->sslctx = NULL;
        conn->ssl = NULL;
        conn->in.read_more = mnthr_bytestream_read_more;
        conn->out.write = mnthr_bytestream_write;
    }

    STQUEUE_INIT(&conn->requests);
}


static mnhttpc_connection_t *
mnhttpc_connection_new(const mnbytes_t *proxy_host,
                       const mnbytes_t *proxy_port,
                       const mnbytes_t *host,
                       const mnbytes_t *port,
                       int scheme)
{
    mnhttpc_connection_t *conn;

    if (MNUNLIKELY((conn = malloc(sizeof(mnhttpc_connection_t))) == NULL)) {
        FAIL("malloc");
    }

    mnhttpc_connection_init(conn, scheme);
    if (proxy_host != NULL) {
        conn->proxy_host = bytes_new_from_bytes(proxy_host);
        BYTES_INCREF(conn->proxy_host);
    }
    if (proxy_port != NULL) {
        conn->proxy_port = bytes_new_from_bytes(proxy_port);
        BYTES_INCREF(conn->proxy_port);
    }
    conn->host = bytes_new_from_bytes(host);
    BYTES_INCREF(conn->host);
    conn->port = bytes_new_from_bytes(port);
    BYTES_INCREF(conn->port);
    return conn;
}


static int
mnhttpc_connection_send_worker(UNUSED int argc, void **argv)
{
    mnhttpc_connection_t *conn;

    assert(argc == 1);
    conn = argv[0];

    while (true) {
        //D16(SDATA(&conn->out, 0), SEOD(&conn->out));
        if (SPOS(&conn->out) < 0) {
            CTRACE("conn=%p out=%p produce %ld->%ld (%ld)",
                   conn,
                   &conn->out,
                   (long)SPOS(&conn->out),
                   (long)SEOD(&conn->out),
                   (long)(SEOD(&conn->out) - SPOS(&conn->out)));
            mnthr_dump(mnthr_me());
            D32(conn, sizeof(*conn));
        }
        if (bytestream_produce_data(&conn->out, conn->fp) != 0) {
            break;
        }

        bytestream_rewind(&conn->out);

        if (mnthr_signal_subscribe(&conn->send_signal) != 0) {
            break;
        }

    }

#ifdef TRRET_DEBUG
    CTRACE("Exiting ...");
#endif
    mnthr_signal_fini(&conn->send_signal);
    if (conn->send_thread != NULL) {
        mnthr_decabac(conn->send_thread);
        conn->send_thread = NULL;
    }

    return 0;
}


static int
mnhttpc_response_line_cb(UNUSED mnhttp_ctx_t *ctx,
                         UNUSED mnbytestream_t *bs,
                         void *udata)
{
    UNUSED mnhttpc_request_t *req = udata;
    //CTRACE("version %d./%d code %d", ctx->version_major, ctx->version_minor, ctx->code.status);
    return 0;
}


static int
mnhttpc_response_header_cb(UNUSED mnhttp_ctx_t *ctx,
                           UNUSED mnbytestream_t *bs,
                           void *udata)
{
    mnhttpc_request_t *req = udata;
    mnbytes_t *name, *value;
    mnhash_item_t *hit;

    name = bytes_new_from_str(SDATA(bs, ctx->current_header_name.start));
    name = bytes_set_lower(name);

    /*
     * first found wins
     */
    if ((hit = hash_get_item(&req->response.in.headers, name)) != NULL) {
        BYTES_DECREF(&name);

    } else {
        BYTES_INCREF(name);
        value = bytes_new_from_str(SDATA(bs, ctx->current_header_value.start));
        BYTES_INCREF(value);
        hash_set_item(&req->response.in.headers, name, value);
    }


    //CTRACE("current header %s - %s",
    //       SDATA(bs, ctx->current_header_name.start),
    //       SDATA(bs, ctx->current_header_value.start));
    return 0;
}


UNUSED static int
mnhttpc_response_in_body_cb_wrap(mnhttp_ctx_t *ctx,
                                 mnbytestream_t *bs,
                                 mnhttpc_request_t *req)
{
    int res;

    res = 0;
    if (req->in_body_cb != NULL) {
        if (ctx->chunk_parser_state == PS_CHUNK_DATA) {
            if (ctx->current_chunk_size == 0) {
                /* last chunk */
            } else {
            }
            res = req->in_body_cb(ctx, bs, req);
        } else {
            res = req->in_body_cb(ctx, bs, req);
        }
    }

    return res;
}


static int
mnhttpc_connection_recv_worker(UNUSED int argc, void **argv)
{
    mnhttpc_connection_t *conn;

    assert(argc == 1);
    conn = argv[0];

    while (true) {
        int res;
        mnhttpc_request_t *req;

        if (mnthr_wait_for_read(conn->fd) != 0) {
            break;
        }
        if ((req = STQUEUE_HEAD(&conn->requests)) == NULL) {
            mnhttp_ctx_t ctx;

            http_ctx_init(&ctx);
            //CTRACE("no request ready");
            conn->in.udata = &ctx;
            res = http_parse_response(conn->fp,
                                      &conn->in,
                                      mnhttpc_response_line_cb,
                                      mnhttpc_response_header_cb,
                                      NULL,
                                      NULL);
            conn->in.udata = NULL;
            http_ctx_fini(&ctx);
            if (res != 0) {
                break;
            };

        } else {
            STQUEUE_DEQUEUE(&conn->requests, link);
            STQUEUE_ENTRY_FINI(link, req);

            conn->in.udata = &req->response.in.ctx;
            res = http_parse_response(conn->fp,
                                      &conn->in,
                                      mnhttpc_response_line_cb,
                                      mnhttpc_response_header_cb,
                                      //mnhttpc_response_in_body_cb_wrap,
                                      (mnhttp_cb_t)req->in_body_cb,
                                      req);
            conn->in.udata = NULL;
            mnthr_signal_send(&req->recv_signal);

            if (res != 0) {
                break;
            }
        }

    }

#ifdef TRRET_DEBUG
    CTRACE("Exiting ...");
#endif
    if (conn->recv_thread != NULL) {
        mnthr_decabac(conn->recv_thread);
        conn->recv_thread = NULL;
    }
    return 0;
}


#define MNHTTPC_CONNECTION_SSL_ERROR (-0x8000)
static int
mnhttpc_connection_ssl_init(UNUSED int argc, void **argv)
{
    int res;
    mnhttpc_connection_t *conn;

    assert(argc == 1);
    conn = argv[0];

    res = 0;
    if (conn->sslctx != NULL) {
        if (MNUNLIKELY((conn->ssl = SSL_new(conn->sslctx)) == NULL)) {
            FAIL("SSL_new");
        }
        /* reset conn->fp to ssl */
        conn->fp = conn->ssl;
        if (MNUNLIKELY(SSL_set_fd(conn->ssl, conn->fd) != 1)) {
            FAIL("SSL_set_fd");
        }

        while (true) {
            res = SSL_connect(conn->ssl);
            if (res > 0) {
                res = 0;
                break;

            } else {
                res = SSL_get_error(conn->ssl, res);
                switch (res) {
                case SSL_ERROR_WANT_READ:
                    if ((res = mnthr_wait_for_read(conn->fd)) != 0) {
                        goto end;
                    }
                    break;

                case SSL_ERROR_WANT_WRITE:
                    if ((res = mnthr_wait_for_write(conn->fd)) != 0) {
                        goto end;
                    }
                    break;

                default:
#ifdef TRRET_DEBUG
                    CTRACE("ssl error %d", SSL_get_error(conn->ssl, res));
#endif
                    res = MNHTTPC_CONNECTION_SSL_ERROR;
                    goto end;
                }
            }
        }
    }

end:
    MNTHRET(res);
}

static int
mnhttpc_connection_connect(mnhttpc_connection_t *conn)
{
    int res;
    mnbytes_t *peer_host;
    mnbytes_t *peer_port;

    assert(conn->host != NULL && conn->port != NULL);

    res = 0;
    if (conn->proxy_host != NULL) {
        peer_host = conn->proxy_host;
    } else {
        peer_host = conn->host;
    }
    if (conn->proxy_port != NULL) {
        peer_port = conn->proxy_port;
    } else {
        peer_port = conn->port;
    }
    if ((conn->fd = mnthr_socket_connect(
                BCDATA(peer_host),
                BCDATA(peer_port),
                AF_UNSPEC)) == -1) {
        res = MNHTTPC_CONNECTION_CONNECT + 1;

    } else {
        mnthr_ctx_t *sslinithread;

        conn->fp = (void *)(intptr_t)conn->fd;
        sslinithread = MNTHR_SPAWN("sslinit", mnhttpc_connection_ssl_init, conn);
        if ((res = mnthr_join(sslinithread)) != 0) {
            goto end;
        }

        conn->send_thread = MNTHR_SPAWN(
                "httpsnd",
                mnhttpc_connection_send_worker, conn);
        mnthr_incabac(conn->send_thread);
        conn->recv_thread = MNTHR_SPAWN(
                "httprcv",
                mnhttpc_connection_recv_worker, conn);
        mnthr_incabac(conn->recv_thread);
    }

end:
    TRRET(res);
}


static int
mnhttpc_connection_ensure_connected(mnhttpc_connection_t *conn)
{
    if (conn->send_thread != NULL && conn->recv_thread != NULL) {
        return 0;
    }
    mnhttpc_connection_close(conn);
    return mnhttpc_connection_connect(conn);
}


static int
mnhttpc_connection_gc(void *key,
                      UNUSED void *value,
                      UNUSED void *udata)
{
    mnhttpc_connection_t *conn = key;
    int res;

    res = 0;
    if (mnthr_sema_acquire(&conn->reqfin_sema) != 0) {
        res = -1;
        goto end;
    }

    bytestream_fini(&conn->in);
    bytestream_fini(&conn->out);

    if (MNUNLIKELY(bytestream_init(&conn->in, 4096) != 0)) {
        FAIL("bytestrem_init");
    }
    conn->in.read_more = mnthr_bytestream_read_more;
    if (MNUNLIKELY(bytestream_init(&conn->out, 4096) != 0)) {
        FAIL("bytestrem_init");
    }
    conn->out.write = mnthr_bytestream_write;

    mnthr_sema_release(&conn->reqfin_sema);

end:
    return res;
}


void
mnhttpc_gc(mnhttpc_t *cli)
{
    (void)hash_traverse(&cli->connections,
                        mnhttpc_connection_gc,
                        NULL);
}


/*
 * mnhttpc_t
 */
void
mnhttpc_init(mnhttpc_t *cli)
{
    hash_init(&cli->connections, 127,
              mnhttpc_connection_hash,
              mnhttpc_connection_cmp,
              mnhttpc_connection_item_fini);
}


void
mnhttpc_fini(mnhttpc_t *cli)
{
    hash_fini(&cli->connections);
}


mnhttpc_request_t *
mnhttpc_new(mnhttpc_t *cli,
            const mnbytes_t *proxy_host,
            const mnbytes_t *proxy_port,
            const mnbytes_t *uri,
            const char *method,
            mnhttpc_request_body_cb_t out_body_cb,
            void *out_body_cb_udata,
            mnhttpc_response_body_cb_t in_body_cb)
{
    mnhttpc_request_t *req;
    mnhash_item_t *hit;
    mnhttpc_connection_t *conn;

    req = mnhttpc_request_new();
    req->request.out.method = method;
    req->out_body_cb = out_body_cb;
    req->out_body_cb_udata = out_body_cb_udata;
    req->in_body_cb = in_body_cb;
    mnhttp_uri_parse(&req->request.out.uri, BCDATA(uri));

    if (bytes_is_null_or_empty(req->request.out.uri.host)) {
        goto err;
    }

    if (bytes_is_null_or_empty(req->request.out.uri.port)) {
        goto err;
    }

    if (bytes_is_null_or_empty(req->request.out.uri.relative)) {
        goto err;
    }

    if (bytes_is_null_or_empty(req->request.out.uri.path)) {
        goto err;
    }

    if ((hit = hash_get_item(&cli->connections,
                             &(const mnhttpc_connection_t){
                                .hash = 0,
                                .proxy_host = (mnbytes_t *)proxy_host,
                                .proxy_port = (mnbytes_t *)proxy_port,
                                .host = req->request.out.uri.host,
                                .port = req->request.out.uri.port,
                             })) == NULL) {
        conn = mnhttpc_connection_new(proxy_host,
                                      proxy_port,
                                      req->request.out.uri.host,
                                      req->request.out.uri.port,
                                      req->request.out.uri.scheme);
        hash_set_item(&cli->connections, conn, NULL);
    } else {
        conn = hit->key;
    }
    assert(bytes_cmp(conn->host, req->request.out.uri.host) == 0);
    assert(bytes_cmp(conn->port, req->request.out.uri.port) == 0);
    req->connection = conn;

end:
    return req;

err:
    mnhttpc_request_destroy(&req);
    goto end;
}


void
mnhttpc_request_out_qterm_addb(mnhttpc_request_t *req,
                               mnbytes_t *name,
                               mnbytes_t *value)
{
    mnhttp_uri_add_qterm(&req->request.out.uri, name, value);
}


int
mnhttpc_request_finalize(mnhttpc_request_t *req)
{
    int res;
    mnhttpc_connection_t *conn;
    mnhash_item_t *hit;
    mnhash_iter_t it;

    conn = req->connection;
    assert(conn != NULL);

    res = 0;

    if (mnthr_sema_acquire(&conn->reqfin_sema) != 0) {
        res = MNHTTPC_REQUEST_FINALIZE + 1;
        goto end1;
    }

    if (mnhttpc_connection_ensure_connected(conn) != 0) {
        res = MNHTTPC_REQUEST_FINALIZE + 2;
        goto end0;
    }

    if ((res = mnhttp_uri_start_request(
                &req->request.out.uri,
                &conn->out,
                req->request.out.method)) == 0) {
    } else {
        TR(res);
        res = MNHTTPC_REQUEST_FINALIZE + 3;
        goto end0;
    }

    (void)http_field_addt(&conn->out, BYTES_REF("Date"), MNTHR_GET_NOW_SEC());
    (void)http_field_addf(&conn->out,
                          BYTES_REF("Host"),
                          "%s:%s",
                          BDATA(req->request.out.uri.host),
                          BDATA(req->request.out.uri.port));
    (void)http_field_addf(&conn->out,
                          BYTES_REF("User-Agent"),
                          "%s/%s",
                          PACKAGE,
                          VERSION);
    for (hit = hash_first(&req->request.out.headers, &it);
         hit != NULL;
         hit = hash_next(&req->request.out.headers, &it)) {
        mnbytes_t *name;
        mnbytes_t *value;

        name = hit->key;
        value = hit->value;
        (void)http_field_addb(&conn->out, name, value);
    }

    (void)http_end_of_header(&conn->out);

    if (req->out_body_cb != NULL) {
        (void)req->out_body_cb(&conn->out, req->out_body_cb_udata);
    }

    STQUEUE_ENQUEUE(&conn->requests, link, req);
    mnthr_signal_send(&conn->send_signal);
    res = mnthr_signal_subscribe(&req->recv_signal);

    //D16(SDATA(&conn->in, 0), SEOD(&conn->in));
    //bytestream_rewind(&conn->in);

end0:
    mnthr_sema_release(&conn->reqfin_sema);

end1:
    return res;
}
