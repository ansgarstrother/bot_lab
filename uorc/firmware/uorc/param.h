#ifndef _PARAM_H
#define _PARAM_H

#include "nkern.h"
#include "net.h"

#define FLASH_PARAM_SIGNATURE 0xed719abf
#define FLASH_PARAM_VERSION 0x0002
#define FLASH_PARAM_LENGTH (sizeof(struct flash_params))

struct __attribute__ ((packed)) flash_params
{
    ///////////////////////////////////////
    uint32_t start_signature;     // used to detect invalid data

    uint32_t version; // used to detect incompatible parameter blocks
    uint32_t length;  // (ditto!)

    ///////////////////////////////////////
    // Our parameters here.

    ip4_addr_t ipaddr;
    ip4_addr_t ipmask;
    ip4_addr_t ipgw;

    int64_t  macaddr;          // lower 32 bits of mac address

    uint8_t  dhcpd_enable;     // should we run a dhcpd server?

    ///////////////////////////////////////
    uint32_t end_signature;    // used to detect invalid data
};

static struct flash_params *flash_params = ((struct flash_params*) 0x3fc00);

void param_init();

#endif
