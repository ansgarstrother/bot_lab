#include <nkern.h>
#include <net.h>

#include "flash.h"
#include "param.h"

extern void serial0_putc(char c);

// A randomly-derived identifier that is fixed for each boot. This is
// used to help identify multiple uOrcs on the same network that might
// have identical MAC or IP addresses. Hopefully, they'll have
// different boot nonces.
static uint32_t device_boot_nonce;

// it will take a certain amount of entropy to make the boot nonce
// useful.
static uint32_t device_boot_nonce_unready = 1024;

void discover_task()
{
    udp_listener_t ul;
    udp_rxdata_t *rxdata;

    udp_listen(&ul, 2377);

    while(1) {
        udp_read(&ul, &rxdata);

        uint8_t *in = rxdata->udp_hdr->payload;
        int      insz = net_decode_u16(rxdata->udp_hdr->length) - UDP_HEADER_SIZE;

        ////////////////////////////////////////////////////////
        // generate a boot nonce if we haven't already
        if (device_boot_nonce==0 && nkern_random_bits_available()>=32) {
            device_boot_nonce = nkern_random_take(32);
        }

        // not enough entropy to generate a boot nonce.
        if (device_boot_nonce==0)
            goto cleanup;

        ////////////////////////////////////////////////////////
        // handle this discover packet.
        uint32_t magic = (in[0]<<24) + (in[1]<<16) + (in[2]<<8) + in[3];
        if (magic != 0xedaab739)
            goto cleanup;

        // update flash.
        uint32_t cmd = (in[4]<<24) + (in[5]<<16) + (in[6]<<8) + in[7];

        switch(cmd)
        {
        case 0x00000000: {
            // they're just querying us.
            break;
        }

        case 0x0000ff01: {
            // they want to write to someone's flash.
            uint32_t req_nonce = (in[8]<<24) + (in[9]<<16) + (in[10]<<8) + in[11];

            // packet isn't for us!
            if (req_nonce != device_boot_nonce)
                goto cleanup;

            // write the flash.
            uint32_t writelen = (in[12]<<24) + (in[13]<<16) + (in[14]<<8) + in[15];
            flash_erase_and_write((uint32_t) flash_params, &in[16], writelen);
            break;
        }

        default: {
            // unknown command.
            kprintf("Discovery: unknown command: %08x\n", (unsigned int) cmd);
            break;
        }
        }

        ////////////////////////////////////////////////////////
        // Send a response containing our current configuration.
        udp_alloc_t udpalloc;
        int         offset;

        if (udp_packet_alloc(&udpalloc, net_decode_ip_addr(rxdata->ip_hdr->src_ip), &offset, 1024))
            goto cleanup;

        int len = 0;
        uint8_t *out = &udpalloc.ipalloc.p->data[offset];

        net_encode_u32(&out[len], 0x754f5243); // 'uORC'
        len += 4;

        net_encode_u32(&out[len], device_boot_nonce);
        len += 4;

        // copy the flash params
        for (int i = 0; i < 256; i++) {
            out[len++] = ((uint8_t*) flash_params)[i];
        }

        udp_packet_send(&udpalloc, len, 2379, net_decode_u16(rxdata->udp_hdr->src_port));

    cleanup:
        udp_read_release(&ul, rxdata);
    }
}

void discover_init()
{
    // requires at least 624 bytes!
    nkern_task_create("discover_task",
                      discover_task, NULL,
                      NKERN_PRIORITY_NORMAL, 1024);

}
