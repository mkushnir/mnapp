#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

//#define TRRET_DEBUG
#include <mrkcommon/dumpm.h>
#include <mrkcommon/bytestream.h>
#include <mrkcommon/util.h>

#include "diag.h"

#include <mrkhttp.h>

#define RFC3986_RESEVED 0x01
#define RFC3986_OTHERS  0x02
#define RFC3986_UNRESERVED  0x04


#define ISSPACE(c) ((c) == ' ' || (c) == '\t')


static char charflags[256] = {
/*  0 1 2 3 4 5 6 7 8 9 a b c d e f 0 1 2 3 4 5 6 7 8 9 a b c d e f */
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,1,2,1,1,2,1,1,1,1,1,1,1,4,4,1,2,2,2,2,2,2,2,2,2,2,1,1,2,1,2,1,
    1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,1,2,1,2,4,
    2,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,2,2,2,4,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
};


char *
http_urlencode_reserved(const char *s, size_t sz)
{
    char *res;
    unsigned char c;
    unsigned int i, j;

    if ((res = malloc(sz * 3 + 1)) == NULL) {
        FAIL("malloc");
    }

    for (i = 0, j = 0; i < sz; ++i, ++j) {
        c = (unsigned char)(s[i]);

        if (!(charflags[c] & RFC3986_UNRESERVED)) {
            unsigned char cc = c >> 4;

            *(res + j++) = '%';
            if (cc < 10) {
                *(res + j++) = '0' + cc;
            } else {
                *(res + j++) = 'A' + cc - 10;
            }
            cc = (c & 0x0f);
            if (cc < 10) {
                *(res + j) = '0' + cc;
            } else {
                *(res + j) = 'A' + cc - 10;
            }

        } else {
            *(res + j) = (char)c;
        }
    }

    *(res + j) = '\0';

    return res;
}


char *
http_urldecode(char *s)
{
    char *src, *dst;
    for (src = dst = s; *src!= '\0'; ++src, ++dst)
    {
        if (*src == '%') {
            ++src;
            if (*src >= '0' && *src <= '9') {
                *dst = (*src - '0') << 4;
            } else if (*src >= 'a' && *src <= 'f') {
                *dst = (*src - 'a' + 10) << 4;
            } else if (*src >= 'A' && *src <= 'F') {
                *dst = (*src - 'A' + 10) << 4;
            } else {
                return NULL;
            }
            ++src;
            if (*src >= '0' && *src <= '9') {
                *dst |= (*src - '0');
            } else if (*src >= 'a' && *src <= 'f') {
                *dst |= (*src - 'a' + 10);
            } else if (*src >= 'A' && *src <= 'F') {
                *dst |= (*src - 'A' + 10);
            } else {
                return NULL;
            }
        } else {
            *dst = *src;
        }
    }

    return dst;
}


void
mrkhttp_uri_init(mrkhttp_uri_t *uri)
{
    uri->scheme = MNHTTPC_MESSAGE_SCHEME_UNDEF;
    uri->user = NULL;
    uri->password = NULL;
    uri->host = NULL;
    uri->port = NULL;
    uri->relative = NULL;
    uri->path = NULL;
    uri->qstring = NULL;
    uri->fragment = NULL;
}


void
mrkhttp_uri_fini(mrkhttp_uri_t *uri)
{
    BYTES_DECREF(&uri->user);
    BYTES_DECREF(&uri->password);
    BYTES_DECREF(&uri->host);
    BYTES_DECREF(&uri->port);
    BYTES_DECREF(&uri->relative);
    BYTES_DECREF(&uri->path);
    BYTES_DECREF(&uri->qstring);
    BYTES_DECREF(&uri->fragment);
}


