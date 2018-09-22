#ifndef MNCOMMAND_H
#define MNCOMMAND_H

#include <getopt.h>
#include <mrkcommon/bytes.h>
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
                          mnbytes_t *,
                          int,
                          int,
                          mnbytes_t *,
                          mncommand_option_func_t);

int mncommand_ctx_init(mncommand_ctx_t *);
int mncommand_ctx_fini(mncommand_ctx_t *);


#ifdef __cplusplus
}
#endif
#endif
