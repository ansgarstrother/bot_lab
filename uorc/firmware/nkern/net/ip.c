#include <stdlib.h>
#include <ctype.h>

#include <nkern.h>
#include <net.h>

#define IP_HEADER_SIZE  20
#define ICMP_PROTOCOL   0x01
#define TCP_PROTOCOL    0x06
#define UDP_PROTOCOL    0x11

// this is the routing table; it should be maintained in order of most
// specific routing information to least specific (default gateway
// address)
ip_config_t *ip_config_head;

uint64_t ip_rx_count, ip_tx_count;

int ip_packet_alloc(ip_alloc_t *ipalloc, ip4_addr_t dest_addr, int *offset, int needed_size)
{
    // Find the IP interface that will send this packet
    // (this will tell us which network device too.)
    ipalloc->ipcfg = ip_route(dest_addr);

    // No route to host?
    if (!ipalloc->ipcfg)
        return -1;

    net_dev_t *netdev = ipalloc->ipcfg->netdev;

    // Ask the device layer for a buffer.
    ipalloc->p =  netdev->packet_alloc(netdev, needed_size + IP_HEADER_SIZE, offset);
    if (!ipalloc->p)
        return -2;

    ipalloc->ip_hdr = (struct ip_header*) &(ipalloc->p->data)[*offset];
    ipalloc->dest_ip_addr = dest_addr;

    *offset += IP_HEADER_SIZE;

    return 0;
}

static uint16_t ip_counter;

int ip_header_fill(ip_alloc_t *ipalloc, int size, int protocol)
{
    struct ip_header *ip_hdr = ipalloc->ip_hdr;

    ip_hdr->ver_hdrlen                  = 0x45;
    ip_hdr->tos                         = 0x00;
    net_encode_u16(ip_hdr->length,        size + IP_HEADER_SIZE );
    net_encode_u16(ip_hdr->id,            ip_counter++ );
    net_encode_u16(ip_hdr->frag_offset,   0 );
    ip_hdr->ttl                         = 64;
    ip_hdr->protocol                    = protocol;
    net_encode_u16(ip_hdr->checksum,      0 );
    net_encode_ip_addr(ip_hdr->src_ip,    ipalloc->ipcfg->ip_addr );
    net_encode_ip_addr(ip_hdr->dst_ip,    ipalloc->dest_ip_addr );

    net_encode_u16(ip_hdr->checksum,      ip_checksum(ip_hdr, IP_HEADER_SIZE, 0) ^ 0xffff );

    ipalloc->p->size = size + IP_HEADER_SIZE;
    return 0;
}

int ip_packet_send(ip_alloc_t *ipalloc)
{
    ip_config_t *ipcfg = ipalloc->ipcfg;
    net_dev_t *netdev = ipcfg->netdev;

    ip_tx_count++;

    // case: local subnet traffic (no gateway needed) or broadcast.
    // XXX: this is not an elegant way of handling broadcast
    // traffic. (Other broadcast addresses like multicast?)
    if (ipcfg->gw_ip_addr == 0 || ipalloc->dest_ip_addr == 0xffffffff)
        return netdev->packet_send(netdev, ipalloc->p, ipalloc->dest_ip_addr, 0x0800);

    // case: Need to route packet via gateway
    return netdev->packet_send(netdev, ipalloc->p, ipcfg->gw_ip_addr, 0x0800);
}