void
mrkhttp_uri_parse(mrkhttp_uri_t *uri, const char *s)
{
    const char *p0, *p1;
    const char *puser,
               *phost,
               *pport,
               *ppath,
               *pqstring,
               *pfragment;

    p0 = s;
    p1 = s;
    if (strstr(p0, "http://") == p0) {
        uri->scheme = MNHTTPC_MESSAGE_SCHEME_HTTP;
        p0 += 7;
    } else if (strstr(p0, "https://") == p0) {
        uri->scheme = MNHTTPC_MESSAGE_SCHEME_HTTPS;
        p0 += 8;
    } else {
        /*
         * will see port conventions;
         */
    }

    if ((ppath = strchr(p0, '/')) != NULL) {
        assert(uri->relative == NULL);
        uri->relative = bytes_new_from_str(ppath);
        BYTES_INCREF(uri->relative);
        if ((pqstring = strchr(ppath, '?')) != NULL) {
            ++pqstring;
            if ((pfragment = strchr(pqstring, '#')) != NULL) {
                assert(uri->qstring == NULL);
                uri->qstring = bytes_new_from_str_len(
                    pqstring, pfragment - pqstring);
                BYTES_INCREF(uri->qstring);
                assert(uri->fragment == NULL);
                uri->fragment = bytes_new_from_str(pfragment);
                BYTES_INCREF(uri->fragment);
            } else {
                /* no fragment */
                assert(uri->qstring == NULL);
                uri->qstring = bytes_new_from_str(pqstring);
                BYTES_INCREF(uri->qstring);
            }
            assert(uri->path == NULL);
            uri->path = bytes_new_from_str_len(ppath, pqstring - ppath - 1);
            BYTES_INCREF(uri->path);
        } else {
            /* no qstring */
            if ((pfragment = strchr(ppath, '#')) != NULL) {
                assert(uri->fragment == NULL);
                uri->fragment = bytes_new_from_str(pfragment);
                BYTES_INCREF(uri->fragment);
                assert(uri->path == NULL);
                uri->path = bytes_new_from_str_len(ppath, pfragment - ppath);
                BYTES_INCREF(uri->path);
            } else {
                assert(uri->path == NULL);
                uri->path = bytes_new_from_str(ppath);
                BYTES_INCREF(uri->path);
            }
        }

        if ((phost = strchr(p0, '@')) != NULL && phost < ppath) {
            if ((puser = strchr(p0, ':')) != NULL && puser < phost) {
                assert(uri->user == NULL);
                uri->user = bytes_new_from_str_len(p0, puser - p0);
                BYTES_INCREF(uri->user);
                ++puser;
                assert(uri->password == NULL);
                uri->password = bytes_new_from_str_len(puser, phost - puser);
                BYTES_INCREF(uri->password);
            } else {
                assert(uri->user == NULL);
                uri->user = bytes_new_from_str_len(p0, phost - p0);
                BYTES_INCREF(uri->user);
            }
            ++phost;
            if ((pport = strchr(phost, ':')) != NULL && pport < ppath) {
                assert(uri->host == NULL);
                uri->host = bytes_new_from_str_len(phost, pport - phost);
                BYTES_INCREF(uri->host);
                ++pport;
                assert(uri->port == NULL);
                uri->port = bytes_new_from_str_len(pport, ppath - pport);
                BYTES_INCREF(uri->port);
            } else {
                assert(uri->host == NULL);
                uri->host = bytes_new_from_str_len(phost, ppath - phost);
                BYTES_INCREF(uri->host);
            }

        } else {
            if ((pport = strchr(p0, ':')) != NULL && pport < ppath) {
                assert(uri->host == NULL);
                uri->host = bytes_new_from_str_len(p0, pport - p0);
                BYTES_INCREF(uri->host);
                ++pport;
                assert(uri->port == NULL);
                uri->port = bytes_new_from_str_len(pport, ppath - pport);
                BYTES_INCREF(uri->port);
            } else {
                assert(uri->host == NULL);
                uri->host = bytes_new_from_str_len(p0, ppath - p0);
                BYTES_INCREF(uri->host);
            }
        }
    } else {
        /* no path, qstring */
        if ((phost = strchr(p0, '@')) != NULL) {
            if ((puser = strchr(p0, ':')) != NULL && puser < phost) {
                assert(uri->user == NULL);
                uri->user = bytes_new_from_str_len(p0, puser - p0);
                BYTES_INCREF(uri->user);
                ++puser;
                assert(uri->password == NULL);
                uri->password = bytes_new_from_str_len(puser, phost - puser);
                BYTES_INCREF(uri->password);
            } else {
                assert(uri->user == NULL);
                uri->user = bytes_new_from_str_len(p0, phost - p0);
                BYTES_INCREF(uri->user);
            }
            ++phost;
            if ((pport = strchr(phost, ':')) != NULL) {
                assert(uri->host == NULL);
                uri->host = bytes_new_from_str_len(phost, pport - phost);
                BYTES_INCREF(uri->host);
                ++pport;
                assert(uri->port == NULL);
                uri->port = bytes_new_from_str(pport);
                BYTES_INCREF(uri->port);
            } else {
                assert(uri->host == NULL);
                uri->host = bytes_new_from_str(phost);
                BYTES_INCREF(uri->host);
            }

        } else {
            if ((pport = strchr(p0, ':')) != NULL) {
                assert(uri->host == NULL);
                uri->host = bytes_new_from_str_len(p0, pport - p0);
                BYTES_INCREF(uri->host);
                ++pport;
                assert(uri->port == NULL);
                uri->port = bytes_new_from_str(pport);
                BYTES_INCREF(uri->port);
            } else {
                assert(uri->host == NULL);
                uri->host = bytes_new_from_str(p0);
                BYTES_INCREF(uri->host);
            }
        }
    }
    if (bytes_is_null_or_empty(uri->port)) {
        BYTES_DECREF(&uri->port);
        if (uri->scheme == MNHTTPC_MESSAGE_SCHEME_HTTPS) {
            assert(uri->port == NULL);
            uri->port = bytes_new_from_str("443");
            BYTES_INCREF(uri->port);
        } else {
            assert(uri->port == NULL);
            uri->port = bytes_new_from_str("80");
            BYTES_INCREF(uri->port);
        }
    }
    if (uri->scheme == MNHTTPC_MESSAGE_SCHEME_UNDEF) {
        BYTES_ALLOCA(https, "443");
        if (bytes_cmp(uri->port, https) == 0) {
            uri->scheme = MNHTTPC_MESSAGE_SCHEME_HTTPS;
        } else {
            uri->scheme = MNHTTPC_MESSAGE_SCHEME_HTTP;
        }
    }
}


