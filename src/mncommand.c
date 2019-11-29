#include <assert.h>
#include <stdbool.h>
#include <ctype.h>
#include <getopt.h>
#include <libgen.h>
#include <stdlib.h>
#include <limits.h>
#include <inttypes.h>
#include <strings.h>

#include <mrkcommon/array.h>
#include <mrkcommon/bytes.h>
#include <mrkcommon/bytestream.h>
#include <mrkcommon/dumpm.h>
#include <mrkcommon/util.h>

#include <mncommand.h>

#include "diag.h"


void
mncommand_ctx_format_help(mncommand_ctx_t *ctx, mnbytestream_t *bs)
{
    mnarray_iter_t it;
    mncommand_cmd_t *cmd;

    bytestream_nprintf(bs, 1024,
        "Usage: %s [OPTIONS] [ARGUMENTS]\n"
        "\n",
        BDATA(ctx->progname));

#define SPECW 24
#define LINEW 78
#define SPECE "                        "
    for (cmd = array_first(&ctx->commands, &it);
         cmd != NULL;
         cmd = array_next(&ctx->commands, &it)) {
        ssize_t nwritten;

        if (MRKLIKELY(
                (nwritten = bytestream_nprintf(
                    bs,
                    1024,
                    "  %-22s",
                    BDATA(cmd->helpspec))) > 0)) {

            if (nwritten > SPECW) {
                (void)bytestream_cat(bs, 1, "\n");
                (void)bytestream_nprintf(bs, 1024, SPECE);
            }

            if (cmd->description != NULL) {
                char *s;
                size_t nhead, ntail;

                s = BCDATA(cmd->description);
                nhead = 0;
                ntail = BSZ(cmd->description) - 1;

                while (true) {
                    size_t ncopy;
                    char buf[LINEW - SPECW + 1 /* optional dash */];
                    ncopy = MIN(ntail,
                                sizeof(buf)
                                    - 1 /* optional dash */
                                    - 1 /* term zero */);
                    memcpy(buf, s + nhead, ncopy);
                    if (isalnum(s[nhead + ncopy - 1]) &&
                            isalnum(s[nhead + ncopy])) {
                        buf[ncopy] = '-';
                        buf[ncopy + 1] = '\0';
                    } else {
                        buf[ncopy] = '\0';
                    }
                    (void)bytestream_nprintf(bs, 1024, "%s\n", buf);
                    ntail -= ncopy;
                    nhead += ncopy;
                    if (ntail > 0) {
                        (void)bytestream_nprintf(bs, 1024, SPECE);
                    } else {
                        goto end0;
                    }
                }
end0:
                ;

            } else {
                (void)bytestream_cat(bs, 1, "\n");
            }
        }
    }
}


static int
getopt_symbol(mncommand_cmd_t *cmd, mnbytestream_t *bs)
{
    if (isalnum(cmd->opt.val)) {
        char c = cmd->opt.val;
        bytestream_cat(bs, 1, &c);
        if (cmd->opt.has_arg != no_argument) {
            bytestream_cat(bs, 1, ":");
        }
    }
    cmd->opt.flag = &cmd->flag;
    return 0;
}


static mncommand_cmd_t *
mncommand_ctx_find_cmd(mncommand_ctx_t *ctx, int curropt, int optch)
{
    mncommand_cmd_t *res;

    res = NULL;
    if (curropt == -1) {
        mnarray_iter_t it;
        for (res = array_first(&ctx->commands, &it);
             res != NULL;
             res = array_next(&ctx->commands, &it)) {
            if (res->opt.val == optch) {
                break;
            }
        }
    } else {
        res = array_get(&ctx->commands, curropt);
    }
    return res;
}


int
mncommand_ctx_getopt(mncommand_ctx_t *ctx,
                     int argc,
                     char *argv[],
                     void *udata)
{
    int res;
    mnbytestream_t bs;
    struct option *options;
    mncommand_cmd_t *cmd;
    mnarray_iter_t it;

    bytestream_init(&bs, 1024);

    if (ctx->progname == NULL) {
        char *s = basename(argv[0]);
        ctx->progname = bytes_new_from_str(s);
    }

    // fixups
    for (cmd = array_first(&ctx->commands, &it);
         cmd != NULL;
         cmd = array_next(&ctx->commands, &it)) {
        struct option *opt = ARRAY_GET(struct option, &ctx->options, it.iter);
        cmd->opt.flag = &cmd->flag;
        opt->flag = &cmd->flag;
    }

    // build getopt_long(3) symbols
    (void)array_traverse(&ctx->commands, (array_traverser_t)getopt_symbol, &bs);
    /* terminating null */
    bytestream_cat(&bs, 1, "");

    if (MRKUNLIKELY((options = array_incr(&ctx->options)) == NULL)) {
        FFAIL("array_incr");
    }
    *options = (struct option){0};

    if ((options = array_get(&ctx->options, 0)) == NULL) {
        res = MNCOMMAND_CTX_GETOPT + 1;
        TR(res);
        goto end;
    }

    //TRACE("opts=%s", SDATA(&bs, 0));

    while (true) {
        int ch;
        int curropt;
        mncommand_cmd_t *cmd;

        curropt = -1;
        if ((ch = getopt_long(argc,
                              argv,
                              SDATA(&bs, 0),
                              options,
                              &curropt)) == -1) {
            break;
        }

        //TRACE("curropt=%d optarg=%s ch=%d", curropt, optarg, ch);

        if ((cmd = mncommand_ctx_find_cmd(ctx, curropt, ch)) == NULL) {
            //TRACE("getopt_long error");
            res = MNCOMMAND_CTX_GETOPT + 2;
            TR(res);
            goto end;

        } else {
            if (cmd->func != NULL) {
                if ((res = cmd->func(ctx, cmd, optarg, udata)) != 0) {
                    goto end;
                }
            }
        }
    }

    res = optind;

end:
    bytestream_fini(&bs);
    return res;
}


