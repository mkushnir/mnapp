#include <assert.h>
#include <stdlib.h>
#include <time.h>

#include "unittest.h"
#define TRRET_DEBUG
#include <mrkcommon/dumpm.h>
#include <mrkcommon/util.h>
#include <mrkapp.h>
#include <mnhttp.h>

#include "diag.h"

#ifndef NDEBUG
const char *_malloc_options = "AJ";
#endif


static void
test0(void)
{
    unsigned i;
    mnhttp_uri_t uri;
    struct _testcase {
        const char *in;
        int scheme;
        const char *user;
        const char *password;
        const char *host;
        const char *port;
        const char *path;
        const char *qstring;
        const char *fragment;
    } data[] = {
        {
            "localhost",
            MNHTTPC_MESSAGE_SCHEME_HTTP,
            NULL,
            NULL,
            "localhost",
            "80",
            NULL,
            NULL,
            NULL
        },

        {
            "localhost:80",
            MNHTTPC_MESSAGE_SCHEME_HTTP,
            NULL,
            NULL,
            "localhost",
            "80",
            NULL,
            NULL,
            NULL
        },

        {
            "http://localhost",
            MNHTTPC_MESSAGE_SCHEME_HTTP,
            NULL,
            NULL,
            "localhost",
            "80",
            NULL,
            NULL,
            NULL
        },

        {
            "http://localhost:80",
            MNHTTPC_MESSAGE_SCHEME_HTTP,
            NULL,
            NULL,
            "localhost",
            "80",
            NULL,
            NULL,
            NULL
        },

        {
            "localhost:443",
            MNHTTPC_MESSAGE_SCHEME_HTTPS,
            NULL,
            NULL,
            "localhost",
            "443",
            NULL,
            NULL,
            NULL
        },

        {
            "https://localhost:443",
            MNHTTPC_MESSAGE_SCHEME_HTTPS,
            NULL,
            NULL,
            "localhost",
            "443",
            NULL,
            NULL,
            NULL
        },

        {
            "https://localhost",
            MNHTTPC_MESSAGE_SCHEME_HTTPS,
            NULL,
            NULL,
            "localhost",
            "443",
            NULL,
            NULL,
            NULL
        },

        {
            "",
            MNHTTPC_MESSAGE_SCHEME_HTTP,
            NULL,
            NULL,
            "",
            "80",
            NULL,
            NULL,
            NULL
        },


        {
            "http://",
            MNHTTPC_MESSAGE_SCHEME_HTTP,
            NULL,
            NULL,
            "",
            "80",
            NULL,
            NULL,
            NULL
        },


        {
            "https://",
            MNHTTPC_MESSAGE_SCHEME_HTTPS,
            NULL,
            NULL,
            "",
            "443",
            NULL,
            NULL,
            NULL
        },

        {
            "https://:80",
            MNHTTPC_MESSAGE_SCHEME_HTTPS,
            NULL,
            NULL,
            "",
            "80",
            NULL,
            NULL,
            NULL
        },


        {
            "https://asd@:80",
            MNHTTPC_MESSAGE_SCHEME_HTTPS,
            "asd",
            NULL,
            "",
            "80",
            NULL,
            NULL,
            NULL
        },

        {
            "https://asd@::80",
            MNHTTPC_MESSAGE_SCHEME_HTTPS,
            "asd",
            NULL,
            "",
            ":80",
            NULL,
            NULL,
            NULL
        },


        {
            "https://asd@:d:80",
            MNHTTPC_MESSAGE_SCHEME_HTTPS,
            "asd",
            NULL,
            "",
            "d:80",
            NULL,
            NULL,
            NULL
        },

        {
            ":",
            MNHTTPC_MESSAGE_SCHEME_HTTP,
            NULL,
            NULL,
            "",
            "80",
            NULL,
            NULL,
            NULL
        },


        {
            ":#",
            MNHTTPC_MESSAGE_SCHEME_HTTP,
            NULL,
            NULL,
            "",
            "#",
            NULL,
            NULL,
            NULL,
        },


        {
            ":80#",
            MNHTTPC_MESSAGE_SCHEME_HTTP,
            NULL,
            NULL,
            "",
            "80#",
            NULL,
            NULL,
            NULL,
        },

        {
            ":80/#",
            MNHTTPC_MESSAGE_SCHEME_HTTP,
            NULL,
            NULL,
            "",
            "80",
            "/",
            NULL,
            "#",
        },

        {
            "localhost?",
            MNHTTPC_MESSAGE_SCHEME_HTTP,
            NULL,
            NULL,
            "localhost?",
            "80",
            NULL,
            NULL,
            NULL,
        },

        {
            "localhost/",
            MNHTTPC_MESSAGE_SCHEME_HTTP,
            NULL,
            NULL,
            "localhost",
            "80",
            "/",
            NULL,
            NULL,
        },

        {
            "localhost/#",
            MNHTTPC_MESSAGE_SCHEME_HTTP,
            NULL,
            NULL,
            "localhost",
            "80",
            "/",
            NULL,
            "#",
        },

        {
            "localhost/?#",
            MNHTTPC_MESSAGE_SCHEME_HTTP,
            NULL,
            NULL,
            "localhost",
            "80",
            "/",
            "",
            "#",
        },


        {
            "user@localhost/?#",
            MNHTTPC_MESSAGE_SCHEME_HTTP,
            "user",
            NULL,
            "localhost",
            "80",
            "/",
            "",
            "#",
        },

        {
            "@localhost/?#",
            MNHTTPC_MESSAGE_SCHEME_HTTP,
            "",
            NULL,
            "localhost",
            "80",
            "/",
            "",
            "#",
        },

        {
            ":@localhost/?#",
            MNHTTPC_MESSAGE_SCHEME_HTTP,
            "",
            "",
            "localhost",
            "80",
            "/",
            "",
            "#",
        },

        {
            ":localhost/?#",
            MNHTTPC_MESSAGE_SCHEME_HTTP,
            NULL,
            NULL,
            "",
            "localhost",
            "/",
            "",
            "#",
        },





    };
    //const char *uris[] = {
    //    "http://localhost",
    //    "localhost:80",
    //    "localhost:443",
    //    "https://localhost",
    //    "user@localhost",
    //    "user:@localhost",
    //    "user:password@localhost",
    //    ":password@localhost",
    //    ":@localhost",
    //    "localhost:",
    //    "localhost?",
    //    "localhost?#",
    //    "localhost#",
    //    "asd@",
    //    ":",
    //    ":@",
    //    "https://:@",
    //    "https://@",
    //    "https://:",
    //    "localhost/?",
    //    "localhost/#",
    //    "localhost/#qwe",
    //    "localhost/?#qwe",
    //    "localhost/?asd#qwe",


    //};

#define ASSERT_BYTES_FIELD(f)                                          \
    TRACE(#f " expect %s found %s", data[i].f, BDATASAFE(uri.f));      \
    if (data[i].f == NULL) {                                           \
        assert(uri.f == NULL);                                         \
    } else {                                                           \
        assert(strcmp(data[i].f, BCDATA(uri.f)) == 0);                 \
    }                                                                  \



    for (i = 0; i < countof(data); ++i) {
        mnhttp_uri_init(&uri);
        mnhttp_uri_parse(&uri, data[i].in);
        TRACE(">>> %s", data[i].in);

        TRACE("scheme %s",
              uri.scheme == MNHTTPC_MESSAGE_SCHEME_HTTP ? "HTTP" :
              uri.scheme == MNHTTPC_MESSAGE_SCHEME_HTTPS ? "HTTPS" :
              "<undef>");
        assert(data[i].scheme == uri.scheme);

        ASSERT_BYTES_FIELD(user);
        ASSERT_BYTES_FIELD(password);
        ASSERT_BYTES_FIELD(host);
        ASSERT_BYTES_FIELD(port);
        ASSERT_BYTES_FIELD(path);
        ASSERT_BYTES_FIELD(qstring);
        ASSERT_BYTES_FIELD(fragment);

        //TRACE("user %s", BDATASAFE(uri.user));
        //TRACE("password %s", BDATASAFE(uri.password));
        //TRACE("host %s", BDATASAFE(uri.host));
        //TRACE("port %s", BDATASAFE(uri.port));
        //TRACE("path %s", BDATASAFE(uri.path));
        //TRACE("qstring %s", BDATASAFE(uri.qstring));
        //TRACE("fragment %s", BDATASAFE(uri.fragment));
        TRACE("<<<");



        mnhttp_uri_fini(&uri);
    }
}


int
main(UNUSED int argc, UNUSED char **argv)
{
    test0();
    return 0;
}
