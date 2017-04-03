#include <assert.h>
#include <stdbool.h>

#include <openssl/ssl.h>

#include <mrkcommon/dumpm.h>
#include <mrkcommon/util.h>

#include <mnhttpc.h>

#include "diag.h"

static mnbytes_t _qwe = BYTES_INITIALIZER("X-Qwe");
static mnbytes_t _asd = BYTES_INITIALIZER("X-Asd");
UNUSED static mnbytes_t _connection = BYTES_INITIALIZER("connection");
UNUSED static mnbytes_t _close = BYTES_INITIALIZER("close");
static mnbytes_t _accept = BYTES_INITIALIZER("accept");
static mnbytes_t _star_dash_star = BYTES_INITIALIZER("*/*");
UNUSED static mnbytes_t _content_type = BYTES_INITIALIZER("content-type");

static int
in_body_cb_ignore(UNUSED mnhttp_ctx_t *ctx,
                  UNUSED mnbytestream_t *bs,
                  UNUSED mnhttpc_request_t *req)
{
    UNUSED mnbytes_t *ct;

    //http_ctx_dump(ctx);
    //CTRACE("current_chunk size %ld - %ld = %ld",
    //       ctx->current_chunk.end,
    //       ctx->current_chunk.start,
    //       ctx->current_chunk.end - ctx->current_chunk.start);

    //D16(SDATA(bs, ctx->current_chunk.start),
    //    ctx->current_chunk.end - ctx->current_chunk.start);
    if (mnhttp_ctx_last_chunk(ctx)) {
        CTRACE("received %d bytes of body", ctx->bodysz);
    }
    return 0;
}


static void
run_uri(mnhttpc_t *cli, mnbytes_t *uri)
{
    mnhttpc_request_t *req;
    mnhash_item_t *hit;
    mnhash_iter_t it;

    if ((req = mnhttpc_get_new(cli, uri, in_body_cb_ignore)) == NULL) {
        FAIL("mnhttpc_get_new");
    }
    mnhttpc_request_out_field_addf(req, &_qwe, "v:%d", 123);
    mnhttpc_request_out_field_addf(req, &_asd, "v:%d", 234);
    mnhttpc_request_out_field_addf(req, &_qwe, "v:%d", 345);
    mnhttpc_request_out_field_addf(req, &_qwe, "v:%d", 678);
    mnhttpc_request_out_field_addb(req, &_accept, &_star_dash_star);
    //mnhttpc_request_out_field_addb(req, &_connection, &_close);
    if (mnhttpc_request_finalize(req) != 0) {
        goto end;
    }
    bytestream_rewind(&req->connection->in);

    for (hit = hash_first(&req->response.in.headers, &it);
         hit != NULL;
         hit = hash_next(&req->response.in.headers, &it)) {
        UNUSED mnbytes_t *name, *value;
        name = hit->key;
        value = hit->value;
        //CTRACE("in headers %s = %s", BDATA(name), BDATA(value));
    }


end:
    mnhttpc_request_destroy(&req);
}


UNUSED static int
run1(UNUSED int argc, UNUSED void **argv)
{
    int i;
    mnhttpc_t cli;
    BYTES_ALLOCA(uri1, "http://example.org/");
    BYTES_ALLOCA(uri2, "https://example.org/");

    mnhttpc_init(&cli);

    for (i = 0; i < 5; ++i) {
        run_uri(&cli, uri1);
        run_uri(&cli, uri2);
        if (mrkthr_sleep(2000) != 0) {
            break;
        }
    }

    mnhttpc_fini(&cli);
    return 0;

}

UNUSED static int
run0(UNUSED int argc, UNUSED void **argv)
{
    mnhttpc_t cli;

    mnhttpc_init(&cli);

    int n = 30;
    uint64_t ts = 120000;
    while (true) {
        BYTES_ALLOCA(uri0,
                "http://www.fileformat.info/format//tiff/sample/"
                "c44cf1326c2240d38e9fca073bd7a805/text.htm");
        BYTES_ALLOCA(uri1, "http://10.1.1.10/100m.dat");
        BYTES_ALLOCA(uri2, "http://example.org/");

        run_uri(&cli, uri0);
        run_uri(&cli, uri1);
        run_uri(&cli, uri2);

        if (mrkthr_sleep(5000) != 0) {
            break;
        }
        mnhttpc_gc(&cli);
        if (mrkthr_sleep(ts) != 0) {
            break;
        }
        ts *= 2;
        if (--n == 0) {
            break;
        }
    }

    mnhttpc_fini(&cli);
    return 0;
}


int
main(UNUSED int argc, UNUSED char **argv)
{
    SSL_load_error_strings();
    SSL_library_init();
    mrkthr_init();
    //MRKTHR_SPAWN("run0", run0);
    MRKTHR_SPAWN("run1", run1);
    mrkthr_loop();
    mrkthr_fini();
}
