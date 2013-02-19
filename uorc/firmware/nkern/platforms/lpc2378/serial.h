#ifndef _SERIAL_H
#define _SERIAL_H

int serial_init(int port);
int serial_configure(int port, int pclk, int baud);

void serial_putc_spin(int port, char c);
int  serial_getc_spin(int port);

long serial_write(void *user, const void *data, int len);
long serial_read(void *user, void *data, int len);
long serial_read_timeout(void *user, void *data, int len, uint64_t usecs);
void serial_putc(int port, char c);
int serial_getc(int port);

// convenience functions
void serial_putc_0(char c);
void serial_putc_1(char c);
char serial_getc_0();
char serial_getc_1();

#endif