void
http_ctx_init(mnhttp_ctx_t *ctx)
{
    ctx->parser_state = PS_START;
    ctx->chunk_parser_state = 0;
    ctx->first_line.start = 0;
    ctx->first_line.end = 0;
    ctx->version_major = 0;
    ctx->version_minor = 0;
    ctx->code.status = 0;
    ctx->request_uri = NULL;
    ctx->bodysz = -1;
    ctx->current_header_name.start = 0;
    ctx->current_header_name.end = 0;
    ctx->current_header_value.start = 0;
    ctx->current_header_value.end = 0;
    ctx->flags = 0;
    ctx->body.start = 0;
    ctx->body.end = 0;
    ctx->current_chunk_size = 0;
    ctx->current_chunk.start = 0;
    ctx->current_chunk.end = 0;
    ctx->udata = NULL;
}


void
http_ctx_fini(mnhttp_ctx_t *ctx)
{
    if (ctx->request_uri != NULL) {
        free(ctx->request_uri);
        ctx->request_uri = NULL;
    }
    http_ctx_init(ctx);
}


void
http_ctx_dump(const mnhttp_ctx_t *ctx)
{
    TRACE("PS=%s CPS=%s f=%d l=%ld/%ld HTTP/%d.%d %d "
          "hn=%ld/%ld hv=%ld/%ld b=%d,%ld/%ld c=%d,%ld/%ld udata=%p",
          PSSTR(ctx->parser_state),
          CPSSTR(ctx->chunk_parser_state),
          ctx->flags,
          (long)ctx->first_line.start,
          (long)ctx->first_line.end,
          ctx->version_major,
          ctx->version_minor,
          ctx->code.status,
          (long)ctx->current_header_name.start,
          (long)ctx->current_header_name.end,
          (long)ctx->current_header_value.start,
          (long)ctx->current_header_value.end,
          ctx->bodysz,
          (long)ctx->body.start,
          (long)ctx->body.end,
          ctx->current_chunk_size,
          (long)ctx->current_chunk.start,
          (long)ctx->current_chunk.end,
          ctx->udata
         );
}


mnhttp_ctx_t *
http_ctx_new(void)
{
    mnhttp_ctx_t *ctx;

    if ((ctx = malloc(sizeof(mnhttp_ctx_t))) == NULL) {
        FAIL("malloc");
    }
    http_ctx_init(ctx);
    return ctx;
}


void
http_ctx_destroy(mnhttp_ctx_t **ctx)
{
    if (*ctx != NULL) {
        http_ctx_fini(*ctx);
        free(*ctx);
        *ctx = NULL;
    }
}


int
http_start_request(mnbytestream_t *out,
                   const char *method,
                   const char *uri)
{
    size_t sz;

    sz = strlen(method) + strlen(uri) + 16;
    if (bytestream_nprintf(out, sz, "%s %s HTTP/1.1\r\n", method, uri)) {
        TRRET(HTTP_START_REQUEST + 1);
    }
    return 0;
}


int
http_start_response(mnbytestream_t *out,
                    int reason,
                    const char *phrase)
{
    size_t sz;

    sz = strlen(phrase) + 32;
    if (bytestream_nprintf(out, sz, "HTTP/1.1 %d %s\r\n", reason, phrase) < 0) {
        TRRET(HTTP_START_RESPONSE + 1);
    }
    return 0;
}


