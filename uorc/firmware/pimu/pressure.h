#ifndef _PRESSURE_H
#define _PRESSURE_H

#include <stdint.h>
#include "nkern.h"
#include "ssi.h"
#include "lm3s8962.h"

void pressure_init();

// data is an 2 vector.
void pressure_get_data(double *data);

#endif
