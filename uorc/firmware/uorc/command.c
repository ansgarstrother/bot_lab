#include <nkern.h>
#include <net.h>

#include "mcp23s17.h"
#include "motor.h"
#include "fast_digio.h"
#include "i2c.h"
#include "flash.h"
#include "param.h"
#include "intadc.h"
#include "extadc.h"
#include "digio.h"
#include "get_state.h"
#include "serial.h"
#include "shiftbrite.h"
#include "watchdog.h"

extern int response_status(uint8_t *buf);

extern const char FIRMWARE_REVISION[];

#define ORC_BASE_PORT 2378

void isp_eth();

// Different commands are received on different ports. This is so that
// we can have multiple threads servicing commands. Specifically, some
// commands are very slow (like those which access AX12
// servos--especially if the AX12 experiences an error or isn't
// actually plugged in), and we don't want to block requests for other
// commands in the meantime.

// the high 8 bits of the command_id correspond to which port the
// command is received on. For example 0x1000 is received on port 0,
// while 0x01007000 is received on port 1.

// handles most fast commands (ORC_BASE_PORT + 0)
void cmd0_task(void *arg)
{
    udp_listener_t ul;
    udp_rxdata_t *rxdata;

    udp_listen(&ul, ORC_BASE_PORT + 0);

    while(1) {
        udp_read(&ul, &rxdata);

        uint8_t *in = rxdata->udp_hdr->payload;
        int      insz = net_decode_u16(rxdata->udp_hdr->length) - UDP_HEADER_SIZE;

        if (insz < 20)
            goto cleanup;

        udp_alloc_t udpalloc;
        int         offset;

        uint32_t header = net_decode_u32(&in[0]);
        uint32_t transaction_id = net_decode_u32(&in[4]);
        uint64_t sender_utime = net_decode_u64(&in[8]);
        uint32_t command_id = net_decode_u32(&in[16]);

        if (header!=0x0ced0002)
            goto cleanup;

        if (udp_packet_alloc(&udpalloc, net_decode_ip_addr(rxdata->ip_hdr->src_ip), &offset, 1024))
            goto cleanup;

        //////////////////////////////////
        int len = 0;
        uint8_t *out = &udpalloc.ipalloc.p->data[offset];

        net_encode_u32(&out[0], 0x0ced0001); // signature
        net_encode_u32(&out[4], transaction_id);
        net_encode_u64(&out[8], nkern_utime());
        net_encode_u32(&out[16], command_id);
        len = 20;

        switch (command_id)
        {
        case 0x0001: // GET_STATUS
        {
            len += get_state(&out[len]);
            break;
        }

        case 0x0002: // GET_VERSION
        {
            for (int i = 0; ; i++) {
                out[len++] = FIRMWARE_REVISION[i];
                if (FIRMWARE_REVISION[i] == 0)
                    break;
            }
            break;
        }

        case 0x1000: // MOTOR SET
        {
            int ncmds = (insz - 20)/4;

            if (ncmds <= 3) {
                for (int i = 0; i < ncmds; i++) {
                    uint32_t port  = in[20+4*i];
                    int8_t   enable = in[21+4*i];
                    int16_t  pwm    = (in[22+4*i]<<8) | in[23+4*i];

                    int error = 0;

                    if (port > 2)
                        error = 1;

                    if (!error) {
                        irqstate_t flags;
                        interrupts_disable(&flags);
                        motor_set_goal_pwm(port, pwm);
                        interrupts_restore(&flags);

                        motor_set_enable(port, enable);
                    }

                    out[len++] = error;
                }
            }

            break;
        }

        case 0x1001: // MOTOR SLEW SET
        {
            uint32_t port = in[20];
            uint32_t slew = (in[21]<<8) | in[22];

            int error = 0;

            if (port > 2)
                error = 1;

            if (!error)
                motor_set_slew(port, slew);

            out[len++] = error;
            break;
        }

        case 0x1002: // MOTOR WATCHDOG SET
        {
            int error = 0;
            uint32_t usecs = (in[20]<<24) | (in[21]<<16) | (in[22]<<8) | (in[23]);

            if (!error)
                motor_set_watchdog_timeout(usecs);

            out[len++] = error;
            break;
        }

        case 0x3000: // ANALOG FILTER SET
        {
            uint32_t port = in[20];
            uint32_t alpha = (in[21]<<8) | in[22];

            int error = 0;
            if (port > 13)
                error = 1;

            if (!error) {
                if (port < 8)
                    extadc_set_filter_alpha(port, alpha);
                else
                    intadc_set_filter_alpha(port - 8, alpha);
            }

            out[len++] = error;

            break;
        }

        case 0x4000: // SSI
        {
            uint32_t maxclk = ((in[20]<<8)+in[21]) * 1000; // given as kHz
            uint8_t  flags = in[22];
            uint32_t nbits = (flags>>0)&31;
            uint32_t spo = (flags>>6)&1;
            uint32_t sph = (flags>>7)&1;

            uint32_t nwords = in[23];

            if (nwords > 16) {
                out[len++] = 1;
                break;
            }

            ssi_lock();

            ssi_config(maxclk, spo, sph, nbits);

            uint32_t tx[nwords];
            uint32_t rx[nwords];


            for (uint32_t i = 0; i < nwords; i++)
                tx[i] = (in[24+2*i]<<8) + in[24+2*i+1];

            gpio_set_bit(GPIO_PORTD_DATA_BITS_R, 7, 0); // assert user SS

            ssi_rxtx(tx, rx, nwords);

            out[len++] = 0; // no error.
            out[len++] = nwords;
            for (uint32_t i = 0; i < nwords; i++) {
                out[len++] = (rx[i]>>8)&0xff;
                out[len++] = rx[i]&0xff;
            }

            gpio_set_bit(GPIO_PORTD_DATA_BITS_R, 7, 1); // de-assert user SS

            ssi_unlock();

            break;
        }

        case 0x5000: // i2c
        {
            // command format:
            // uint8_t: addr
            // uint8_t: options
            // uint8_t: writelen0
            // uint8_t: readlen0
            // uint8_t[writelen0]: writebuf
            // uint8_t: writelen1               <--- repeat 0 or more transactions
            // uint8_t: readlen1
            // uint8_t[writelen1]: writebuf
            // ...

            // response format:
            // uint8_t: error
            // uint8_t: readlen0
            // uint8_t[readlen0]: readbuf
            // uint8_t: readlen1
            // uint8_t[readlen1]: readbuf
            // ...

            int32_t inpos = 20;
            uint32_t addr = in[inpos++];
            uint32_t options = in[inpos++];

            if (options & 1)
                i2c_config(400000); // 400khz mode (more like 357 kbps)
            else
                i2c_config(100000);

            while (inpos < insz) {
                uint32_t writelen = in[inpos++];
                uint32_t readlen = in[inpos++];

                uint8_t rxbuf[16];

                int error = 0;
                if (writelen > 16 || readlen > 16)
                    error = -1;

                if (addr > 127)
                    error = -2;

                if (error == 0)
                    error = i2c_transaction(addr, &in[inpos], writelen, rxbuf, readlen);

                inpos += writelen;

                out[len++] = error;
                out[len++] = readlen;

                for (uint32_t i = 0; i < readlen; i++) {
                    out[len++] = rxbuf[i];
                }

//                if (inpos < insz)
//                    nkern_usleep(5000);
            }

            break;
        }

        case 0x6000: // simple digital IO: configure I/O direction & pullups
        {
            uint8_t port = in[20];  // port = [0,7], always on portb (porta is used internally).
            uint8_t iodir = in[21];   // 0 = output, 1 = input
            uint8_t pullup = in[22];  // 0 = no pullup, 1 = pullup (iff configured as input)

            int error = 0;
            if (port > 7)
                error = -1;
            if (iodir > 1)
                error = -2;
            if (pullup > 1)
                error = -3;

            if (!error)
                digio_configure(port, iodir, pullup);

            out[len++] = error;
            break;
        }

        case 0x6001: // simple digital IO: set I/O output value
        {
            uint8_t port = in[20];
            uint8_t value = in[21];

            int error = 0;

            if (port > 7)
                error = -1;

            if (value > 1)
                error = -2;

            if (!error)
                digio_set(port, value);

            out[len++] = error;
            break;
        }

        // fast digital IO
        case 0x7000:
        {
            int ncmds = (insz - 20) / 6;

            if (ncmds <= 7) {
                for (int i = 0; i < ncmds; i++) {
                    int error = 0;
                    uint8_t port = in[20 + 6*i];
                    uint8_t mode = in[21 + 6*i];

                    uint32_t value = (in[22+6*i]<<24) | (in[23+6*i]<<16) |
                        (in[24+6*i]<<8) | (in[25+6*i]);

                    if (port > 7)
                        error = 1;

                    if (!error)
                        fast_digio_set_configuration(port, mode, value);

                    out[len++] = error;
                }
            }
            break;
        }

        // shiftbrite ring used in MAGIC 2010. uses fast digital I/O ports 0, 1, 3.
        case 0x7f01:
        {
            shiftbrite_init(); // reinit if necessary.

            uint32_t rgb_on[3], rgb_off[3];

            int cmdsz = 20;
            int ncmds = (insz - cmdsz) / cmdsz;

            for (int i = 0; i < ncmds; i++) {
                uint32_t ring = in[20 + cmdsz*i];

                rgb_on[0] = (in[21+cmdsz*i]<<8) | in[22+cmdsz*i];
                rgb_on[1] = (in[23+cmdsz*i]<<8) | in[24+cmdsz*i];
                rgb_on[2] = (in[25+cmdsz*i]<<8) | in[26+cmdsz*i];
                rgb_off[0] = (in[27+cmdsz*i]<<8) | in[28+cmdsz*i];
                rgb_off[1] = (in[29+cmdsz*i]<<8) | in[30+cmdsz*i];
                rgb_off[2] = (in[31+cmdsz*i]<<8) | in[32+cmdsz*i];

                uint32_t period_ms = (in[33+cmdsz*i]<<8) | in[34+cmdsz*i];
                uint32_t on_ms = (in[35+cmdsz*i]<<8) | in[36+cmdsz*i];
                uint32_t count = (in[37+cmdsz*i]<<8) | in[38+cmdsz*i];
                uint32_t sync = in[39 + cmdsz*i];
                shiftbrite_set(ring, rgb_on, rgb_off, period_ms, on_ms, count, sync);
            }

            out[len++] = 0;
            break;
        }

        case 0xdead:
        {
            // crash the uorc board. (used for testing watch dog timer)
            irqstate_t flags;
            interrupts_disable(&flags);

            while (1) {
            }

            break;
        }

        case 0xdeae:
        {
            // send response now!
            out[len++] = 0;
            udp_packet_send(&udpalloc, len, ORC_BASE_PORT+0, net_decode_u16(rxdata->udp_hdr->src_port));
            nkern_usleep(50000); // wait for packet to be sent

            watchdog_disable();

            servo_stop();
            irqstate_t flags;
            interrupts_disable(&flags);

            isp_eth();

            while(1);
            break;
        }

        case 0xff00: // parameter block read
        {
            for (int i = 0; i < 256; i++) {
                out[len++] = ((uint8_t*) flash_params)[i];
            }
            break;
        }

        case 0xff01: // parameter block write and reset board
        {
            flash_erase_and_write((uint32_t) flash_params, &in[20], insz - 20);

            out[len++] = 0;

            irqstate_t flags;
            interrupts_disable(&flags);

            while (1) {
            }

            break;
        }

        default:
            // command not known.
            break;
        }

        udp_packet_send(&udpalloc, len, ORC_BASE_PORT+0, net_decode_u16(rxdata->udp_hdr->src_port));

    cleanup:

        udp_read_release(&ul, rxdata);
    }

}

