#ifndef _PARAM_H
#define _PARAM_H

#include <stdint.h>

/** Disable interrupts! **/
int flash_erase(int cpu_khz, int flash_sector);
int flash_write(int cpu_khz, int flash_sector, void *flash_addr, void *ram_data, uint32_t datalen);

#endif
