#include <assert.h>
#include <stdbool.h>
#include <ctype.h>
#include <getopt.h>
#include <libgen.h>

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

                s = (char *)BDATA(cmd->description);
                nhead = 0;
                ntail = BSZ(cmd->description) - 1;

                while (true) {
                    size_t ncopy;
                    char buf[LINEW - SPECW];
                    ncopy = MIN(ntail, sizeof(buf) - 1);
                    memcpy(buf, s + nhead, ncopy);
                    buf[ncopy] = '\0';
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
                (void)bytestream_cat(bs, 1, "\n");

            } else {
                if (nwritten > SPECW) {
                    (void)bytestream_cat(bs, 1, "\n");
                } else {
                    (void)bytestream_cat(bs, 2, "\n\n");
                }
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
    return 0;
}


static mncommand_cmd_t *
mncommand_ctx_find_cmd(mncommand_ctx_t *ctx, int curropt, int optarg)
{
    mncommand_cmd_t *res;

    res = NULL;
    if (curropt == -1) {
        mnarray_iter_t it;
        for (res = array_first(&ctx->commands, &it);
             res != NULL;
             res = array_next(&ctx->commands, &it)) {
            if (res->opt.val == optarg) {
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
    mnbytestream_t bs;
    struct option *options;

    if (ctx->progname == NULL) {
        char *s = basename(argv[0]);
        ctx->progname = bytes_new_from_str(s);
    }

    bytestream_init(&bs, 1024);
    (void)array_traverse(&ctx->commands, (array_traverser_t)getopt_symbol, &bs);
    /* terminating null */
    bytestream_cat(&bs, 1, "");

    if ((options = array_get(&ctx->options, 0)) == NULL) {
        TRRET(MNCOMMAND_CTX_GETOPT + 1);
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
            goto err;

        } else {
            //TRACE("longname=%s, optarg=%s helpspec=%s",
            //      BDATA(cmd->longname),
            //      optarg,
            //      BDATA(cmd->helpspec));
            if (cmd->func != NULL) {
                if (cmd->func(ctx, cmd, optarg, udata) != 0) {
                    break;
                }
            }
        }
    }

    bytestream_fini(&bs);

    return optind;
err:
    return -1;
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
mncommand_ctx_add_cmd(mncommand_ctx_t *ctx,
                      mnbytes_t *longname,
                      int shortname,
                      int has_arg,
                      mnbytes_t *description,
                      mncommand_option_func_t func)
{
    mncommand_cmd_t *cmd;
    struct option *opt;

    if (MRKUNLIKELY((cmd = array_incr(&ctx->commands)) == NULL)) {
        TRRET(MNCOMMAND_CTX_ADD_CMD + 1);
    }

    assert(longname != NULL);
    cmd->longname = longname;
    if (cmd->longname != NULL) {
        BYTES_INCREF(cmd->longname);
    }
    cmd->description = description;
    if (cmd->description != NULL) {
        BYTES_INCREF(cmd->description);
    }
    cmd->func = func;
    cmd->opt.name = (const char *)BDATASAFE(cmd->longname);
    cmd->opt.has_arg = has_arg;
    cmd->opt.flag = &cmd->flag;
    cmd->opt.val = shortname;
    mncommand_cmd_compute_helpspec(cmd);

    if (MRKUNLIKELY((opt = array_incr(&ctx->options)) == NULL)) {
        TRRET(MNCOMMAND_CTX_ADD_CMD + 2);
    }

    *opt = cmd->opt;

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
                   NULL,
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