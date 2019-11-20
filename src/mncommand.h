#ifndef MNCOMMAND_H
#define MNCOMMAND_H

#include <stdbool.h>
#include <stdint.h>
#include <getopt.h>

#include <mrkcommon/bytes.h>
#include <mrkcommon/bytestream.h>
#include <mrkcommon/array.h>


struct _mncommand_ctx;
struct _mncommand_cmd;

typedef int (*mncommand_option_func_t)(
    struct _mncommand_ctx *,
    struct _mncommand_cmd *,
    const char *,
    void *);

typedef struct _mncommand_cmd {
    mnbytes_t *longname;
    int flag;
    mnbytes_t *description;
    mnbytes_t *helpspec;
    struct option opt;
    mncommand_option_func_t func;
    void *udata;
} mncommand_cmd_t;


typedef struct _mncommand_ctx {
    mnbytes_t *progname;
    /* mncommand_cmd_t */
    mnarray_t commands;
    /* struct option */
    mnarray_t options;
} mncommand_ctx_t;


#ifdef __cplusplus
extern "C" {
#endif

void mncommand_ctx_format_help(mncommand_ctx_t *, mnbytestream_t *);
int mncommand_ctx_getopt(mncommand_ctx_t *, int, char *[], void *);

int mncommand_ctx_add_cmd(mncommand_ctx_t *,
                          const mnbytes_t *,
                          int,
                          int,
                          const mnbytes_t *,
                          mncommand_option_func_t,
                          void *);

int mncommand_option_int(mncommand_ctx_t *,
                         mncommand_cmd_t *,
                         const char *,
                         void *);

int mncommand_option_uint(mncommand_ctx_t *,
                          mncommand_cmd_t *,
                          const char *,
                          void *);

int mncommand_option_double(mncommand_ctx_t *,
                            mncommand_cmd_t *,
                            const char *,
                            void *);

int mncommand_option_bool(mncommand_ctx_t *,
                          mncommand_cmd_t *,
                          const char *,
                          void *);

int mncommand_option_bytes(mncommand_ctx_t *,
                           mncommand_cmd_t *,
                           const char *,
                           void *);

#define MNCOMMAND_CTX_ADD_CMDTG(ctx,                                   \
                                longname,                              \
                                shortname,                             \
                                has_arg,                               \
                                description,                           \
                                value)                                 \
    mncommand_ctx_add_cmd(ctx,                                         \
                          longname,                                    \
                          shortname,                                   \
                          has_arg,                                     \
                          description,                                 \
                          _Generic(value,                              \
                              intmax_t *: mncommand_option_int,        \
                              uintmax_t *: mncommand_option_uint,      \
                              double *: mncommand_option_double,       \
                              bool *: mncommand_option_bool,           \
                              mnbytes_t **: mncommand_option_bytes),   \
                          value)                                       \


int mncommand_ctx_init(mncommand_ctx_t *);
int mncommand_ctx_fini(mncommand_ctx_t *);


#ifdef __cplusplus
}
#endif
#endif