int
http_add_header_field(mnbytestream_t *out,
                      const char *name,
                      const char *value)
{
    int res = 0;
    size_t sz;

    /* strlen(": \r\n") = 4 */
    sz = strlen(name) + strlen(value) + 4;
    /*
     * Reserve space in out at 4096 byte boundary.
     *
     * Do we really need it? bytestream_nprintf() grows its internal
     * buffer by BLOCKSZ chunks.
     */
    sz += (4096 - (sz % 4096));

    if ((res = bytestream_nprintf(out, sz,
                                  "%s: %s\r\n",
                                  name, value)) < 0) {
        res = HTTP_ADD_HEADER_FIELD + 1;
    } else {
        res = 0;
    }
    TRRET(res);
}


int PRINTFLIKE(3, 4)
http_field_addf(mnbytestream_t *out,
                mnbytes_t *name,
                const char *fmt,
                ...)
{
    int res;
    mnbytes_t *value;
    assert(name != NULL);
    va_list ap;

    va_start(ap, fmt);
    value = bytes_vprintf(fmt, ap);
    va_end(ap);
    res = http_add_header_field(out,
                                (char *)BDATA(name),
                                (char *)BDATA(value));
    BYTES_DECREF(&value);
    return res;
}


int
http_field_addb(mnbytestream_t *out,
                mnbytes_t *name,
                mnbytes_t *value)
{
    assert(name != NULL);
    assert(value != NULL);
    return http_add_header_field(out,
                                 (char *)BDATA(name),
                                 (char *)BDATA(value));
}


int
http_field_addt(mnbytestream_t *out,
                mnbytes_t *name,
                time_t t)
{
    int res;
    mnbytes_t *value;
    size_t n;
    char buf[64];
    struct tm *tv;

    tv = gmtime(&t);
    n = strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT",
                 tv);
    value = bytes_new_from_str_len(buf, n);
    res = http_add_header_field(out,
                                (char *)BDATA(name),
                                (char *)BDATA(value));
    BYTES_DECREF(&value);
    return res;
}



int
http_end_of_header(mnbytestream_t *out)
{
    int res = 0;

    if ((res = bytestream_cat(out, 2, "\r\n")) <= 0) {
        res = HTTP_END_OF_HEADER + 1;
    } else {
        res = 0;
    }
    TRRET(res);
}


int
http_add_body(mnbytestream_t *out, const char *body, size_t sz)
{
    int res = 0;

    if ((res = bytestream_cat(out, sz, body)) < 0) {
        res = HTTP_ADD_BODY + 1;
    } else {
        res = 0;
    }
    TRRET(res);
}




/*
 * Find the CR and LF pair in s, up to sz bytes long.
 */
char *
findcrlf(char *s, int sz)
{
    char *ss;

    for (ss = s + sz; s < ss; ++s) {
        if (*s == '\r') {
            if ((s + 1) < ss && *(s + 1) == '\n') {
                return s;
            }
        }
    }
    return NULL;
}


static char *
findsp(char *s, int sz)
{
    char *ss;

    for (ss = s + sz; s < ss; ++s) {
        if (ISSPACE(*s)) {
            return s;
        }
    }
    return NULL;
}


static char *
findnosp(char *s, int sz)
{
    char *ss;

    for (ss = s + sz; s < ss; ++s) {
        if (!ISSPACE(*s)) {
            return s;
        }
    }
    return NULL;
}


static int
process_header(mnhttp_ctx_t *ctx, mnbytestream_t *in)
{
#ifdef TRRET_DEBUG
    TRACE("current_header_name=%s",
          SDATA(in, ctx->current_header_name.start));
    TRACE("current_header_value=%s",
          SDATA(in, ctx->current_header_value.start));
#endif

    if (strcasecmp(SDATA(in, ctx->current_header_name.start),
                   "content-length") == 0) {

        ctx->bodysz = strtol(SDATA(in, ctx->current_header_value.start),
                            NULL, 10);

    } else if (strcasecmp(SDATA(in, ctx->current_header_name.start),
                          "transfer-encoding") == 0) {

        if (strcmp(SDATA(in, ctx->current_header_value.start),
                   "chunked") == 0) {

            ctx->flags |= PS_FLAG_CHUNKED;
            ctx->chunk_parser_state = PS_CHUNK_SIZE;
            ctx->bodysz = 0;
        }
    }

    return MRKHTTP_PARSE_NEED_MORE;
}


void
recycle_stream_buffer(mnhttp_ctx_t *ctx, mnbytestream_t *in)
{
    off_t recycled;

    ctx->first_line.start = 0;
    ctx->first_line.end = 0;
    ctx->current_header_name.start = 0;
    ctx->current_header_name.end = 0;
    ctx->current_header_value.start = 0;
    ctx->current_header_value.end = 0;
    ctx->body.start = 0;
    ctx->body.end = 0;

    recycled = bytestream_recycle(in, 512, ctx->current_chunk.start);

    ctx->current_chunk.start -= recycled;
    ctx->current_chunk.end -= recycled;
}


