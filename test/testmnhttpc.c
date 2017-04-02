#include <assert.h>
#include <stdbool.h>
#include <mrkcommon/dumpm.h>
#include <mrkcommon/util.h>

#include <mnhttpc.h>

#include "diag.h"

mnbytes_t _qwe = BYTES_INITIALIZER("X-Qwe");
mnbytes_t _asd = BYTES_INITIALIZER("X-Asd");
mnbytes_t _connection = BYTES_INITIALIZER("connection");
mnbytes_t _close = BYTES_INITIALIZER("close");
mnbytes_t _accept = BYTES_INITIALIZER("accept");
mnbytes_t _star_dash_star = BYTES_INITIALIZER("*/*");
mnbytes_t _content_type = BYTES_INITIALIZER("content-type");

static int
in_body_cb_ignore(UNUSED mnhttp_ctx_t *ctx,
                  UNUSED mnbytestream_t *bs,
                  UNUSED mnhttpc_request_t *req)
{
    mnbytes_t *ct;

    if ((ct = mnhttpc_request_in_header(req, &_content_type)) != NULL) {
        CTRACE("content type is %s", BDATA(ct));
    }
    CTRACE("body size %ld - %ld = %ld",
           ctx->body.end,
           ctx->body.start,
           ctx->body.end - ctx->body.start);
    CTRACE("current_chunk size %ld - %ld = %ld",
           ctx->current_chunk.end,
           ctx->current_chunk.start,
           ctx->current_chunk.end - ctx->current_chunk.start);
    //D16(SDATA(bs, ctx->current_chunk.start),
    //    ctx->current_chunk.end - ctx->current_chunk.start);
    return 0;
}


static int
run0(UNUSED int argc, UNUSED void **argv)
{
    mnhttpc_t cli;

    mnhttpc_init(&cli);

    int n = 3;
    while (true) {
        int res;
        mnhttpc_request_t *req;
        BYTES_ALLOCA(uri, "http://example.org/");
        mnhash_item_t *hit;
        mnhash_iter_t it;

        if ((req = mnhttpc_get_new(&cli, uri, in_body_cb_ignore)) == NULL) {
            FAIL("mnhttpc_get_new");
        }
        mnhttpc_request_out_field_addf(req, &_qwe, "v:%d", 123);
        mnhttpc_request_out_field_addf(req, &_asd, "v:%d", 234);
        mnhttpc_request_out_field_addf(req, &_qwe, "v:%d", 345);
        mnhttpc_request_out_field_addf(req, &_qwe, "v:%d", 678);
        mnhttpc_request_out_field_addb(req, &_accept, &_star_dash_star);
        //mnhttpc_request_out_field_addb(req, &_connection, &_close);
        if ((res = mnhttpc_request_finalize(req)) != 0) {
            break;
        }
        bytestream_rewind(&req->connection->in);

        for (hit = hash_first(&req->response.in.headers, &it);
             hit != NULL;
             hit = hash_next(&req->response.in.headers, &it)) {
            mnbytes_t *name, *value;
            name = hit->key;
            value = hit->value;
            //CTRACE("in headers %s = %s", BDATA(name), BDATA(value));
        }

        if (mrkthr_sleep(1000) != 0) {
            break;
        }
        mnhttpc_request_destroy(&req);
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
    mrkthr_init();
    MRKTHR_SPAWN("run0", run0);
    mrkthr_loop();
    mrkthr_fini();
}
