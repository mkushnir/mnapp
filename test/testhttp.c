#include <assert.h>
#include <stdlib.h>
#include <time.h>

#include <mncommon/unittest.h>
#define TRRET_DEBUG
#include <mncommon/dumpm.h>
#include <mncommon/util.h>
#include <mnapp.h>
#include <mnhttp.h>

#include "diag.h"

#ifndef NDEBUG
const char *_malloc_options = "AJ";
#endif


static int
myline(mnhttp_ctx_t *ctx, UNUSED mnbytestream_t *bs, UNUSED void *udata)
{
    CTRACE("method=%d request uri=%s", ctx->code.method, ctx->request_uri);
    return 0;
}


static int
mybody(mnhttp_ctx_t *ctx, mnbytestream_t *bs, UNUSED void *udata)
{
    size_t sz;

    sz = ctx->current_chunk.end - ctx->current_chunk.start;
    D8(SDATA(bs, ctx->current_chunk.start), sz);
    return 0;
}


static int
myheader(mnhttp_ctx_t *ctx, mnbytestream_t *bs, UNUSED void *udata)
{
    size_t sz;

    sz = ctx->current_header_name.end - ctx->current_header_name.start;
    D8(SDATA(bs, ctx->current_header_name.start), sz);
    sz = ctx->current_header_value.end - ctx->current_header_value.start;
    D8(SDATA(bs, ctx->current_header_value.start), sz);
    return 0;
}


static int
_mycb(UNUSED int argc, void **argv)
{
    int res;
    mnapp_tcp_server_t *srv;
    int fd;
    UNUSED void *udata;
    mnbytestream_t in, out;
    mnhttp_ctx_t *ctx;

    assert(argc == 3);

    srv = argv[0];
    fd = (int)(intptr_t)argv[1];
    udata = argv[2];

    if (srv->shutting_down) {
        return 1;
    }

    bytestream_init(&in, 1024);
    bytestream_init(&out, 1024);
    in.read_more = mnthr_bytestream_read_more;
    ctx = http_ctx_new();
    in.udata = ctx;
    out.write = mnthr_bytestream_write;

    res = http_parse_request((void *)(intptr_t)fd, &in, myline, myheader, mybody, NULL);
    CTRACE("http_parse_request() res=%d", res);
    (void)http_start_response(&out, 200, "OKK");
    (void)http_end_of_header(&out);
    (void)http_add_body(&out, "test\n", 5);

    res = bytestream_produce_data(&out, (void *)(intptr_t)fd);
    CTRACE("bytesteam_produce_data() res=%d", res);

    bytestream_fini(&in);
    bytestream_fini(&out);
    close(fd);
    http_ctx_destroy(&ctx);
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


int
main(int argc, char **argv)
{
    if (argc != 2) {
        FAIL("main");
    }
    mnthr_init();
    (void)MNTHR_SPAWN("test1", test1, argv[1]);
    mnthr_loop();
    mnthr_fini();
    return 0;
}
