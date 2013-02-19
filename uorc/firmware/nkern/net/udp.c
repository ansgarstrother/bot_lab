#include <string.h>

#include <nkern.h>
#include <net.h>

udp_listener_t *listeners_head;

static int tx_packet_count; // how many transmitted packets?
static int rx_packet_count; // how many packets total?
static int rx_orphan_packet_count; // how many received packets that had no listener?
static int rx_checksum_err; // how many checksum errors?
static int rx_no_space; // how many packets were discarded due to too few rx buffer records?
static int tx_no_space; // how many packet allocs failed?

void net_udp_init()
{
}

// XXX To-do: rewrite me as a queue
void net_udp_receive(net_packet_t *p, struct ip_header *ip_hdr, int offset)
{
    irqstate_t flags;
    struct udp_header *udp_hdr = (struct udp_header*) &p->data[offset];
    int udp_size = net_decode_u16(ip_hdr->length) - IP_HEADER_SIZE;

    // checksum test
    if (1) {
        // compute checksum on UDP header and body
        uint16_t chk = ip_checksum(udp_hdr, udp_size, 0);

        // add in checksum on pseudo-header
        chk = ip_checksum(&ip_hdr->src_ip, 8, chk);
        uint8_t ps[4]; ps[0] = 0; ps[1] = ip_hdr->protocol; ps[2] = udp_size >> 8; ps[3] = udp_size & 0xff;
        chk = ip_checksum(ps, 4, chk);

        if (chk != 0xffff) {
            rx_checksum_err++;
            goto exit;
        }
    }

    // packet received
    rx_packet_count++;

    // is there a listener?
    uint32_t port = net_decode_u16(udp_hdr->dst_port);

    interrupts_disable(&flags);
    udp_listener_t *listener = listeners_head;
    while (listener != NULL) {
        if (listener->port == port)
            break;
        listener = listener->next;
    }
    interrupts_restore(&flags);

    // no listener.
    if (listener == NULL) {
        // XXX send ICMP unreachable
        rx_orphan_packet_count++;
        goto exit;
    }

    // allocate a rx buffer record
    udp_rxdata_t *rxdata = NULL;
    interrupts_disable(&flags); // maybe don't have to disable interrupts for this.

    for (int i = 0; i < UDP_RXDATA_RECORDS; i++) {
        if (listener->rxdata[i].state == 0) {
            rxdata = &listener->rxdata[i];
            rxdata->state = 1;
            break;
        }
    }

    if (rxdata == NULL) {
        // no buffers available.
        rx_no_space++;
        interrupts_restore(&flags);
        goto exit;
    }

    interrupts_restore(&flags);

    rxdata->p = p;
    rxdata->ip_hdr = ip_hdr;
    rxdata->udp_hdr = udp_hdr;

    interrupts_disable(&flags);

    listener->rx_packet_count++;
    interrupts_restore(&flags);

    nkern_semaphore_up(&listener->rx_sem);

    return;

 exit:
    p->free(p);
}

int udp_listen(struct udp_listener *listener, uint16_t port)
{
    listener->port = port;
    listener->rx_packet_count = 0;

    nkern_semaphore_init(&listener->rx_sem, "udp_rx", 0);

    irqstate_t flags;
    interrupts_disable(&flags);

    listener->next = listeners_head;
    listeners_head = listener;

    for (int i = 0; i < UDP_RXDATA_RECORDS; i++)
        listener->rxdata[i].state = 0;

    interrupts_restore(&flags);

    return 0;
}

int udp_read(struct udp_listener *listener, udp_rxdata_t **rxdata)
{
    nkern_semaphore_down(&listener->rx_sem);

    *rxdata = NULL;

    irqstate_t flags;
    interrupts_disable(&flags);

    for (int i = 0; i < UDP_RXDATA_RECORDS; i++) {
        if (listener->rxdata[i].state == 1) {
            *rxdata = &listener->rxdata[i];
            listener->rxdata[i].state = 2;
            break;
        }
    }

    if (*rxdata == NULL)
        nkern_fatal(0x588a);

    interrupts_restore(&flags);
    return 0;
}

int udp_read_release(struct udp_listener *listener, udp_rxdata_t *rxdata)
{
    (void) listener;
    irqstate_t flags;
    interrupts_disable(&flags);

    rxdata->p->free(rxdata->p);

    if (rxdata->state != 2)
        nkern_fatal(0x588b);

    rxdata->state = 0;

    interrupts_restore(&flags);
    return 0;
}

int udp_packet_alloc(udp_alloc_t *udpalloc, ip4_addr_t dest_addr, int *offset, int needed_size)
{
    if (ip_packet_alloc(&udpalloc->ipalloc, dest_addr, offset, needed_size + UDP_HEADER_SIZE)) {
        tx_no_space++;
        return -1;
    }

    udpalloc->udp_hdr = (struct udp_header*) &(udpalloc->ipalloc.p->data)[*offset];

    *offset += UDP_HEADER_SIZE;

    return 0;
}

uint8_t *udp_packet_get_buffer(udp_alloc_t *udpalloc)
{
    return udpalloc->ipalloc.p->data;
}

int udp_packet_send(udp_alloc_t *udpalloc, int size, int src_port, int dst_port)
{
    int udp_size = size + UDP_HEADER_SIZE;

    ip_header_fill(&udpalloc->ipalloc, size + UDP_HEADER_SIZE, IP_PROTOCOL_UDP);

    struct udp_header *udp_hdr = udpalloc->udp_hdr;
    net_packet_t *p = udpalloc->ipalloc.p;

    net_encode_u16(udp_hdr->length, udp_size);
    net_encode_u16(udp_hdr->checksum, 0);
    net_encode_u16(udp_hdr->src_port, src_port);
    net_encode_u16(udp_hdr->dst_port, dst_port);
    uint16_t chk = ip_checksum(udpalloc->udp_hdr, size + UDP_HEADER_SIZE, 0);

    // add in checksum on pseudo-header
    struct ip_header *ip_hdr = udpalloc->ipalloc.ip_hdr;

    chk = ip_checksum(&ip_hdr->src_ip, 8, chk);
    uint8_t ps[4]; ps[0] = 0; ps[1] = ip_hdr->protocol; ps[2] = udp_size >> 8; ps[3] = udp_size & 0xff;
    chk = ip_checksum(ps, 4, chk);
    net_encode_u16(udp_hdr->checksum, chk ^ 0xffff);

    tx_packet_count++;

    return ip_packet_send(&udpalloc->ipalloc);
}

void udp_print_stats(iop_t *iop)
{
    pprintf(iop, "UDP: RX packets %8d, TX packets %8d, checksum err %4d, orphans %4d, rx no space %4d, tx no space %4d\n",
            rx_packet_count, tx_packet_count, rx_checksum_err, rx_orphan_packet_count, rx_no_space, tx_no_space);

    udp_listener_t *listener = listeners_head;
    while (listener != NULL) {
        pprintf(iop, "  Listening on port %5d, RX count %8d\n", (int) listener->port, listener->rx_packet_count);
        listener = listener->next;
    }
}
