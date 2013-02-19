#ifndef _ETHERNET_H
#define _ETHERNET_H

#include <net.h>
#include <stdint.h>

net_dev_t *ethernet_init(uint32_t pclk, eth_addr_t mac);

#endif
