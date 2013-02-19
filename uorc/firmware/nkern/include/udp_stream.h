#ifndef _UDP_PUTC_H
#define _UDP_PUTC_H

int udp_stream_init(int _local_port);

void udp_stream_putc(char c);
char udp_stream_getc();

#endif
