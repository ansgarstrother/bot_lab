#ifndef _SERIAL_H
#define _SERIAL_H

#include <nkern.h>

iop_t *serial0_configure(int pclk, int baud);

void serial0_putc_spin(char c);
void serial0_putc(char c);

char serial0_getc();

iop_t *serial1_configure_halfduplex(int pclk, int baud);
void serial1_halfduplex_enable_tx(int enable);
int serial1_halfduplex_write(const char *msg, int len);
void serial1_read_wait_setup(nkern_wait_t *w);
int serial1_read_available();

int serial1_getc_nonblock();

#endif