static int
process_body(mnhttp_ctx_t *ctx,
             mnbytestream_t *in,
             mnhttp_cb_t body_cb,
             void *udata)
{
    //TRACE("flags=%d body sz=%d start=%ld end=%ld",
    //      ctx->flags, ctx->bodysz, ctx->body.start, ctx->body.end);
    //D16(SDATA(in, ctx->body.start), ctx->body.end - ctx->body.start);

    if (ctx->flags & PS_FLAG_CHUNKED) {
        if (ctx->chunk_parser_state == PS_CHUNK_SIZE) {
            char *end, *tmp = NULL;

            if ((end = findcrlf(SDATA(in, ctx->current_chunk.start),
                                SEOD(in) - ctx->current_chunk.start))
                == NULL) {

                //D8(SDATA(in, ctx->current_chunk.start),
                //   MIN(128, SEOD(in) - ctx->current_chunk.start));

                SADVANCEPOS(in, SEOD(in) - SPOS(in));
                TRRET(MRKHTTP_PARSE_NEED_MORE);
            }
            ctx->current_chunk_size = strtol(SDATA(in,
                        ctx->current_chunk.start), &tmp, 16);
            ctx->bodysz += ctx->current_chunk_size;

            if (tmp != end || ctx->current_chunk_size  < 0) {
                //D32(SDATA(in, ctx->current_chunk.start), 32);
                TRRET(PROCESS_BODY + 1);
            }

            //TRACE("tmp=%p end=%p", tmp, end);
            //TRACE("found chunk %d", ctx->current_chunk_size);

            SADVANCEPOS(in, end - SPDATA(in) + 2);
            ctx->current_chunk.start = SPOS(in);
            ctx->current_chunk.end = ctx->current_chunk.start;
            ctx->chunk_parser_state = PS_CHUNK_DATA;

            TRRET(MRKHTTP_PARSE_NEED_MORE);

        } else if (ctx->chunk_parser_state == PS_CHUNK_DATA) {
            int needed, navail;

            needed = ctx->current_chunk_size -
                     (ctx->current_chunk.end - ctx->current_chunk.start);

            navail = SEOD(in) - SPOS(in);

            if (needed > 0) {
                int incr = MIN(needed, navail);

                //TRACE("chunk incomplete: sz=%d/%ld",
                //      ctx->current_chunk_size,
                //      ctx->current_chunk.end - ctx->current_chunk.start);

                ctx->current_chunk.end += incr;
                SADVANCEPOS(in, incr);

                TRRET(MRKHTTP_PARSE_NEED_MORE);

            } else {
                //TRACE("chunk complete: sz=%d/%ld",
                //      ctx->current_chunk_size,
                //      ctx->current_chunk.end - ctx->current_chunk.start);
                //D32(SDATA(in, ctx->current_chunk.start),
                //    ctx->current_chunk.end - ctx->current_chunk.start);

                //ctx->bodysz += ctx->current_chunk.end -
                //               ctx->current_chunk.start;
                if (body_cb != NULL) {
                    if (body_cb(ctx, in, udata) != 0) {
                        TRRET(PROCESS_BODY + 2);
                    }
                }

                /* prepare to the next chunk */
                ctx->chunk_parser_state = PS_CHUNK_SIZE;
                /* account for \r\n */
                SADVANCEPOS(in, 2);
                ctx->current_chunk.start = SPOS(in);
                ctx->current_chunk.end = ctx->current_chunk.start;

                if (ctx->current_chunk_size > 0) {
                    /* recycle */
                    recycle_stream_buffer(ctx, in);
                    TRRET(MRKHTTP_PARSE_NEED_MORE);
                } else {
                    /* recycle */
                    recycle_stream_buffer(ctx, in);
                    TRRET(0);
                }
            }
        }
    } else {
        ssize_t navail, accumulated, needed, incr;

        navail = SEOD(in) - SPOS(in);
        accumulated = ctx->body.end - ctx->body.start;
        needed = ctx->bodysz - accumulated;
        incr = MIN(navail, needed);

        ctx->body.end += incr;
        ctx->current_chunk.end = ctx->body.end;
        SADVANCEPOS(in, incr);
        accumulated = ctx->body.end - ctx->body.start;

#ifdef TRRET_DEBUG
        TRACE("bodysz=%d navail=%ld accumulated=%ld incr=%ld "
              "SPOS=%ld SEOD=%ld",
              ctx->bodysz,
              navail,
              accumulated,
              incr,
              SPOS(in),
              SEOD(in));
#endif
        //D16(SPDATA(in), navail);

        if (accumulated < ctx->bodysz) {
            TRRET(MRKHTTP_PARSE_NEED_MORE);
        } else {
            if (body_cb != NULL) {
                if (body_cb(ctx, in, udata) != 0) {
                    TRRET(PROCESS_BODY + 3);
                }
            }
            recycle_stream_buffer(ctx, in);
            TRRET(0);
        }
    }

    TRRET(PROCESS_BODY + 4);
}


