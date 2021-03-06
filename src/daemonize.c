#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

#include <mncommon/util.h>
#include <mncommon/dumpm.h>
#include "diag.h"

static void
myfork(void)
{
    pid_t pid;

    if ((pid = fork()) == -1) {
        FAIL("fork");
    } else if (pid > 0) {
        /* parent */
        _exit(0);
    } else {
        /* child */
    }
}

void
daemonize(const char *pidfile,
              const char *stdoutfile,
              const char *stderrfile)
{
    mode_t old_umask;
    int i;
    int nfiles;
    struct rlimit rlim;
    int fd;

    /* perform first fork */
    myfork();

    /* create a clean session in the first child */
    old_umask = umask(0);
    if (setsid() == -1) {
        FAIL("setsid");
    }

    /* perform second fork */
    myfork();

    /* close all possibly open file descriptors */
    if (getrlimit(RLIMIT_NOFILE, &rlim) == 0) {
        if (rlim.rlim_cur == RLIM_INFINITY) {
            nfiles = 1024;
        } else {
            nfiles = (int) rlim.rlim_cur;
        }
    } else {
        nfiles = 1024;
    }
    for (i = 0; i < nfiles; ++i) {
        close(i);
    }
    /* now open stdin/stdout/stderr */
    fd = open("/dev/null", O_RDWR);
    assert(fd == 0);
    if (stdoutfile == NULL) {
        stdoutfile = "/dev/null";
    }
    if (stderrfile == NULL) {
        stderrfile = "/dev/null";
    }
    if (strcmp(stdoutfile, "/dev/null") == 0) {
        dup2(0, 1);
    } else {
        fd = open(stdoutfile, O_WRONLY|O_CREAT|O_APPEND, 0644);
        assert(fd == 1);
    }
    if (strcmp(stderrfile, stdoutfile) == 0) {
        dup2(1, 2);
    } else {
        fd = open(stderrfile, O_WRONLY|O_CREAT|O_APPEND, 0644);
        assert(fd == 2);
    }


    umask(old_umask);

    if (pidfile != NULL) {
        char buf[32];

#ifdef O_EXLOCK
        if ((fd = open(pidfile, O_WRONLY|O_CREAT|O_TRUNC|O_NONBLOCK|O_EXLOCK, 0644)) < 0) {
            exit(1);
        }
#else
        if ((fd = open(pidfile, O_WRONLY|O_CREAT|O_NONBLOCK, 0644)) < 0) {
            exit(1);
        }
        if (flock(fd, LOCK_EX|LOCK_NB) == -1) {
            exit(1);
        }
        if (lseek(fd, 0, SEEK_END) < 0) {
            exit(1);
        }
#endif
        snprintf(buf, countof(buf), "%d", getpid());
        if (write(fd, buf, strlen(buf)) <= 0) {
            FAIL("write");
        }
        if (fsync(fd) != 0) {
            FAIL("fsync");
        }
        if (chdir("/") != 0) {
            FAIL("chdir");
        }
    }
}
