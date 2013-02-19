#ifndef _I2C_H
#define _I2C_H

#include <nkern.h>
#include <nkern_sync.h>

void i2c_init();
void i2c_lock();
void i2c_unlock();
void i2c_config(uint32_t I2C_freq);
int i2c_transaction(uint32_t addr, uint8_t *writebuf, uint32_t writelen, uint8_t *readbuf, uint32_t readlen);

#endif
