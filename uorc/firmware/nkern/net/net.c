#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <nkern.h>
#include <net.h>

#define ETHERNET_HEADER_SIZE 14

uint64_t net_rx_count;

void net_arp_init();
void net_udp_init();
void net_tcp_init();

void net_packet_dump(net_packet_t *packet)
{
    kprintf("net_packet_t, frame size %6d\n", packet->size);
    
    int pos = 0;

    for (int i = 0; i < packet->size; i++, pos++) {
        
        if ((pos & 15) == 0)
            kprintf("%04x : ", pos);
        
        kprintf("%02x ", ((char*) packet->data)[i]);
        if ((pos & 15) == 15)
            kprintf("\n");
    }

    kprintf("\n");
}

void net_ethernet_receive(net_packet_t *p, int offset)
{
    struct ethernet_header *ehdr = (struct ethernet_header*) &p->data[offset];
    
    uint32_t type = net_decode_u16(ehdr->type);
    
    net_rx_count++;

    if (type == 0x0800) {
        net_ip_receive(p, offset + ETHERNET_HEADER_SIZE);
        return;
    }
    
    if (type == 0x0806) {
        net_arp_receive(p, offset + ETHERNET_HEADER_SIZE);
        return;
    }

    //    kprintf("rx unknown type %04x\n", (int) type);
    //    net_packet_dump(p);

    p->free(p);
}

void net_init()
{
    net_arp_init();
    net_udp_init();
    net_tcp_init();
}

ip4_addr_t net_decode_ip_addr(uint8_t *p)
{
    ip4_addr_t addr = 0;

    for (int i = 0; i < 4; i++)
        addr = (addr << 8) | p[i];

    return addr;
}

void net_decode_eth_addr(uint8_t *p, eth_addr_t *_addr)
{
    eth_addr_t addr = 0;

    for (int i = 0; i < 6; i++)
        addr = (addr << 8) | p[i];

    *_addr = addr;
}

void net_encode_u8(uint8_t *p, uint8_t v)
{
    *p = v;
}

uint8_t net_decode_u8(uint8_t *p)
{
    return *p;
}

void net_encode_u64(uint8_t *p, uint64_t v)
{
    p[7] = v & 0xff;
    v >>= 8;
    p[6] = v & 0xff;
    v >>= 8;
    p[5] = v & 0xff;
    v >>= 8;
    p[4] = v & 0xff;
    v >>= 8;    
    p[3] = v & 0xff;
    v >>= 8;
    p[2] = v & 0xff;
    v >>= 8;
    p[1] = v & 0xff;
    v >>= 8;
    p[0] = v & 0xff;
}

void net_encode_ip_addr(uint8_t *p, uint32_t ip_addr)
{
    p[3] = ip_addr & 0xff;
    ip_addr >>= 8;
    p[2] = ip_addr & 0xff;
    ip_addr >>= 8;
    p[1] = ip_addr & 0xff;
    ip_addr >>= 8;
    p[0] = ip_addr & 0xff;
}

void net_encode_eth_addr(uint8_t *p, eth_addr_t eth_addr)
{
    p[5] = eth_addr & 0xff;
    eth_addr >>= 8;
    p[4] = eth_addr & 0xff;
    eth_addr >>= 8;
    p[3] = eth_addr & 0xff;
    eth_addr >>= 8;
    p[2] = eth_addr & 0xff;
    eth_addr >>= 8;
    p[1] = eth_addr & 0xff;
    eth_addr >>= 8;
    p[0] = eth_addr & 0xff;
}

void net_encode_u16(uint8_t *p, uint16_t v)
{
    p[0] = v >> 8;
    p[1] = v & 0xff;
}

uint16_t net_decode_u16(uint8_t *p)
{
    return (p[0]<<8) | p[1];
}

uint64_t net_decode_u64(uint8_t *p)
{
    uint64_t hi32 = net_decode_u32(p);
    uint64_t lo32 = net_decode_u32(p);

    return hi32<<32 | lo32;
}
