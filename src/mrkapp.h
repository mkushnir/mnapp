#ifndef MRKAPP_H_DEFINED
#define MRKAPP_H_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

const char *mrkapp_diag_str(int);

int local_server(int, void **);
void local_server_shutdown(void);
void daemonize(const char *, const char *, const char *);
uint64_t fasthash(uint64_t, const unsigned char *, size_t);

#ifdef __cplusplus
}
#endif
#endif /* MRKAPP_H_DEFINED */
