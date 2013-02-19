#ifndef _UORC_H
#define _UORC_H

#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>

typedef struct
{
    int sock;
    struct sockaddr_in send_addr;

    uint32_t next_tid;

    int timeoutms;

} uorc_t;

uorc_t *uorc_create(const char *ipaddr);

int uorc_transaction_once(uorc_t *uorc, uint32_t cmd, const uint8_t *params,
                          int paramslen, uint8_t *resp, int *resplen, int64_t *uorc_utime);

void uorc_destroy(uorc_t *uorc);

#endif
