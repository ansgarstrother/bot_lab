#ifndef _ACCEL_H
#define _ACCEL_H

#include <stdint.h>
#include "nkern.h"
#include "lm3s8962.h"
#include "ssi.h"

void accel_init();

// data is an 8 vector.
void accel_get_data(uint32_t *data);

#endif
