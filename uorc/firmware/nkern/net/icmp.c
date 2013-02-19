#include <string.h>

#include <nkern.h>
#include <net.h>

#define ICMP_PING_REQUEST  8
#define ICMP_PING_RESPONSE 0

int icmp_packet_alloc(icmp_alloc_t *icmpalloc, ip4_addr_t dest_addr, int *offset, int needed_size)
{
    if (ip_packet_alloc(&icmpalloc->ipalloc, dest_addr, offset, needed_size + ICMP_HEADER_SIZE))
        return -1;

    icmpalloc->icmp_hdr = (struct icmp_header*) &(icmpalloc->ipalloc.p->data)[*offset];

    *offset += ICMP_HEADER_SIZE;

    return 0;
}

int icmp_packet_send(icmp_alloc_t *icmpalloc, int size)
{
    ip_header_fill(&icmpalloc->ipalloc, size + ICMP_HEADER_SIZE, IP_PROTOCOL_ICMP);

    // must clear checksum field before computing checksum, since the
    // checksummed region includes the checksum field.
    net_encode_u16(icmpalloc->icmp_hdr->checksum, 0);
    net_encode_u16(icmpalloc->icmp_hdr->checksum, ip_checksum(icmpalloc->icmp_hdr,
                                                              size + ICMP_HEADER_SIZE,
                                                              0) ^ 0xffff);
    
    return ip_packet_send(&icmpalloc->ipalloc);
}

/** offset is the beginning of the ping payload **/
static void icmp_ping_receive(net_packet_t *p, 
                              struct ip_header *ip_hdr, struct icmp_header *icmp_hdr, int offset)
{
    (void) icmp_hdr;

    int ip_size = net_decode_u16(ip_hdr->length);
    int icmp_size = ip_size - IP_HEADER_SIZE;
    int ping_size = icmp_size - ICMP_HEADER_SIZE;

    int out_offset;
    icmp_alloc_t icmpalloc;
    ip4_addr_t dest_ip_addr = net_decode_ip_addr(ip_hdr->src_ip);
    if (icmp_packet_alloc(&icmpalloc, dest_ip_addr, &out_offset, ping_size)) 
        goto exit;

    struct icmp_header *out_icmp_hdr = icmpalloc.icmp_hdr;
    net_packet_t *out_p = icmpalloc.ipalloc.p;

    out_icmp_hdr->type = ICMP_PING_RESPONSE;
    out_icmp_hdr->code = 0;

    memcpy(&out_p->data[out_offset], &p->data[offset], ping_size);

    if (icmp_packet_send(&icmpalloc, ping_size)) {
        kprintf("icmp send failed, size %i\n", ping_size);
    }

exit:
    p->free(p);
}

void net_icmp_receive(net_packet_t *p, struct ip_header *ip_hdr, int offset)
{
    struct icmp_header *icmp_hdr = (struct icmp_header*) &p->data[offset];
    int size = net_decode_u16(ip_hdr->length) - IP_HEADER_SIZE;

    if (ip_checksum(&p->data[offset], size, 0) != 0xffff)
        goto exit;

    if (icmp_hdr->type == ICMP_PING_REQUEST) {
        icmp_ping_receive(p, ip_hdr, icmp_hdr, offset + ICMP_HEADER_SIZE);
        return;
    }

    //    kprintf("\tICMP %02x\n", icmp_hdr->type);
    //    net_packet_dump(p);

exit:
    p->free(p);
}
