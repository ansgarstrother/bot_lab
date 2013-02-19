#ifndef _PARAM_H
#define _PARAM_H

#include "nkern.h"
#include "net.h"

#define FLASH_PARAM_SIGNATURE 0xed881905
#define FLASH_PARAM_VERSION 0x0005
#define FLASH_PARAM_LENGTH (sizeof(struct flash_params))

struct __attribute__ ((packed)) flash_params
{
    ///////////////////////////////////////
    uint32_t start_signature;     // used to detect invalid data

    uint32_t version; // used to detect incompatible parameter blocks
    uint32_t length;  // (ditto!)

    ///////////////////////////////////////
    // Our parameters here.

    double gyro_gain[8];   // in fixed point
    double gyro_offset[8]; // in adc ticks

    ///////////////////////////////////////

    ///////////////////////////////////////
    uint32_t end_signature;    // used to detect invalid data
};

static struct flash_params *flash_params = ((struct flash_params*) 0x3fc00);

void param_init();

#endif