static int ax12_slow_count = 0;    // total transaction was > 10 ms
static int ax12_timeout_count = 0; // no character for 5ms
static int ax12_fakeout_count = 0; // thought we had a character, but then no.
static int ax12_flush_count = 0;   // unexpected characters discarded
static int ax12_bad_response_count = 0;

int ax12_read_char()
{
    nkern_wait_t waits[1];

    uint64_t endtime = nkern_utime() + 5000;
    while (1) {
        serial1_read_wait_setup(&waits[0]);
        int ev = nkern_wait_many_until(waits, 1, endtime);
        if (ev < 0) {
            ax12_timeout_count++;
            return -1;
        }

        int c = serial1_getc_nonblock();
        if (c < 0) {
            ax12_fakeout_count++;
            continue;
        }

        return c;
    }
}

int ax12_read_response(char *buf, int buflen)
{
    int c;

restart:
    // HEADER
    if ((c = ax12_read_char()) < 0)
        return -1;

    if (c != 0xff)
        goto restart;

    if ((c = ax12_read_char()) < 0)
        return -1;

    if (c != 0xff)
        goto restart;

    buf[0] = 0xff;
    buf[1] = 0xff;

    // ID
    if ((c = ax12_read_char()) < 0)
        return -1;
    buf[2] = c;

    // LEN
    if ((c = ax12_read_char()) < 0)
        return -1;
    buf[3] = c;

    int len = buf[3] & 0xff;

    // message too long?
    if (len + 3 >= buflen)
        goto restart;

    // PAYLOAD + CHECKSUM
    for (int i = 0; i < len; i++) {

        if ((c = ax12_read_char()) < 0)
            return -1;

        buf[4+i] = c;
    }

    return (4 + len);
}

