#ifndef _MAGNETOMETER_H
#define _MAGNETOMETER_H

#include <stdint.h>
#include "nkern.h"
#include "i2c.h"
#include "lm3s8962.h"

void magnetometer_init();

// data is an 3 vector.
void magnetometer_get_data(uint32_t *data);

#endif