static int
parse_method(char *s, size_t sz)
{
    if (strncasecmp(s, "GET", sz) == 0) {
        return MRKHTTP_MGET;
    }
    if (strncasecmp(s, "HEAD", sz) == 0) {
        return MRKHTTP_MHEAD;
    }
    if (strncasecmp(s, "POST", sz) == 0) {
        return MRKHTTP_MPOST;
    }
    if (strncasecmp(s, "PUT", sz) == 0) {
        return MRKHTTP_MPUT;
    }
    if (strncasecmp(s, "DELETE", sz) == 0) {
        return MRKHTTP_MDELETE;
    }
    if (strncasecmp(s, "OPTIONS", sz) == 0) {
        return MRKHTTP_MOPTIONS;
    }
    if (strncasecmp(s, "CONNECT", sz) == 0) {
        return MRKHTTP_MCONNECT;
    }
    if (strncasecmp(s, "TRACE", sz) == 0) {
        return MRKHTTP_MTRACE;
    }
    if (sz == 0) {
        return -1;
    }
    return MRKHTTP_MEXT;
}


static int
parse_request_line(mnhttp_ctx_t *ctx, mnbytestream_t *in, UNUSED void *udata)
{
    char *end, *tmp, *tmp1;
    size_t sz;

    if ((end = findcrlf(SDATA(in, ctx->first_line.start),
                        SEOD(in) - ctx->first_line.start)) == NULL) {

        /* MRKHTTP_PARSE_NEED_MORE */
        SADVANCEPOS(in, SEOD(in) - SPOS(in));
        return MRKHTTP_PARSE_NEED_MORE;
    }

    ctx->first_line.end = SDPOS(in, end);

    if ((tmp = findsp(SDATA(in, ctx->first_line.start),
                       ctx->first_line.end -
                       ctx->first_line.start)) == NULL) {

        TRRET(HTTP_PARSE_MESSAGE + 1);
    }

    sz = tmp - SDATA(in, ctx->first_line.start);
    if ((ctx->code.method = parse_method(SDATA(in, ctx->first_line.start),
                                         sz)) < 0) {
        TRRET(HTTP_PARSE_MESSAGE + 1);
    }

    if ((tmp1 = findnosp(tmp,
                         ctx->first_line.end -
                         ctx->first_line.start -
                         sz)) == NULL) {

        TRRET(HTTP_PARSE_MESSAGE + 1);
    }

    tmp = tmp1;
    sz = tmp - SDATA(in, ctx->first_line.start);

    if ((tmp1 = findsp(tmp,
                       ctx->first_line.end -
                       ctx->first_line.start - sz)) == NULL) {

        TRRET(HTTP_PARSE_MESSAGE + 1);
    }

    sz = tmp1 - tmp;
    if ((ctx->request_uri = malloc(sz + 1)) == NULL) {
        FAIL("malloc");
    }
    (void)memcpy(ctx->request_uri, tmp, sz);
    ctx->request_uri[sz]= '\0';


    sz = tmp1 - SDATA(in, ctx->first_line.start);
    if ((tmp = findnosp(tmp1,
                        ctx->first_line.end -
                        ctx->first_line.start -
                        sz)) == NULL) {

        TRRET(HTTP_PARSE_MESSAGE + 1);
    }


    /* strlen("HTTP/") = 5 */
    if (strncasecmp(tmp, "HTTP/", 5) != 0) {
        TRRET(HTTP_PARSE_MESSAGE + 2);
    }

    tmp += 5;

    ctx->version_major = strtol((const char *)tmp, &tmp1, 10);

    if (ctx->version_major == 0 &&
        (errno == EINVAL || errno == ERANGE)) {

        TRRET(HTTP_PARSE_MESSAGE + 3);
    }

    if (*tmp1 != '.') {
        TRRET(HTTP_PARSE_MESSAGE + 4);
    }

    /* advance by a single dot */
    tmp = tmp1 + 1;

    ctx->version_minor = strtol(tmp, &tmp1, 10);

    if (ctx->version_minor == 0 &&
        (errno == EINVAL || errno == ERANGE)) {

        TRRET(HTTP_PARSE_MESSAGE + 5);
    }

    /*
     * Reason phrase (ignore)
     */
    ctx->parser_state = PS_HEADER_FIELD_IN;
    SADVANCEPOS(in, end - SPDATA(in) + 2);

    return 0;
}


