#include <sys/time.h>
#include <errno.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <netdb.h>
#include <time.h>
#include <assert.h>

#include "getopt.h"
#include "uorc.h"

#define MTU 1518

#define ISP_PING 0
#define ISP_ERASE 1
#define ISP_WRITE 2
#define ISP_READ 3
#define ISP_RESET 4

typedef struct
{
    int eth_sock;
    int if_index;
    uint8_t src_mac[6]; // MACean address corresponding to dev if_index

    uint32_t next_tid;
    getopt_t *gopt;
} uisp_t;

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

int eth_send(uisp_t *uisp, uint8_t *dest_mac, uint32_t type, uint8_t *buf, ssize_t buflen)
{
    struct sockaddr_ll saddr;
    memset(&saddr, 0, sizeof(struct sockaddr_ll));

    char packet[MTU];
    memset(packet, 0, sizeof(packet));

    saddr.sll_family = AF_PACKET;
    saddr.sll_protocol = htons(type);
    saddr.sll_ifindex = uisp->if_index;
//     saddr.sll_hatype = ARPHRD_ETHER; // rx only
//     saddr.sll_pkttype = PACKET_BROADCAST; // rx only
    saddr.sll_halen = ETH_ALEN;

    struct ethhdr *hdr = (struct ethhdr*) packet;
    memcpy(&hdr->h_dest, dest_mac, ETH_ALEN);
    memcpy(&hdr->h_source, uisp->src_mac, ETH_ALEN);
    hdr->h_proto = htons(ETH_P_CUST);

    memcpy(packet + ETH_HLEN, buf, buflen);

    int res = sendto(uisp->eth_sock, packet, ETH_HLEN + buflen, 0,
                     (struct sockaddr*) &saddr, sizeof(saddr));

    if (getopt_get_bool(uisp->gopt, "verbose")) {
        printf("send: %d\n", res);
        print_buffer(packet, 14+buflen);
    }
    //////////////

    return 0; // no error
}

// receive a whole ethernet frame, including header.
int eth_recv(uisp_t *uisp, uint8_t *src_addr, uint8_t *dest_addr, uint32_t *type,
             uint8_t *buf, ssize_t buflen, int timeoutms)
{
    struct pollfd pfd;

    pfd.fd = uisp->eth_sock;
    pfd.events = POLLIN;
    int res = poll(&pfd, 1, timeoutms);
    if (res < 0) {
        perror("poll");
        return -1;
    }

    if (res == 0) {
        errno = ETIMEDOUT;
        return 0; // timeout
    }

    // ready to read.
    char packet[MTU];

    struct sockaddr_ll srcaddr;
    memset(&srcaddr, 0, sizeof(srcaddr));
    socklen_t srcaddrlen = sizeof(srcaddr);

    ssize_t len = recvfrom(uisp->eth_sock, packet, MTU, 0,
                           (struct sockaddr*) &srcaddr, &srcaddrlen);
    if (len < 0) {
        perror("recvfrom");
    }

/*
    if (getopt_get_bool(uisp->gopt, "verbose")) {
        printf("addr (%d, %x): \n", srcaddr.sll_ifindex, htons(srcaddr.sll_protocol));
        for (int i = 0; i < 8; i++)
            printf("%02x ", srcaddr.sll_addr[i]);
        printf("\n");
    }
*/

    for (int i = 0; i < 6; i++) {
        dest_addr[i] = packet[i];
        src_addr[i] = packet[6+i];
    }

    *type = (packet[12]<<8) + packet[13];

    assert(len >= 14);

    if (len - 14 > buflen) {
        printf("packet too long (%d)\n", (int) len);
        return 0;
    }

    memcpy(buf, &packet[14], len - 14);
    return len - 14;
}

