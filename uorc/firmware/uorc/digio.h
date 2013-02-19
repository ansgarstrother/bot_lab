#ifndef _DIGIO_H
#define _DIGIO_H

void digio_init();
uint32_t digio_get();
uint32_t digio_get_config();
void digio_configure(int port, int iodir, int pullup);
void digio_set(int port, int v);
int digio_get_estop();

#endif
