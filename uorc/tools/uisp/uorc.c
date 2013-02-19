#include <poll.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <unistd.h>
//#include <net/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/time.h>

#include "uorc.h"

// useconds
static int64_t timestamp_now()
{
    struct timeval tv;
    gettimeofday (&tv, NULL);
    return (int64_t) tv.tv_sec * 1000000 + tv.tv_usec;
}

static void print_buffer(void *vbuf, int buflen)
{
    uint8_t *buf = (uint8_t*) vbuf;

    for (int i = 0; i < buflen; i++) {
        if ((i&15)==0)
            printf("%04x: ", i);
        printf("%02x ", buf[i]&0xff);
        if ((i&15)==15 || i+1 == buflen)
            printf("\n");
    }
}

uorc_t *uorc_create(const char *ipaddr)
{
    uorc_t *uorc = calloc(sizeof(uorc_t), 1);
    int port = 2378;

    uorc->sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (uorc->sock < 0) {
        perror("socket");
        return NULL;
    }

    struct sockaddr_in listen_addr;
    memset(&listen_addr, 0, sizeof(struct sockaddr_in));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = INADDR_ANY; // ephemeral port, please
    listen_addr.sin_addr.s_addr = INADDR_ANY;

    int res = bind(uorc->sock, (struct sockaddr*) &listen_addr, sizeof(struct sockaddr_in));
    if (res < 0) {
        perror("bind");
        return NULL;
    }

    // fill in address structure
    memset(&uorc->send_addr, 0, sizeof(struct sockaddr_in));
    uorc->send_addr.sin_family = AF_INET;
    uorc->send_addr.sin_port = htons(port);

    struct hostent *host = gethostbyname(ipaddr);
    if (host == NULL) {
        perror("gethostbyname");
        return NULL;
    }

    memcpy(&uorc->send_addr.sin_addr.s_addr, host->h_addr, host->h_length);

    uorc->timeoutms = 20;

    return uorc;
}

// resplen is length of resp buffer on input. Actual rx size on output.
int uorc_transaction_once(uorc_t *uorc, uint32_t cmd, const uint8_t *params, int paramslen, uint8_t *resp, int *resplen, int64_t *uorc_utime)
{
    int send_packet_len = 20 + paramslen;
    uint8_t send_packet[send_packet_len];

    ////////////////////////////////////////////////////
    // build the packet
    send_packet[0] = 0x0c; // magic header
    send_packet[1] = 0xed;
    send_packet[2] = 0x00;
    send_packet[3] = 0x02;

    // XXX not thread safe
    int tid = uorc->next_tid;
    uorc->next_tid++;

    send_packet[4] = (tid >> 24) & 0xff;
    send_packet[5] = (tid >> 16) & 0xff;
    send_packet[6] = (tid >> 8) & 0xff;
    send_packet[7] = (tid >> 0) & 0xff;

    int64_t utime = timestamp_now();
    send_packet[8] = (utime >> 56) & 0xff;
    send_packet[9] = (utime >> 48) & 0xff;
    send_packet[10] = (utime >> 40) & 0xff;
    send_packet[11] = (utime >> 32) & 0xff;
    send_packet[12] = (utime >> 24) & 0xff;
    send_packet[13] = (utime >> 16) & 0xff;
    send_packet[14] = (utime >> 8) & 0xff;
    send_packet[15] = (utime >> 0) & 0xff;

    send_packet[16] = (cmd >> 24) & 0xff;
    send_packet[17] = (cmd >> 16) & 0xff;
    send_packet[18] = (cmd >> 8) & 0xff;
    send_packet[19] = (cmd >> 0) & 0xff;

    if (paramslen > 0)
        memcpy(&send_packet[20], params, paramslen);

    ////////////////////////////////////////////////////
    // now actually send.

//    print_buffer(send_packet, send_packet_len);

    int res = sendto(uorc->sock, send_packet, send_packet_len, 0,
                     (struct sockaddr*) &uorc->send_addr, sizeof(uorc->send_addr));
    if (res < 0) {
        perror("sendto");
        return res;
    }

    ////////////////////////////////////////////////////
    // read the response
    struct pollfd pfd;

    pfd.fd = uorc->sock;
    pfd.events = POLLIN;
    res = poll(&pfd, 1, uorc->timeoutms);
    if (res < 0) {
        perror("poll");
        return res;
    }

    // timeout
    if (res == 0) {
        printf("timeout\n");
        return -1;
    }

    // ready to read.
    struct sockaddr_ll srcaddr;
    memset(&srcaddr, 0, sizeof(srcaddr));
    socklen_t srcaddrlen = sizeof(srcaddr);

    // all packets fit into a single frame...
    int rxbuf_len = 2048;
    uint8_t rxbuf[rxbuf_len];

read_again:
    ; // silence compiler warning

    ssize_t len = recvfrom(uorc->sock, (void*) rxbuf, rxbuf_len, 0,
                           (struct sockaddr*) &srcaddr, &srcaddrlen);

    if (len < 0) {
        perror("recvfrom");
        return len;
    }

    if (len < 20) {
        printf("packet too short\n");
        return -1;
    }

//    print_buffer(rxbuf, len);

    // bad header
    if (rxbuf[0] != 0x0c || rxbuf[1] != 0xed || rxbuf[2] != 0x00 || rxbuf[3] != 0x01) {
        printf("bad header\n");
        goto read_again;
    }

    if (rxbuf[4] != send_packet[4] || rxbuf[5] != send_packet[5] ||
        rxbuf[6] != send_packet[6] || rxbuf[7] != send_packet[7]) {
        printf("wrong transaction\n");
        goto read_again;
    }

    if (rxbuf[16] != send_packet[16] || rxbuf[17] != send_packet[17] ||
        rxbuf[18] != send_packet[18] || rxbuf[19] != send_packet[19]) {
        printf("wrong command\n");
        goto read_again;
    }

    *uorc_utime = 0;
    for (int i = 0; i < 8; i++) {
        *uorc_utime <<= 8;
        *uorc_utime += (send_packet[8+i] & 0xff);
    }

    int cpsz = len - 20;
    if (*resplen < cpsz) {
        printf("resp buffer too small\n");
        return -1;
    }

    *resplen = cpsz;
    memcpy(resp, &rxbuf[20], *resplen);

    return 0;
}

void uorc_destroy(uorc_t *uorc)
{
    close(uorc->sock);
    free(uorc);
}