int isp_transaction(uisp_t *uisp, uint32_t cmd, uint8_t *params, uint32_t paramlen, uint8_t *resp, uint32_t resplen)
{
    uint32_t send_packet_len = paramlen+8;
    uint8_t send_packet[send_packet_len];

retry:
    ; // silence warning

    uint32_t tid = uisp->next_tid++;

    send_packet[0] = (tid>>24)&0xff;
    send_packet[1] = (tid>>16)&0xff;
    send_packet[2] = (tid>>8)&0xff;
    send_packet[3] = (tid>>0)&0xff;

    send_packet[4] = (cmd>>24)&0xff;
    send_packet[5] = (cmd>>16)&0xff;
    send_packet[6] = (cmd>>8)&0xff;
    send_packet[7] = (cmd>>0)&0xff;

    memcpy(&send_packet[8], params, paramlen);

    uint8_t dest_mac[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

    long start_utime = timestamp_now();

    if (eth_send(uisp, dest_mac, ETH_P_CUST, send_packet, send_packet_len)) {
        perror("eth_send");
        return -1;
    }

    // keep reading packets until we either get what we want, or a timeout occurs
    while (1) {
        uint32_t recv_packet_len = MTU; //resplen + 8;
        uint8_t recv_packet[recv_packet_len];

        uint8_t src_addr[6], dest_addr[6];
        uint32_t type;

        // we look at the actual amount of elapsed time, since when
        // there's other network traffic, our call to eth_send will
        // never time out even if the message wasn't received.
        long now_utime = timestamp_now();
        double dt = (now_utime - start_utime) / 1.0E6;
        if (dt > 0.1) {
            printf("timeout\n");
            goto retry;
        }

        int recv_actual_len = eth_recv(uisp, src_addr, dest_addr, &type, recv_packet, recv_packet_len, 200);
        if (recv_actual_len < 0) {
            perror("eth_recv");
            return -1;
        }

        if (recv_actual_len == 0) {
            printf("timeout\n");
            goto retry;
        }

        if (type != ETH_P_CUST)
            continue;

        if (getopt_get_bool(uisp->gopt, "verbose"))
            print_buffer(recv_packet, recv_actual_len);

        if (recv_packet[0]!=send_packet[0] || recv_packet[1]!=send_packet[1] ||
            recv_packet[2]!=send_packet[2] || recv_packet[3]!=send_packet[3]) {
            printf("wrong transaction id\n");
            continue;
        }

        if (recv_packet[4]!=(0x80 | send_packet[4]) || recv_packet[5]!=send_packet[5] ||
            recv_packet[6]!=send_packet[6] || recv_packet[7]!=send_packet[7]) {
            printf("wrong command flags\n");
            continue;
        }

        if (recv_actual_len - 8 > resplen) {
            printf("Not enough space\n");
            return -1;
        }

        memcpy(resp, &recv_packet[8], recv_actual_len - 8);

        // success!
        return recv_actual_len - 8;
    }
}


// 7 netdevice
// packet
int main(int argc, char *argv[])
{
    uisp_t *uisp = calloc(sizeof(uisp_t), 1);
    uisp->gopt = getopt_create();
    getopt_add_string(uisp->gopt, 'i', "interface", "eth0", "Interface");
    getopt_add_string(uisp->gopt, '\0', "uorc", "192.168.237.7", "uorc IP addr");
    getopt_add_bool(uisp->gopt, 'v', "verbose", 0, "verbose debugging info");
    getopt_add_bool(uisp->gopt, 'h', "help", 0, "show this help");
    getopt_add_bool(uisp->gopt, '\0', "ping-test", 0, "non-destructive ping test");

    getopt_add_string(uisp->gopt, 'p', "phases", "ewvr", "erase/write/verify/reset phases");

    if (getopt_parse(uisp->gopt, argc, argv, 1) || getopt_get_bool(uisp->gopt, "help")) {
        getopt_do_usage(uisp->gopt);
        return 1;
    }

    if (1) {
        const char *uorc_ip = getopt_get_string(uisp->gopt, "uorc");

        if (strlen(uorc_ip) > 0) {
            uorc_t *uorc = uorc_create(uorc_ip);

            for (int trial = 0; trial < 5; trial++) {
                printf("Attempting to switch uorc into programming mode... (%d)\n", trial);

                uint8_t resp[2048];
                int resplen = sizeof(resp);
                int64_t uorc_utime;

                int res = uorc_transaction_once(uorc, 0xdeae, NULL, 0, resp, &resplen, &uorc_utime);

                if (res == 0)
                    break;

                usleep(100000);
            }

            uorc_destroy(uorc);
        }
    }

    /////////////////////////
    // load the .bin file
    if (g_ptr_array_size(uisp->gopt->extraargs) != 1) {
        printf("You must specify exactly one .bin file (%d)\n", g_ptr_array_size(uisp->gopt->extraargs));
        return -1;
    }

    const char *path = g_ptr_array_index(uisp->gopt->extraargs, 0);
    struct stat st;
    memset(&st, 0, sizeof(struct stat));
    if (stat(path, &st)) {
        perror("stat");
        return -1;
    }

    if (strstr(path, ".bin") == NULL) {
        printf("Refusing to use firmware image not ending in .bin\n");
        exit(1);
    }

    uint8_t *image = calloc(1, st.st_size);
    uint32_t image_size = st.st_size;

    if (image_size > 255*1024) {
        printf("This image is too big. Exiting.\n");
        exit(1);
    }

    if (1) {
        FILE *f = fopen(path, "rb");
        if (fread(image, 1, image_size, f) != image_size) {
            perror("fread");
            return -1;
        }

        printf("read %d byte image\n", image_size);
    }

    /////////////////////////
    // create raw ethernet frame
    uisp->eth_sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    if (1) {
        if (uisp->eth_sock < 0) {
            perror("socket");
            return 1;
        }

        struct ifreq ifr;
        memset(&ifr, 0, sizeof(struct ifreq));

        const char *ifname = getopt_get_string(uisp->gopt, "interface");
        strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

        if (ioctl(uisp->eth_sock, SIOCGIFINDEX, &ifr) < 0) {
            perror("get interface info");
            return 1;
        }

        uisp->if_index = ifr.ifr_ifindex;

        if (ioctl(uisp->eth_sock, SIOCGIFHWADDR, &ifr) < 0) {
            perror("get interface info (2)");
            return 1;
        }

        for (int i = 0; i < 6; i++)
            uisp->src_mac[i] = ifr.ifr_hwaddr.sa_data[i]&0xff;
    }

    /////////////////////////
    if (getopt_get_bool(uisp->gopt, "ping-test")) {

        while (1) {
            uint8_t pingdata[64];
            for (int i = 0; i < sizeof(pingdata); i++)
                pingdata[i] = i;

            uint8_t resp[1024];
            memset(resp, 0, sizeof(resp));

            int res = isp_transaction(uisp, ISP_PING, pingdata, sizeof(pingdata), resp, sizeof(resp));
            if (res != sizeof(pingdata)) {
                printf("bad ping");
            }

            if (memcmp(resp, pingdata, sizeof(pingdata)))
                printf("-");
            else
                printf("+");
            fflush(NULL);
        }
    }


    /////////////////////////
    if (1) {
        printf("erasing...");
        fflush(NULL);

        for (uint32_t addr = 0; addr <= image_size; addr += 1024) {
            uint8_t erasedata[4];

            erasedata[0] = (addr>>24)&0xff;
            erasedata[1] = (addr>>16)&0xff;
            erasedata[2] = (addr>>8)&0xff;
            erasedata[3] = (addr>>0)&0xff;

            uint8_t resp[1024];
            int res = isp_transaction(uisp, ISP_ERASE, erasedata, sizeof(erasedata), resp, sizeof(resp));

            if (res < 0) {
                printf("erase failed: res=%d\n", res);
                return 1;
            }

            uint32_t v = (resp[0]<<24) + (resp[1]<<16) + (resp[2]<<8) + (resp[3]<<0);
            if (v != 0) {
                printf("erase failed: v=%d\n", v);
                return 1;
            }

            printf("\rerasing...   (%d kB)", addr / 1024);
            fflush(NULL);
        }

        printf("\n");
    }

    /////////////////////////
    if (1) {
        printf("writing...");
        fflush(NULL);

        for (uint32_t addr = 0; addr <= image_size; addr += 1024) {

            int len = 1024;
            if (len > image_size - addr)
                len = image_size - addr;

            uint8_t writedata[8 + len];
            memset(writedata, 255, sizeof(writedata));

            writedata[0] = (addr>>24)&0xff;
            writedata[1] = (addr>>16)&0xff;
            writedata[2] = (addr>>8)&0xff;
            writedata[3] = (addr>>0)&0xff;
            writedata[4] = (len>>24)&0xff;
            writedata[5] = (len>>16)&0xff;
            writedata[6] = (len>>8)&0xff;
            writedata[7] = (len>>0)&0xff;

            memcpy(&writedata[8], &image[addr], len);

            uint8_t resp[1024];
            int res = isp_transaction(uisp, ISP_WRITE, writedata, sizeof(writedata), resp, sizeof(resp));

            if (res < 0) {
                printf("failed: res=%d\n", res);
                return 1;
            }

            uint32_t v = (resp[0]<<24) + (resp[1]<<16) + (resp[2]<<8) + (resp[3]<<0);
            if (v != 0) {
                printf("failed: v=%d\n", v);
                return 1;
            }

            printf("\rwriting...   (%d kB)", addr / 1024);
            fflush(NULL);
        }

        printf("\n");
    }

    /////////////////////////
    if (1) {
        printf("verifying...");

        for (uint32_t addr = 0; addr <= image_size; addr += 1024) {

            int len = 1024;
            if (len > image_size - addr)
                len = image_size - addr;

            uint8_t readdata[8];

            readdata[0] = (addr>>24)&0xff;
            readdata[1] = (addr>>16)&0xff;
            readdata[2] = (addr>>8)&0xff;
            readdata[3] = (addr>>0)&0xff;
            readdata[4] = (len>>24)&0xff;
            readdata[5] = (len>>16)&0xff;
            readdata[6] = (len>>8)&0xff;
            readdata[7] = (len>>0)&0xff;

            uint8_t resp[4 + len];
            int res = isp_transaction(uisp, ISP_READ, readdata, sizeof(readdata), resp, sizeof(resp));

            if (res < 0) {
                printf("failed: res=%d\n", res);
                return 1;
            }

            uint32_t v = (resp[0]<<24) + (resp[1]<<16) + (resp[2]<<8) + (resp[3]<<0);
            if (v != 0) {
                printf("failed: v=%d\n", v);
                return 1;
            }

            if (memcmp(&image[addr], &resp[4], len)) {
                printf("mismatch at %08x\n", addr);
                return 1;
            }

            printf("\rverifying... (%d kB)", addr / 1024);
            fflush(NULL);
        }

        printf("\n");
    }

    /////////////////////////
    if (1) {
        printf("resetting...");
        fflush(NULL);

        uint8_t rebootdata[0];

        uint8_t resp[1024];
        int res = isp_transaction(uisp, ISP_RESET, rebootdata, sizeof(rebootdata), resp, sizeof(resp));

        if (res < 0) {
            printf("failed: res=%d\n", res);
            return 1;
        }

        uint32_t v = (resp[0]<<24) + (resp[1]<<16) + (resp[2]<<8) + (resp[3]<<0);
        if (v != 0) {
            printf("failed: v=%d\n", v);
            return 1;
        }

        printf(" done!\n");
    }
}
