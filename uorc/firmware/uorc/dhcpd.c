#include <stdint.h>
#include <string.h>
#include <nkern.h>
#include <net.h>

/** We assign IP addresses on the subnet that we're already configured
 * for (see main.c) within the range below.  (range is INCLUSIVE).
 **/
#define RANGE_MIN 80
#define RANGE_MAX 89

struct dhcpd_reservation
{
    uint32_t   lastxid; // corresponding transaction id
    eth_addr_t ethaddr; // mac address of remote client
    uint8_t    valid;   // is this record valid?
    uint8_t    ip8;      // What IP are we assigning to this client? (last octet)
};

#define MAX_RESERVATIONS 4

static struct dhcpd_reservation reservations[MAX_RESERVATIONS];

void dhcpd_task(void *args)
{
    udp_listener_t ul;
    udp_rxdata_t *rxdata;

    udp_listen(&ul, 67);

    while(1) {
        udp_read(&ul, &rxdata);

        kprintf("DHCPD: packet received\n");

        uint8_t *in = rxdata->udp_hdr->payload;
        int      insz = net_decode_u16(rxdata->udp_hdr->length) - UDP_HEADER_SIZE;

        int op     = in[0];
        int htype  = in[1];  // should be 0x01 (ethernet)
        int hlen   = in[2];  // shoud be 0x06 (ethernet mac addr length = 6)
        int hop    = in[3];

        uint32_t xid = net_decode_u32(&in[4]);

        int seconds = net_decode_u16(&in[8]);
        uint32_t flags = net_decode_u16(&in[10]);

        ip4_addr_t ciaddr = net_decode_ip_addr(&in[12]); // client
        ip4_addr_t yiaddr = net_decode_ip_addr(&in[16]); // next server
        ip4_addr_t siaddr = net_decode_ip_addr(&in[20]); // relay agent
        ip4_addr_t giaddr = net_decode_ip_addr(&in[24]);
        eth_addr_t ethaddr;
        net_decode_eth_addr(&in[28], &ethaddr); // mac address

        uint32_t magic = net_decode_u32(&in[236]); // 0x63825363

        int dhcpop = -1;

        // parse options
        if (1) {
            int pos = 240;
            while (in[pos] != 0xff && pos < insz) {

                if (in[pos] == 0x00) {
                    // padding byte, no length field. (Special case!)
                    pos++;
                    continue;
                }

                // in[pos+0] = type
                // in[pos+1] = length of data field
                // in[pos+2] = <beginning of length data bytes>

                if (in[pos] == 0x35) {
                    // DHCP message type
                    dhcpop = in[pos+2]; // get the REAL operation.
                }

                if (in[pos] == 0x0c) {
                    // host name
                }

                if (in[pos] == 0x37) {
                    // parameter request list
                }

                pos += in[pos+1] + 2;
            }
        }

        /////////////////////////////////////////////////////
        // Find the current reservation for this client (if one exists)
        struct dhcpd_reservation *reservation = NULL;
        for (int i = 0; i < MAX_RESERVATIONS; i++) {
            if (reservations[i].valid && reservations[i].ethaddr == ethaddr)
                reservation = &reservations[i];
        }

        // No reservation record? Create a new one.
        if (reservation == NULL) {
            // find an open reservation record
            for (int i = 0; i < MAX_RESERVATIONS; i++) {
                if (!reservations[i].valid) {

                    reservations[i].ip8 = 0; // mark as invalid.

                    // find an open ip address
                    for (int ip8 = RANGE_MIN; ip8 <= RANGE_MAX; ip8++) {

                        // do any other valid reservations already use this ip8?
                        int bad = 0;
                        for (int j = 0; j < MAX_RESERVATIONS; j++) {
                            if (reservations[j].valid && (reservations[j].ip8&0xff) == ip8)
                                bad = 1;
                        }

                        // If not, we've found a good ip!
                        if (!bad) {
                            reservations[i].ip8 = ip8;
                            break;
                        }
                    }

                    if (reservations[i].ip8 == 0) {
                        // no IP addresses available.  (this shouldn't
                        // happen in sane configurations). Just delete
                        // all reservations and let the client try
                        // again.
                        for (int i = 0; i < MAX_RESERVATIONS; i++)
                            reservations[i].valid = 0;
                        return;
                    }

                    // we found a record! Fill in the rest of the
                    // fields (ip8 filled in above).
                    reservation = &reservations[i];
                    reservation->valid = 1;
                    reservation->lastxid = xid;
                    reservation->ethaddr = ethaddr;
                    break;
                }
            }
        }

        /////////////////////////////////////////////////////
        // Now, actually do something.  Our responses, the DHCP_OFFER
        // and DHCP_ACK are almost identical, thus we handle them
        // together... with just a few special cases inline.
        udp_alloc_t udpalloc;
        int offset;

        if (!udp_packet_alloc(&udpalloc, inet_addr("255.255.255.255"), &offset, 512)) {

            uint8_t *out = &udpalloc.ipalloc.p->data[offset];
            memset(out, 0, 512);

            // we send an offer.
            out[0] = 0x02; // op
            out[1] = 0x01; // eth hw type
            out[2] = 0x06; // ethernet mac addr len
            out[3] = 0x00; // hops
            net_encode_u32(&out[4], xid);
            net_encode_u16(&out[8], 0);  // seconds
            net_encode_u16(&out[10], 0); // flags
            net_encode_u32(&out[12], 0); // ciaddr

            ip4_addr_t client_addr = (ip_primary_address() & 0xffffff00) + reservation->ip8;
            net_encode_u32(&out[16], client_addr); // yiaddr  IP address that we're assinging

            net_encode_u32(&out[20], 0); // siaddr
            net_encode_u32(&out[24], 0); // giaddr
            net_encode_eth_addr(&out[28], ethaddr);
            net_encode_u32(&out[236], 0x63825363); // magic

            int pos = 240;
            out[pos+0] = 53; // DHCP op
            out[pos+1] = 1;  // len
            if (dhcpop == 1)
                out[pos+2] = 2;  // type = DHCP OFFER
            else if (dhcpop == 3)
                out[pos+2] = 5; // type = DHCP ACK
            pos += 3;

            out[pos+0] = 1; // subnet mask
            out[pos+1] = 4; // len
            net_encode_ip_addr(&out[pos+2], 0xffffff00);
            pos += 6;

/*
  // don't specify a router: this causes the dhcp client to create a
  // default route via the uorc to the outside world.

            out[pos+0] = 3; // router
            out[pos+1] = 4; // len
            net_encode_ip_addr(&out[pos+2], udpalloc.ipalloc.ipcfg->gw_ip_addr);
            pos += 6;
*/

            out[pos+0] = 54; // dhcp server
            out[pos+1] = 4;  // len
            net_encode_ip_addr(&out[pos+2], udpalloc.ipalloc.ipcfg->ip_addr); // server's ip address
            pos += 6;

            out[pos+0] = 51; // lease time
            out[pos+1] = 4;  // len
            net_encode_u32(&out[pos+2], 0xffffffff); // infinite duration (seconds)
            pos+=6;

            out[pos+0] = 0xff; // end
            pos+=1;

            udp_packet_send(&udpalloc, pos, 67, 68);

            if (dhcpop == 1)
                kprintf("DHCPD: Sending offer %d.%d.%d.%d\n", out[16], out[17], out[18], out[19]);
            else
                kprintf("DHCPD: Sending ack\n");
        }

        udp_read_release(&ul, rxdata);
    }
}

void dhcpd_init()
{
        nkern_task_create("dhcpd_task",
                          dhcpd_task, NULL,
                          NKERN_PRIORITY_IDLE, 1024);
}
