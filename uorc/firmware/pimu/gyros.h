#ifndef _GYROS_H
#define _GYROS_H

#include <stdint.h>
#include "nkern.h"
#include "ssi.h"
#include "lm3s8962.h"

void gyros_init();

// data is an 8 vector.
void gyros_get_data(uint32_t *data);

#endif
