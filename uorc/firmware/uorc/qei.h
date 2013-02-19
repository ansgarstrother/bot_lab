#ifndef _QEI_H
#define _QEI_H

void qei_init();
uint32_t qei_get_position(int port);
int32_t qei_get_velocity(int port);

#endif
