#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

#include "lm3s8962.h"
#include "luminary.h"
#include <nkern.h>
#include <net.h>

static int readline(iop_t *iop, char *buf, int maxlen)
{
    int pos = 0;

    while (pos < maxlen) {
        int c = getc_iop(iop);
        if (c < 0)
            return -1;

        if (c=='\r' || c=='\n')
            break;
        buf[pos++] = c;
    }
    buf[pos] = 0;

    return pos;
}

void shell_client_task(void *arg)
{
    tcp_connection_t *con = (tcp_connection_t*) arg;

    iop_t *iop = tcp_get_iop(con);

    pprintf(iop, "uOrc shell\n\n");

    while (1) {
        char line[64];

        pprintf(iop, "%% ");
        flush_iop(iop);

        int len = readline(iop, line, sizeof(line));
        if (len < 0)
            break;

        if (len == 0)
            continue;

        if (!strcmp(line, "stat")) {
            nkern_print_stats(iop);
            udp_print_stats(iop);
            tcp_print_stats(iop);
        } else if (!strcmp(line, "test")) {
            pprintf(iop, "pi: %f\n", 3.1415926535897932384626);
        } else {
            pprintf(iop, "Unknown command: ");
            for (int i = 0; i < len; i++)
                pprintf(iop, "%02x ", line[i]);
        }
    }

    tcp_close(con);
    tcp_free(con);
}

void shell_task(void *arg)
{
    tcp_listener_t *tcplistener = tcp_listen(23);

    while (1) {
        tcp_connection_t *con = tcp_accept(tcplistener);

        nkern_task_create("shell_client",
                          shell_client_task, con,
                          NKERN_PRIORITY_LOW, 2048);
    }

}

void shell_init()
{
    nkern_task_create("shell",
                      shell_task, NULL,
                      NKERN_PRIORITY_NORMAL, 1024);
}
