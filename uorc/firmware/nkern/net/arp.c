#include <stdio.h>

#include <net.h>
#include <nkern.h>

#define ARP_CACHE_ENTRIES 16

// XXX make cache lookup more efficient
struct arp_entry
{
    uint32_t   ip_addr;
    eth_addr_t eth_addr;
    uint64_t   utime;
};

struct arp_entry arp_cache[ARP_CACHE_ENTRIES];

#define ARP_OP_REQUEST   1
#define ARP_OP_RESPONSE  2
#define RARP_OP_REQUEST  3
#define RARP_OP_RESPONSE 4

#define ARP_HW_ETHERNET 0x0001
#define ARP_PROT_IP     0x0800

#define ARP_HEADER_SIZE sizeof(struct arp_header)

// XXX We should do separate ARP tables for each device, since it will
// reduce the number of entries we need to search for a query.
void net_arp_init()
{
    for (int i = 0; i < ARP_CACHE_ENTRIES; i++)
        arp_cache[i].utime = 0;
}

int arp_packet_alloc(net_dev_t *netdev, net_packet_t **p, 
                     struct arp_header **arp_hdr);

// handle all ip-address-to-ethernet-address queries, including those
// special cases for local broadcast and multicast.
int net_arp_query(eth_dev_t *eth, uint32_t query_ip_addr, eth_addr_t *eth_addr)
{
    // special case?
    // XXX: Should we also use the broadcast MAC if they send to e.g. 192.168.1.255?
    if (query_ip_addr == 0xffffffff) {
        *eth_addr = 0xffffffffffffULL;
        return 0;
    }

    // XXX handle multicast addresses here.

    // in the cache?
    for (int i = 0; i < ARP_CACHE_ENTRIES; i++) {
        if (arp_cache[i].ip_addr == query_ip_addr) {
            arp_cache[i].utime = nkern_utime();
            *eth_addr = arp_cache[i].eth_addr;
            return 0;
        }
    }

    ip_config_t *ipcfg = ip_route(query_ip_addr);

    // not in the cache, send an ARP request
    // XXX throttle the rate at which we do this, add in-process queries to the arp table
    // with a special flag.

    net_packet_t *out_p;
    struct arp_header *out_arp_hdr;

    if (arp_packet_alloc(&eth->netdev, &out_p, &out_arp_hdr))
        goto exit;

    net_encode_u16(out_arp_hdr->hw_type, ARP_HW_ETHERNET);
    net_encode_u16(out_arp_hdr->prot_type, ARP_PROT_IP);
    out_arp_hdr->hw_size = 6;
    out_arp_hdr->prot_size = 4;
    net_encode_u16(out_arp_hdr->op, ARP_OP_REQUEST);
    net_encode_eth_addr(out_arp_hdr->sender_eth_addr, eth->eth_addr);
    net_encode_ip_addr(out_arp_hdr->sender_ip_addr, ipcfg->ip_addr);
    net_encode_eth_addr(out_arp_hdr->target_eth_addr, 0);
    net_encode_ip_addr(out_arp_hdr->target_ip_addr, query_ip_addr);

    eth->netdev.packet_send(&eth->netdev, out_p, 0xffffffff, 0x0806);

exit:
    return -1;
}

int arp_packet_alloc(net_dev_t *netdev, net_packet_t **p, 
                     struct arp_header **arp_hdr)
{
    int offset;
    *p =  netdev->packet_alloc(netdev, ARP_HEADER_SIZE, &offset);
    if (!*p)
      return -1;

    *arp_hdr = (struct arp_header*) &((*p)->data)[offset];

    return 0;
}

int net_arp_put(eth_dev_t *ethdev, uint32_t ip_addr, eth_addr_t eth_addr)
{
    (void) ethdev;

    uint64_t oldest_utime = UINT64_MAX;
    int oldest_idx = 0;

    for (int i = 0; i < ARP_CACHE_ENTRIES; i++) {
        if (arp_cache[i].utime < oldest_utime) {
            oldest_idx = i;
            oldest_utime = arp_cache[i].utime;
        }

        // update existing entry?
        if (arp_cache[i].ip_addr == ip_addr) {
            arp_cache[i].eth_addr = eth_addr;
            arp_cache[i].utime = nkern_utime();
            return 0;
        }
    }

    arp_cache[oldest_idx].ip_addr = ip_addr;
    arp_cache[oldest_idx].eth_addr = eth_addr;
    arp_cache[oldest_idx].utime = nkern_utime();

    return 0;
}

void net_arp_receive(net_packet_t *p, int offset)
{
    (void) offset;

    struct arp_header *arp_hdr = (struct arp_header*) &p->data[offset];

    net_dev_t *netdev = p->netdev;
    eth_dev_t *ethdev = (eth_dev_t*) netdev;

    // not ethernet hardware type?
    if (net_decode_u16(arp_hdr->hw_type) != 0x0001)
        goto exit;

    uint16_t op = net_decode_u16(arp_hdr->op);

    if (op == ARP_OP_REQUEST) {

        eth_addr_t eth_addr_src;
        ip4_addr_t ip_addr_src,  ip_addr_target;

        ip_addr_target = net_decode_ip_addr(arp_hdr->target_ip_addr);
        if (!ip_is_local_address(ip_addr_target))
            goto exit;

        // this is an arp request for us. Produce a response.
        ip_addr_src = net_decode_ip_addr(arp_hdr->sender_ip_addr);
        net_decode_eth_addr(arp_hdr->sender_eth_addr, &eth_addr_src);

        // put the requestor in the arp table to avoid our own arp query
        net_arp_put(ethdev, ip_addr_src, eth_addr_src);

        net_packet_t *out_p;
        struct arp_header *arp_hdr;

        if (arp_packet_alloc(netdev, &out_p, &arp_hdr))
            goto exit;
        
        net_encode_u16(arp_hdr->hw_type, ARP_HW_ETHERNET);   // ethernet
        net_encode_u16(arp_hdr->prot_type, ARP_PROT_IP); // IP
        arp_hdr->hw_size = 6;
        arp_hdr->prot_size = 4;
        net_encode_u16(arp_hdr->op, ARP_OP_RESPONSE);       // ARP reply

        net_encode_eth_addr(arp_hdr->sender_eth_addr, ethdev->eth_addr);
        net_encode_ip_addr(arp_hdr->sender_ip_addr, ip_addr_target);

        net_encode_eth_addr(arp_hdr->target_eth_addr, eth_addr_src);
        net_encode_ip_addr(arp_hdr->target_ip_addr, ip_addr_src);

        netdev->packet_send(netdev, out_p, ip_addr_src, 0x0806);
        goto exit;
    }

    if (op == ARP_OP_RESPONSE) {
        kprintf("ARP response\n");
        eth_addr_t eth_addr_src;
        ip4_addr_t ip_addr_src = net_decode_ip_addr(arp_hdr->sender_ip_addr);
        
        net_decode_eth_addr(arp_hdr->sender_eth_addr, &eth_addr_src);
        net_arp_put(ethdev, ip_addr_src, eth_addr_src);
        goto exit;
    }

    net_packet_dump(p);

exit:
    p->free(p);
}