/** offset is the index (always in the first chunk) to teh beginning of the ip_header **/
void net_ip_receive(net_packet_t *p, int offset)
{
    struct ip_header * ip_hdr = (struct ip_header*) &p->data[offset];

    // unknown IP version or incompatible IP header length?
    if (ip_hdr->ver_hdrlen != 0x45)
        goto exit;

    ip_rx_count++;

    //    kprintf("IP %02x size %04x\n", ip_hdr->protocol, net_decode_u16(ip_hdr->length));

    // short packet?
    if (net_decode_u16(ip_hdr->length) + offset > p->size)
        goto exit;

    // bad checksum?
    if (ip_checksum(ip_hdr, IP_HEADER_SIZE, 0) != 0xffff)
        goto exit;


    if (ip_hdr->protocol == UDP_PROTOCOL) {
        net_udp_receive(p, ip_hdr, offset + IP_HEADER_SIZE);
        return;
    }

    if (ip_hdr->protocol == TCP_PROTOCOL) {
        net_tcp_receive(p, ip_hdr, offset + IP_HEADER_SIZE);
        return;
    }

    if (ip_hdr->protocol == ICMP_PROTOCOL) {
        net_icmp_receive(p, ip_hdr, offset + IP_HEADER_SIZE);
        return;
    }


exit:
    // unhandled packet
    //    net_packet_dump(p);

    p->free(p);
}

int ip_config_add(net_dev_t *netdev, ip4_addr_t ip_addr, ip4_addr_t ip_mask, ip4_addr_t gw_ip_addr)
{
    ip_config_t *cfg = calloc(1, sizeof(ip_config_t));
    cfg->ip_addr = ip_addr;
    cfg->ip_mask = ip_mask;
    cfg->gw_ip_addr = gw_ip_addr;
    cfg->netdev = netdev;

    // add to the list of routes in decreasing order of specificity,
    // e.g., default gateway should be last.
    ip_config_t **ptr = &ip_config_head;

    while (1) {
        // is this entry less specific than the one we're looking at?
        // if so, skip it.
        if (*ptr && cfg->ip_mask < (*ptr)->ip_mask) {
            ptr = &((*ptr)->next);
            continue;
        }

        // reached end of list (*ptr == NULL) or this is the spot to insert
        cfg->next = *ptr;
        *ptr = cfg;
        break;
    }

    return 0;
}

ip_config_t *ip_route(ip4_addr_t addr)
{
    for (ip_config_t *ipcfg = ip_config_head; ipcfg != NULL; ipcfg = ipcfg->next) {
        if ((addr & ipcfg->ip_mask) == (ipcfg->ip_addr & ipcfg->ip_mask))
            return ipcfg;
    }

    return NULL;
}

int ip_is_local_address(ip4_addr_t addr)
{
    for (ip_config_t *ipcfg = ip_config_head; ipcfg != NULL; ipcfg = ipcfg->next) {
        if (ipcfg->ip_addr == addr)
            return 1;
    }

    return 0;
}

ip4_addr_t ip_primary_address()
{
    if (ip_config_head)
        return ip_config_head->ip_addr;
    return 0x7f000001;
}

static ip4_addr_t inet_addr_numeric(const char *s)
{
    ip4_addr_t addr = 0;

    for (int i = 0; i < 4; i++) {
        int v = 0;
        while (*s && *s!='.') {
            v=v*10 + (*s-'0');
            s++;
        }
        if (*s=='.')
            s++;
        addr = (addr<<8) | v;
    }

    return addr;
}

ip4_addr_t inet_addr(const char *s)
{
    if (isdigit(s[0]))
        return inet_addr_numeric(s);

    return 0;
}

//
static int hex2string_decimal(int v, char *s)
{
    int pos = 0;
    int v100 = (v/100)%10;
    if (v100 > 0)
        s[pos++] = '0'+v100;

    int v10 = (v/10)%10;
    if (v100 > 0 || v10 > 0)
        s[pos++] = '0'+v10;

    int v1 = v%10;
    s[pos++] = '0'+v1;

    s[pos] = 0;

    return pos;
}

void inet_addr_to_dotted(ip4_addr_t addr, char *s) // s should have at least 16 characters space
{
    int pos = 0;

    pos += hex2string_decimal(addr>>24, &s[pos]);
    s[pos++] = '.';
    pos += hex2string_decimal(((addr>>16)&0xff), &s[pos]);
    s[pos++] = '.';
    pos += hex2string_decimal(((addr>>8)&0xff), &s[pos]);
    s[pos++] = '.';
    pos += hex2string_decimal(addr&0xff, &s[pos]);
    s[pos] = 0;
}
