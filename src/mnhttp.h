#ifndef MNHTTP_H
#define MNHTTP_H

#include <stdarg.h>
#include <stdbool.h>
#include <time.h>

#include <mncommon/bytes.h>
#include <mncommon/hash.h>
#include <mncommon/bytestream.h>

#ifdef __cplusplus
extern "C" {
#endif

/* bytestream_consume */
#define MNHTTP_PARSE_EOF (-1)
#define MNHTTP_PARSE_NEED_MORE (-2)
/* end of data */
#define MNHTTP_PARSE_EOD (-3)
#define MNHTTP_PARSE_EMPTY (-4)
#define MNHTTP_PARSE_CONSUME_DATA_ERROR (-5)

#define MNHTTP_MOPTIONS    (1)
#define MNHTTP_METHOD_OPTIONS  "OPTIONS"
#define MNHTTP_MGET        (2)
#define MNHTTP_METHOD_GET      "GET"
#define MNHTTP_MHEAD       (3)
#define MNHTTP_METHOD_HEAD     "HEAD"
#define MNHTTP_MPOST       (4)
#define MNHTTP_METHOD_POST     "POST"
#define MNHTTP_MPUT        (5)
#define MNHTTP_METHOD_PUT      "PUT"
#define MNHTTP_MDELETE     (6)
#define MNHTTP_METHOD_DELETE   "DELETE"
#define MNHTTP_MTRACE      (7)
#define MNHTTP_METHOD_TRACE    "TRACE"
#define MNHTTP_MCONNECT    (8)
#define MNHTTP_METHOD_CONNECT  "CONNECT"
/* MS Graph API*/
#define MNHTTP_MPATCH      (9)
#define MNHTTP_METHOD_PATCH    "PATCH"

#define MNHTTP_MEXT        (10)

#define MNHTTP_MSTR(m)                                \
((m) == MNHTTP_MOPTIONS ? MNHTTP_METHOD_OPTIONS:     \
 (m) == MNHTTP_MGET ? MNHTTP_METHOD_GET:             \
 (m) == MNHTTP_MHEAD ? MNHTTP_METHOD_HEAD:           \
 (m) == MNHTTP_MPOST ? MNHTTP_METHOD_POST:           \
 (m) == MNHTTP_MPUT ? MNHTTP_METHOD_PUT:             \
 (m) == MNHTTP_MDELETE ? MNHTTP_METHOD_DELETE:       \
 (m) == MNHTTP_MTRACE ? MNHTTP_METHOD_TRACE:         \
 (m) == MNHTTP_MCONNECT ? MNHTTP_METHOD_CONNECT:     \
 (m) == MNHTTP_MPATCH ? MNHTTP_METHOD_PATCH:         \
 "<ext>")                                              \


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
typedef struct _mnhttp_uri {
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
    mnhash_t qterms;
    size_t qtermsz;
} mnhttp_uri_t;


char *findcrlf(char *, int);

void http_ctx_init(mnhttp_ctx_t *);
void http_ctx_fini(mnhttp_ctx_t *);
void mnhttp_uri_init(mnhttp_uri_t *);
void mnhttp_uri_fini(mnhttp_uri_t *);
void mnhttp_uri_add_qterm(mnhttp_uri_t *, mnbytes_t *, mnbytes_t *);
int mnhttp_uri_start_request(mnhttp_uri_t *, mnbytestream_t *, const char *);
int mnhttp_parse_qterms(mnbytes_t *, char, char, mnhash_t *);
int mnhttp_parse_kvpbd(mnbytes_t *, char, char, mnhash_t *);
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
                                     const mnbytes_t *,
                                     const char *,
                                     ...);
int http_field_addb(mnbytestream_t *, const mnbytes_t *, const mnbytes_t *);
int http_field_addt(mnbytestream_t *, const mnbytes_t *, time_t);


#define MNHTTP_PRINTF_HEADER(out, name, fmt, ...)                             \
    (void)bytestream_nprintf(out, 4096, name ": " fmt "\r\n", __VA_ARGS__)     \


#define MNHTTP_PRINTF_HEADER_INT(out, name, val)                              \
    (void)bytestream_nprintf(out, 4096, name ": %ld\r\n", (intmax_t)(val))     \


#define MNHTTP_PRINTF_HEADER_BOOL(out, name, val)                             \
    (void)bytestream_nprintf(out, 4096, name ": %s\r\n", (val) ? "yes" : "no") \


#define MNHTTP_PRINTF_HEADER_FLOAT(out, name, val)                            \
    (void)bytestream_nprintf(out, 4096, name ": %lf\r\n", (double)(val))       \


#define MNHTTP_PRINTF_HEADER_BYTES(out, name, val)    \
    (void)bytestream_nprintf(out,                      \
                             4096 + BSZ(val),          \
                             name ": %s\r\n",          \
                             BCDATA(val))              \


#define MNHTTP_PRINTF_HEADER_RFC1123(out, name, val)          \
do {                                                           \
    char __mnhttp_printf_header_rfc1123_buf[256];             \
    size_t __mnhttp_printf_header_rfc1123_sz;                 \
    struct tm *__mnhttp_printf_header_rfc1123_t;              \
    __mnhttp_printf_header_rfc1123_t = gmtime(val);           \
    __mnhttp_printf_header_rfc1123_sz = strftime(             \
            __mnhttp_printf_header_rfc1123_buf,               \
            sizeof(__mnhttp_printf_header_rfc1123_buf),       \
            "%a, %d %b %Y %H:%M:%S GMT",                       \
            __mnhttp_printf_header_rfc1123_t);                \
    (void)bytestream_nprintf(                                  \
            out,                                               \
            4096 + __mnhttp_printf_header_rfc1123_sz,         \
            name ": %s\r\n",                                   \
            __mnhttp_printf_header_rfc1123_buf);              \
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

void mnhttp_uri_parse(mnhttp_uri_t *, const char *);

#ifdef __cplusplus
}
#endif

#endif