static int
parse_status_line(mnhttp_ctx_t *ctx, mnbytestream_t *in, UNUSED void *udata)
{
    char *end, *tmp, *tmp1;

    if ((end = findcrlf(SDATA(in, ctx->first_line.start),
                        SEOD(in) - ctx->first_line.start)) == NULL) {

        /* MRKHTTP_PARSE_NEED_MORE */
        SADVANCEPOS(in, SEOD(in) - SPOS(in));
        return MRKHTTP_PARSE_NEED_MORE;
    }

    ctx->first_line.end = SDPOS(in, end);

    if ((tmp1 = findnosp(SDATA(in, ctx->first_line.start),
                         ctx->first_line.end -
                         ctx->first_line.start)) == NULL) {

        TRRET(HTTP_PARSE_MESSAGE + 1);
    }

    tmp = tmp1;

    /* strlen("HTTP/") = 5 */
    if (strncasecmp(tmp, "HTTP/", 5) != 0) {
        TRRET(HTTP_PARSE_MESSAGE + 2);
    }

    tmp += 5;

    ctx->version_major = strtol((const char *)tmp, &tmp1, 10);

    if (ctx->version_major == 0 &&
        (errno == EINVAL || errno == ERANGE)) {

        TRRET(HTTP_PARSE_MESSAGE + 3);
    }

    if (*tmp1 != '.') {
        TRRET(HTTP_PARSE_MESSAGE + 4);
    }

    /* advance by a single dot */
    tmp = tmp1 + 1;

    ctx->version_minor = strtol(tmp, &tmp1, 10);

    if (ctx->version_minor == 0 &&
        (errno == EINVAL || errno == ERANGE)) {

        TRRET(HTTP_PARSE_MESSAGE + 5);
    }

    tmp = tmp1;

    /*
     * Status code
     */
    if ((tmp1 = findnosp(tmp, end - tmp)) == NULL) {
        TRRET(HTTP_PARSE_MESSAGE + 6);
    }

    tmp = tmp1;

    ctx->code.status = strtol(tmp, &tmp1, 10);

    if (ctx->code.status == 0 && (errno == EINVAL || errno == ERANGE)) {
        TRRET(HTTP_PARSE_MESSAGE + 7);
    }

    /*
     * Reason phrase (ignore)
     */
    ctx->parser_state = PS_HEADER_FIELD_IN;
    SADVANCEPOS(in, end - SPDATA(in) + 2);

    return 0;
}


