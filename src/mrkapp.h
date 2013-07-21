#ifndef MRKAPP_H_DEFINED
#define MRKAPP_H_DEFINED

#ifdef __cplusplus
extern "C" {
#endif


int mrk_local_server(int, void **);
void mrk_local_server_shutdown(void);

#ifdef __cplusplus
}
#endif
#endif /* MRKAPP_H_DEFINED */
