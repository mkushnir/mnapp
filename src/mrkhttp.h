#ifndef MRKHTTP_H
#define MRKHTTP_H

#include <stdarg.h>
#include <stdbool.h>
#include <time.h>
#include "mrkcommon/bytes.h"
#include "mrkcommon/bytestream.h"

#ifdef __cplusplus
extern "C" {
#endif

/* bytestream_consume */
#define MRKHTTP_PARSE_EOF (-1)
#define MRKHTTP_PARSE_NEED_MORE (-2)
/* end of data */
#define MRKHTTP_PARSE_EOD (-3)
#define MRKHTTP_PARSE_EMPTY (-4)
#define MRKHTTP_PARSE_CONSUME_DATA_ERROR (-5)

#define MRKHTTP_MOPTIONS (1)
#define MRKHTTP_MGET (2)
#define MRKHTTP_MHEAD (3)
#define MRKHTTP_MPOST (4)
#define MRKHTTP_MPUT (5)
#define MRKHTTP_MDELETE (6)
#define MRKHTTP_MTRACE (7)
#define MRKHTTP_MCONNECT (8)
#define MRKHTTP_MEXT (9)

#define MRKHTTP_MSTR(m)                \
((m) == MRKHTTP_MOPTIONS ? "OPTIONS":  \
 (m) == MRKHTTP_MGET ? "GET":          \
 (m) == MRKHTTP_MHEAD ? "HEAD":        \
 (m) == MRKHTTP_MPOST ? "POST":        \
 (m) == MRKHTTP_MPUT ? "PUT":          \
 (m) == MRKHTTP_MDELETE ? "DELETE":    \
 (m) == MRKHTTP_MTRACE ? "TRACE":      \
 (m) == MRKHTTP_MCONNECT ? "CONNECT":  \
 "<ext>")                              \


typedef struct _mnhttp_ctx {
#define PS_START 1
#define PS_LINE 2
#define PS_HEADER_FIELD_IN 3
#define PS_HEADER_FIELD_OUT 4
#define PS_BODY_IN 5
#define PS_BODY 6

#define PSSTR(ps) (                                            \
        (ps) == PS_START ? "START" :                           \
        (ps) == PS_LINE ? "LINE" :                             \
        (ps) == PS_HEADER_FIELD_IN ? "HEADER_FIELD_IN" :       \
        (ps) == PS_HEADER_FIELD_OUT ? "HEADER_FIELD_OUT" :     \
        (ps) == PS_BODY_IN ? "BODY_IN" :                       \
        (ps) == PS_BODY ? "BODY" :                             \
        "<unknown>"                                            \
        )                                                      \


    char parser_state;

#define PS_CHUNK_SIZE 1
#define PS_CHUNK_DATA 2

#define CPSSTR(ps) (                           \
        (ps) == PS_CHUNK_SIZE ? "CHUNK_SIZE" : \
        (ps) == PS_CHUNK_DATA ? "CHUNK_DATA" : \
        "<unknown>"                            \
        )                                      \


    char chunk_parser_state;
    byterange_t first_line;
    int version_major;
    int version_minor;
    union {
        int status;
        int method;
    } code;
    char *request_uri;
    int bodysz;
    byterange_t current_header_name;
    byterange_t current_header_value;
#define PS_FLAG_CHUNKED 0x01
    int flags;
    byterange_t body;
    int current_chunk_size;
    byterange_t current_chunk;
    void *udata;
} mnhttp_ctx_t;


/*
 * https://tools.ietf.org/html/rfc3986
 */
typedef struct _mrkhttp_uri {
#define MNHTTPC_MESSAGE_SCHEME_UNDEF    (-1)
#define MNHTTPC_MESSAGE_SCHEME_HTTP     (0)
#define MNHTTPC_MESSAGE_SCHEME_HTTPS    (1)
    int scheme;
    mnbytes_t *user;
    mnbytes_t *password;
    mnbytes_t *host;
    mnbytes_t *port;
    mnbytes_t *relative;
    mnbytes_t *path;
    mnbytes_t *qstring;
    mnbytes_t *fragment;
} mrkhttp_uri_t;


