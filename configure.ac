AC_INIT(mrkapp, 0.1, markiyan.kushnir@gmail.com)
if test -n "$USE_GCC"
then
    if test "$USE_GCC" = "46" && which gcc46
    then
        CC="gcc46"
        CXX="c++46"
        CPP="cpp"
    else
        if which gcc
        then
            CC="gcc"
            CXX="g++"
            CPP="cpp"
        else
            echo "Cannot find gcc"
            exit 1
        fi
    fi
else
    if which clang >/dev/null
    then
        CC="clang -Qunused-arguments -fcolor-diagnostics"
        CXX="clang++ -Qunused-arguments -fcolor-diagnostics"
        CPP="clang-cpp"
        clang_version=`clang --version | grep version | awk '{print $4;}'`
        clang_version_mjr=`echo $clang_version | awk -F. '{print $1;}'`
        clang_version_mnr=`echo $clang_version | awk -F. '{print $2;}'`
        if test "$clang_version_mjr" = "3" -a "$clang_version_mnr" -lt 3
        then
            AC_SUBST(CLANG_DEBUG, ['-ftrapv -fcatch-undefined-behavior'])
        else
            # we assume all others are 3.3+
            AC_SUBST(CLANG_DEBUG, ['-fsanitize=undefined-trap -fsanitize-undefined-trap-on-error'])
        fi
    else
        CC="cc"
        CXX="c++"
        CPP="cpp"
    fi
fi

AC_CANONICAL_HOST
AC_CANONICAL_TARGET
case "$target_os" in
    *linux*)
        AC_SUBST(_GNU_SOURCE_MACRO, ['-D_GNU_SOURCE'])
    ;;
    *)
        AC_SUBST(_GNU_SOURCE_MACRO)
    ;;
esac

AC_CONFIG_MACRO_DIR([m4])

AC_LANG(C)

AC_PROG_CC
AC_PROG_CXX
AM_PROG_AR
AC_PROG_RANLIB
AC_PROG_INSTALL

LT_INIT
AM_INIT_AUTOMAKE
AM_SILENT_RULES([yes])

dnl AM_MAINTAINER_MODE

AC_CONFIG_FILES([Makefile src/Makefile test/Makefile man/Makefile])
AC_CONFIG_HEADERS(config.h)

AC_CHECK_HEADERS([syslog.h fcntl.h netdb.h sys/socket.h sys/time.h sys/file.h ])
AC_CHECK_HEADER_STDBOOL


AC_C_CONST
AC_C_INLINE

AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_UINT64_T

AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_FORK
AC_CHECK_FUNCS([dup2 memset socket memchr strcasecmp strchr strdup strncasecmp strstr strtol])


configure.ac: warning: missing AC_CHECK_FUNCS([memchr]) wanted by: src/mrkhttp.c:1112
configure.ac: warning: missing AC_CHECK_FUNCS([strcasecmp]) wanted by: src/mrkhttp.c:615
configure.ac: warning: missing AC_CHECK_FUNCS([strchr]) wanted by: src/local_server.c:129
configure.ac: warning: missing AC_CHECK_FUNCS([strdup]) wanted by: src/local_server.c:138
configure.ac: warning: missing AC_CHECK_FUNCS([strncasecmp]) wanted by: src/mrkhttp.c:795
configure.ac: warning: missing AC_CHECK_FUNCS([strstr]) wanted by: src/mrkhttp.c:156
configure.ac: warning: missing AC_CHECK_FUNCS([strtol]) wanted by: src/mrkhttp.c:618
configure.ac: warning: missing AC_CHECK_HEADERS([sys/file.h]) wanted by: src/daemonize.c:5
configure.ac: warning: missing AC_CHECK_HEADER_STDBOOL wanted by: src/mrkapp.h:28
configure.ac: warning: missing AC_TYPE_SSIZE_T wanted by: src/mrkhttp.c:751
configure.ac: warning: missing AC_TYPE_UINT64_T wanted by: src/mnhttpc.h:70


AC_CHECK_MEMBER([struct sockaddr_un.sun_len],
                AC_DEFINE([HAVE_SUN_LEN], [1], [Define if sockaddr_un.sun_len exists]))

AC_PRESERVE_HELP_ORDER

AC_SUBST(MRKCOMMON_LOCAL_CFLAGS, [''])
AC_SUBST(MRKCOMMON_LOCAL_LDFLAGS, [''])

AC_SUBST(MRKTHR_LOCAL_CFLAGS, [''])
AC_SUBST(MRKTHR_LOCAL_LDFLAGS, [''])

AC_ARG_ENABLE(debug,
              AC_HELP_STRING([--enable-debug],
                             [Enable debug build (default=no)]),
              [AM_CONDITIONAL([DEBUG], [enable_debug=yes])],
              [AM_CONDITIONAL([DEBUG], [test "$enable_debug" = "yes"])])

# pass LDFLAGS=-L${LOCALBASE}/lib in the environment
AC_CHECK_LIB(mrkcommon, _fini, [], [AC_MSG_FAILURE(libmrkcommon.so is required. Please find it at git@github.com:mkushnir/mrkcommon.git)]) 
AC_CHECK_LIB(mrkthr, _fini, [], [AC_MSG_FAILURE(libmrkthr.so is required. Please find it at git@github.com:mkushnir/mrkthr.git)]) 

AC_OUTPUT
