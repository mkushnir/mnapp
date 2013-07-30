#ifndef MRKAPP_H_DEFINED
#define MRKAPP_H_DEFINED

#ifdef __cplusplus
extern "C" {
#endif


int local_server(int, void **);
void local_server_shutdown(void);
void daemonize(const char *, const char *, const char *);

#ifdef __cplusplus
}
#endif
#endif /* MRKAPP_H_DEFINED */