static int packet_alloc_fail_count = 0;

// handles slow serial commands (ORC_BASE_PORT + 1)
void cmd1_task(void *arg)
{
    udp_listener_t ul;
    udp_rxdata_t *rxdata;

    udp_listen(&ul, ORC_BASE_PORT + 1);

    while(1) {
        udp_read(&ul, &rxdata);

        uint8_t *in = rxdata->udp_hdr->payload;
        int      insz = net_decode_u16(rxdata->udp_hdr->length) - UDP_HEADER_SIZE;

        if (insz < 20)
            goto cleanup;

        udp_alloc_t udpalloc;
        int         offset;

        uint32_t header = net_decode_u32(&in[0]);
        uint32_t transaction_id = net_decode_u32(&in[4]);
        uint64_t sender_utime = net_decode_u64(&in[8]);
        uint32_t command_id = net_decode_u32(&in[16]);

        if (header!=0x0ced0002)
            goto cleanup;

        if (udp_packet_alloc(&udpalloc, net_decode_ip_addr(rxdata->ip_hdr->src_ip), &offset, 1024)) {
            packet_alloc_fail_count++;
            goto cleanup;
        }

        //////////////////////////////////
        int len = 0;
        uint8_t *out = &udpalloc.ipalloc.p->data[offset];

        net_encode_u32(&out[0], 0x0ced0001); // signature
        net_encode_u32(&out[4], transaction_id);
        net_encode_u64(&out[8], nkern_utime());
        net_encode_u32(&out[16], command_id);
        len = 20;

        switch (command_id)
        {
            // AX-12 servo on uart1
        case 0x01007a00:
        {
            uint32_t length = insz - 20;

            // flush read buffer.
            while (serial1_getc_nonblock() >= 0)
                ax12_flush_count++;

            int64_t t0 = nkern_utime();

            serial1_halfduplex_write((char*) &in[20], length);

            char resp[128];
            int resplen = ax12_read_response(resp, 128);
            out[len++] = resplen; // < 0 if error.

            if (resplen > 0) {
                for (int i = 0; i < resplen; i++)
                    out[len++] = resp[i];
            }

            if (resplen < 0) {
                ax12_bad_response_count++;
            }

            int64_t t1 = nkern_utime();

            if (t1 - t0 > 10000) {
                ax12_slow_count++;
            }

            break;
        }

        // debug info
        case 0x01007d00:
        {
            out[len++] = ax12_slow_count;
            out[len++] = ax12_timeout_count;
            out[len++] = ax12_fakeout_count;
            out[len++] = ax12_flush_count;
            out[len++] = ax12_bad_response_count;
            break;
        }

        default:
            // command not known.
            break;
        }

        udp_packet_send(&udpalloc, len, ORC_BASE_PORT+1, net_decode_u16(rxdata->udp_hdr->src_port));

    cleanup:

        udp_read_release(&ul, rxdata);
    }
}

void command_init()
{
    nkern_task_create("cmd0_task",
                      cmd0_task, NULL,
                      NKERN_PRIORITY_NORMAL, 1024);

    nkern_task_create("cmd1_task",
                      cmd1_task, NULL,
                      NKERN_PRIORITY_NORMAL, 1024);

}