static void
mncommand_cmd_compute_helpspec(mncommand_cmd_t *cmd)
{
    mnbytes_t *namespec;
    mnbytes_t *argspec;

    if (cmd->longname != NULL) {
        if (isalnum(cmd->opt.val)) {
            namespec = bytes_printf(
                "--%s|-%c", BDATA(cmd->longname), cmd->opt.val);
        } else {
            namespec = bytes_printf("--%s", BDATA(cmd->longname));
        }
    } else {
        if (isalnum(cmd->opt.val)) {
            namespec = bytes_printf("-%c", cmd->opt.val);
        } else {
            namespec = bytes_new_from_str("--");
        }
    }

    if (cmd->opt.has_arg) {
        argspec = bytes_new_from_str("ARG");
    } else {
        argspec = bytes_new_from_str("");
    }

    cmd->helpspec = bytes_printf("%s %s", BDATA(namespec), BDATA(argspec));

    BYTES_DECREF(&namespec);
    BYTES_DECREF(&argspec);
}


int
mncommand_option_int(UNUSED mncommand_ctx_t *ctx,
                     mncommand_cmd_t *cmd,
                     const char *optarg,
                     UNUSED void *udata)
{
    intmax_t *v = cmd->udata;
    *v = strtoimax(optarg, NULL, 10);
    return 0;
}


int
mncommand_option_uint(UNUSED mncommand_ctx_t *ctx,
                      mncommand_cmd_t *cmd,
                      const char *optarg,
                      UNUSED void *udata)
{
    uintmax_t *v = cmd->udata;
    *v = strtoumax(optarg, NULL, 10);
    return 0;
}


int
mncommand_option_double(UNUSED mncommand_ctx_t *ctx,
                        mncommand_cmd_t *cmd,
                        const char *optarg,
                        UNUSED void *udata)
{
    double *v = cmd->udata;
    *v = strtod(optarg, NULL);
    return 0;
}


int
mncommand_option_bool(UNUSED mncommand_ctx_t *ctx,
                      mncommand_cmd_t *cmd,
                      const char *optarg,
                      UNUSED void *udata)
{
    bool *v = cmd->udata;
    if ((optarg != NULL) && (strcasecmp(optarg, "0") == 0 ||
        strcasecmp(optarg, "off") == 0 ||
        strcasecmp(optarg, "false") == 0 ||
        strcasecmp(optarg, "no") == 0)) {
        *v = false;
    } else {
        *v = true;
    }
    return 0;
}


int
mncommand_option_bytes(UNUSED mncommand_ctx_t *ctx,
                       mncommand_cmd_t *cmd,
                       const char *optarg,
                       UNUSED void *udata)
{
    mnbytes_t **v = cmd->udata;
    BYTES_DECREF(v);
    *v = bytes_new_from_str(optarg);
    BYTES_INCREF(*v);
    return 0;
}


int
mncommand_ctx_add_cmd(mncommand_ctx_t *ctx,
                      const mnbytes_t *longname,
                      int shortname,
                      int has_arg,
                      const mnbytes_t *description,
                      mncommand_option_func_t func,
                      void *udata)
{
    mncommand_cmd_t *cmd;
    struct option *opt;

    if (MRKUNLIKELY((cmd = array_incr(&ctx->commands)) == NULL)) {
        TRRET(MNCOMMAND_CTX_ADD_CMD + 1);
    }

    assert(longname != NULL);
    BYTES_DECREF(&cmd->longname);
    cmd->longname = bytes_new_from_bytes(longname);
    BYTES_INCREF(cmd->longname);

    BYTES_DECREF(&cmd->description);
    if (description != NULL) {
        cmd->description = bytes_new_from_bytes(description);
        BYTES_INCREF(cmd->description);
    }

    cmd->func = func;
    cmd->udata = udata;
    cmd->opt.name = BCDATA(cmd->longname);
    cmd->opt.has_arg = has_arg;
    cmd->opt.flag = NULL; // fixup later, set to cmd->flag
    cmd->opt.val = shortname;
    mncommand_cmd_compute_helpspec(cmd);

    if (MRKUNLIKELY((opt = array_incr(&ctx->options)) == NULL)) {
        TRRET(MNCOMMAND_CTX_ADD_CMD + 2);
    }

    *opt = cmd->opt;

    return 0;

}


static int
mncommand_cmd_init(mncommand_cmd_t *cmd)
{
    memset(cmd, 0, sizeof(mncommand_cmd_t));
    return 0;
}


static int
mncommand_cmd_fini(mncommand_cmd_t *cmd)
{
    BYTES_DECREF(&cmd->longname);
    BYTES_DECREF(&cmd->description);
    BYTES_DECREF(&cmd->helpspec);
    return 0;
}


int
mncommand_ctx_init(mncommand_ctx_t *ctx)
{
    ctx->progname = NULL;
    if (array_init(&ctx->commands,
                   sizeof(mncommand_cmd_t),
                   0,
                   (array_initializer_t)mncommand_cmd_init,
                   (array_finalizer_t)mncommand_cmd_fini) != 0) {
        TRRET(MNCOMMAND_CTX_INIT + 1);
    }

    if (array_init(&ctx->options,
                   sizeof(struct option),
                   0,
                   NULL,
                   NULL) != 0) {
        TRRET(MNCOMMAND_CTX_INIT + 2);
    }

    return 0;
}


int
mncommand_ctx_fini(mncommand_ctx_t *ctx)
{
    (void)array_fini(&ctx->options);
    (void)array_fini(&ctx->commands);
    BYTES_DECREF(&ctx->progname);
    return 0;
}
