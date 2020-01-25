mnapp
======

Application components.

The following components are implemented in this library:

daemonize
---------

The _daemonize()_ function turns the running process into a daemon.  It
optionally handles process ID file, as well as redirection of standard
output and standard error file descriptors to the user-provided locations.

It is an alternative API to POSIX.1 _daemon(3)_.


local\_server
-----------

This module offers simple TCP or Unix socket server based on the async
event-driven threading library
[mnthr](http://github.com/mkushnir/mnthr).

The server context is contained in the `mnapp_tcp_server_t` structure,
and should be managed by `mnapp_tcp_server_init()` and
`mnapp_tcp_server_fini()` pair.

The server accepts a single handler of type `mnapp_tcp_server_cb_t`, and
a `void *` user data.

Typical workflow within the context lifecycle is:
`mnapp_tcp_server_start()` -> `mnapp_tcp_server_serve()` ->
`mnapp_tcp_server_stop()`.


mnhttp
-------

This module provides basic parsers of the [HTTP/1.1
protocol](https://www.ietf.org/rfc/rfc2616.txt).

`http_parse_request()` is a request parser, and `http_parse_response()` is
a response parser.  Both accept three `http_cb_t` callbacks that will be
called upon the request/status line, each header found, and the body, if
present.


The parsing context should be composed of a `bytestream_t` instance
initialized for reading, and an `http_ctx_t *` instance passed as the
`udata` field of the bytestream:

```C
    bytestream_t in;
    http_ctx_t *ctx;

    bytestream_init(&in, 1024);
    ctx = http_ctx_new();
    in.udata = ctx;
```

The module also provides basic HTTP writing functions that are essentially
formatting wrappers over `bytestream_t`: `http_start_request()`,
`http_start_response()`, `http_add_header_field()`,
`http_end_of_header()`, `http_add_body()`.


Combined request/response pattern can look like this (partial example):
```C
    bytestream_t in, out;
    http_ctx_t *ctx;

    bytestream_init(&in, 1024);
    in.read_more = mnthr_bytestream_read_more;
    ctx = http_ctx_new();
    in.udata = ctx;

    res = http_parse_request(fd, &in, myline, myheader, mybody, NULL);
    /* check res */

    bytestream_init(&out, 1024);
    out.write = mnthr_bytestream_write;

    (void)http_start_response(&out, 200, "OK");
    (void)http_end_of_header(&out);
    (void)http_add_body(&out, "test\n", 5);

    res = bytestream_produce_data(&out, fd);
    /* check res */

    bytestream_fini(&in);
    bytestream_fini(&out);
    close(fd);
    http_ctx_destroy(&ctx);
```