static int
_http_parse_message(int fd,
                    mnbytestream_t *in,
                    mnhttp_cb_t parse_line_cb,
                    mnhttp_cb_t line_cb,
                    mnhttp_cb_t header_cb,
                    mnhttp_cb_t body_cb,
                    void *udata)
{
    int res = MRKHTTP_PARSE_NEED_MORE;
    mnhttp_ctx_t *ctx = in->udata;

    while (res == MRKHTTP_PARSE_NEED_MORE) {

        if (SNEEDMORE(in)) {
            int lres;
            if ((lres = bytestream_consume_data(in, fd)) != 0) {
                /* this must be treated as EOF condition */
#ifdef TRRET_DEBUG
                http_ctx_dump(ctx);
                TRACE("consume_data returned %08x", lres);
                perror("consume_data");
#endif
                if (lres == MRKHTTP_PARSE_EOF && ctx->parser_state <= PS_START) {
                    res = MRKHTTP_PARSE_EMPTY;
                } else {
                    res = MRKHTTP_PARSE_CONSUME_DATA_ERROR;
                }
                break;
            }
        }

        //D8(SPDATA(in), SEOD(in));
        //TRACE("parser_state=%s", PSSTR(ctx->parser_state));


        if (ctx->parser_state == PS_START) {

            ctx->first_line.start = SPOS(in);
            ctx->parser_state = PS_LINE;

        } else if (ctx->parser_state == PS_LINE) {
            if ((res = parse_line_cb(ctx, in, NULL)) == MRKHTTP_PARSE_NEED_MORE) {
                //TRACE("parse_line_cb() returned MRKHTTP_PARSE_NEED_MORE");
                continue;
            } else if (res != 0) {
                //TRACE("parse_line_cb() returned %d", res);
                return res;
            }

            if (line_cb != NULL) {
                if (line_cb(ctx, in, udata) != 0) {
                    TRRET(HTTP_PARSE_MESSAGE + 8);
                }
            }

            //TRACE("after line_cb() parser_state=%s", PSSTR(ctx->parser_state));
            res = MRKHTTP_PARSE_NEED_MORE;

        } else if (ctx->parser_state == PS_HEADER_FIELD_IN) {
            ctx->current_header_name.start = SPOS(in);
            ctx->parser_state = PS_HEADER_FIELD_OUT;
            //TRACE("SPOS=%ld SEOD=%ld", SPOS(in), SEOD(in));

        } else if (ctx->parser_state == PS_HEADER_FIELD_OUT) {
            char *end, *tmp;

            /*
             * XXX Header line continuations are not handled ATM.
             */
            if ((end = findcrlf(SDATA(in, ctx->current_header_name.start),
                                SEOD(in) -
                                ctx->current_header_name.start)) == NULL) {

                /* MRKHTTP_PARSE_NEED_MORE */
                //TRACE("SEOD=%ld SPOS=%ld", SEOD(in), SPOS(in));
                SADVANCEPOS(in, SEOD(in) - SPOS(in));
                continue;
            }

            //TRACE("current_header_name=%p end=%p",
            //      SDATA(in, ctx->current_header_name.start), end);
            //D8(SDATA(in, ctx->current_header_name.start), 8);
            //D8(end, 8);

            if (end == SDATA(in, ctx->current_header_name.start)) {
                /* empty line */
                SPOS(in) += 2;
                ctx->body.start = SPOS(in);
                ctx->body.end = ctx->body.start;
                /* in case the body is chunked */
                ctx->current_chunk.start = SPOS(in);
                ctx->current_chunk.end = ctx->current_chunk.start;
                ctx->parser_state = PS_BODY_IN;
                goto BODY_IN;

            } else {
                /* next header */
                ctx->current_header_value.end = SDPOS(in, end);

                if ((tmp = memchr(SDATA(in, ctx->current_header_name.start),
                                  ':',
                                  ctx->current_header_value.end -
                                  ctx->current_header_name.start)) == NULL) {

                    TRRET(HTTP_PARSE_MESSAGE + 9);
                }

                ctx->current_header_name.end = SDPOS(in, tmp);
                *tmp++ = '\0';
                tmp = findnosp(tmp, end - tmp);
                ctx->current_header_value.start = SDPOS(in, tmp);
                *end = '\0';

                if ((res = process_header(ctx, in)) != 0) {
                    if (res != MRKHTTP_PARSE_NEED_MORE) {
                        TRRET(HTTP_PARSE_MESSAGE + 10);
                    }
                }

                if (header_cb != NULL) {
                    if (header_cb(ctx, in, udata) != 0) {
                        TRRET(HTTP_PARSE_MESSAGE + 11);
                    }
                }

                ctx->parser_state = PS_HEADER_FIELD_IN;
            }

            SADVANCEPOS(in, end - SPDATA(in) + 2);

        } else if (ctx->parser_state == PS_BODY_IN) {

BODY_IN:
            if ((res = process_body(ctx, in, body_cb, udata)) != 0) {
                if (res != MRKHTTP_PARSE_NEED_MORE) {
                    TRRET(HTTP_PARSE_MESSAGE + 12);
                }
            }

            //D32(SDATA(in, ctx->current_chunk.start),
            //          ctx->current_chunk.end - ctx->current_chunk.start);
            ctx->parser_state = PS_BODY;

        } else if (ctx->parser_state == PS_BODY) {
            if ((res = process_body(ctx, in, body_cb, udata)) != 0) {
                if (res != MRKHTTP_PARSE_NEED_MORE) {
                    TRRET(HTTP_PARSE_MESSAGE + 13);
                }
            }

            //D32(SDATA(in, ctx->current_chunk.start),
            //          ctx->current_chunk.end - ctx->current_chunk.start);

        } else {
            //TRACE("state=%s", PSSTR(ctx->parser_state));
            TRRET(HTTP_PARSE_MESSAGE + 14);
        }
        //TRACE("res=%d", res);
    }

    TRRET(res);
}



int
http_parse_request(int fd,
                   mnbytestream_t *in,
                   mnhttp_cb_t line_cb,
                   mnhttp_cb_t header_cb,
                   mnhttp_cb_t body_cb,
                   void *udata)
{
    return _http_parse_message(fd, in, parse_request_line, line_cb, header_cb, body_cb, udata);
}

int
http_parse_response(int fd,
                    mnbytestream_t *in,
                    mnhttp_cb_t line_cb,
                    mnhttp_cb_t header_cb,
                    mnhttp_cb_t body_cb,
                    void *udata)
{
    return _http_parse_message(fd, in, parse_status_line, line_cb, header_cb, body_cb, udata);
}