char *findcrlf(char *, int);

void http_ctx_init(mnhttp_ctx_t *);
void http_ctx_fini(mnhttp_ctx_t *);
void mrkhttp_uri_init(mrkhttp_uri_t *);
void mrkhttp_uri_fini(mrkhttp_uri_t *);
mnhttp_ctx_t *http_ctx_new(void);
void http_ctx_destroy(mnhttp_ctx_t **);

typedef int (*mnhttp_cb_t) (mnhttp_ctx_t *, mnbytestream_t *, void *);
#define mnhttp_ctx_last_chunk(ctx) ((bool)((ctx)->current_chunk_size == 0))

char *http_urlencode_reserved(const char *, size_t);
char *http_urldecode(char *);

int http_start_request(mnbytestream_t *, const char *, const char *);
int http_start_response(mnbytestream_t *, int, const char *);
int http_add_header_field(mnbytestream_t *, const char *, const char *);
int PRINTFLIKE(3, 4) http_field_addf(mnbytestream_t *,
                                     mnbytes_t *,
                                     const char *,
                                     ...);
int http_field_addb(mnbytestream_t *, mnbytes_t *, mnbytes_t *);
int http_field_addt(mnbytestream_t *, mnbytes_t *, time_t);


#define MRKHTTP_PRINTF_HEADER(out, name, fmt, ...)                             \
    (void)bytestream_nprintf(out, 4096, name ": " fmt "\r\n", __VA_ARGS__)     \


#define MRKHTTP_PRINTF_HEADER_INT(out, name, val)                              \
    (void)bytestream_nprintf(out, 4096, name ": %ld\r\n", (intmax_t)(val))     \


#define MRKHTTP_PRINTF_HEADER_BOOL(out, name, val)                             \
    (void)bytestream_nprintf(out, 4096, name ": %s\r\n", (val) ? "yes" : "no") \


#define MRKHTTP_PRINTF_HEADER_FLOAT(out, name, val)                            \
    (void)bytestream_nprintf(out, 4096, name ": %lf\r\n", (double)(val))       \


#define MRKHTTP_PRINTF_HEADER_BYTES(out, name, val)    \
    (void)bytestream_nprintf(out,                      \
                             4096 + BSZ(val),          \
                             name ": %s\r\n",          \
                             (char *)BDATA(val))       \


#define MRKHTTP_PRINTF_HEADER_RFC1123(out, name, val)          \
do {                                                           \
    char __mrkhttp_printf_header_rfc1123_buf[256];             \
    size_t __mrkhttp_printf_header_rfc1123_sz;                 \
    struct tm *__mrkhttp_printf_header_rfc1123_t;              \
    __mrkhttp_printf_header_rfc1123_t = gmtime(val);           \
    __mrkhttp_printf_header_rfc1123_sz = strftime(             \
            __mrkhttp_printf_header_rfc1123_buf,               \
            sizeof(__mrkhttp_printf_header_rfc1123_buf),       \
            "%a, %d %b %Y %H:%M:%S GMT",                       \
            __mrkhttp_printf_header_rfc1123_t);                \
    (void)bytestream_nprintf(                                  \
            out,                                               \
            4096 + __mrkhttp_printf_header_rfc1123_sz,         \
            name ": %s\r\n",                                   \
            __mrkhttp_printf_header_rfc1123_buf);              \
} while (0)                                                    \


void http_ctx_dump(const mnhttp_ctx_t *);

int http_end_of_header(mnbytestream_t *);
int http_add_body(mnbytestream_t *, const char *, size_t);

int http_parse_request(void *,
                       mnbytestream_t *,
                       mnhttp_cb_t,
                       mnhttp_cb_t,
                       mnhttp_cb_t,
                       void *);

int http_parse_response(void *,
                        mnbytestream_t *,
                        mnhttp_cb_t,
                        mnhttp_cb_t,
                        mnhttp_cb_t,
                        void *);

void mrkhttp_uri_parse(mrkhttp_uri_t *, const char *);

#ifdef __cplusplus
}
#endif

#endif
