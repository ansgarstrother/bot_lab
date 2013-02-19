#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
//#include "assert.h"
#include "lm3s8962.h"
#include "luminary_isp.h"

#define NKERN_STACK_MAGIC_NUMBER 0x5718a9bf
#define CPU_HZ (50E6)

void init(void); // forward decl
int main(void); // forward decl

#define PACKET_SIZE 2048
#define ETH_TYPE 0x6006

uint8_t rxbuf[PACKET_SIZE];
uint8_t txbuf[PACKET_SIZE];

uint32_t _nkern_stack[512];
extern uint32_t _nkern_stack[], _nkern_stack_top[];

extern uint32_t _heap_end[];
extern uint32_t _edata[], _etext[], _data[];
extern uint32_t __bss_start__[], __bss_end__[];

typedef uint64_t eth_addr_t;

eth_addr_t eth_addr = 0x02000000ededULL;

__attribute__ ((section("vectors")))
void *_vects[] = {
    _heap_end,                              // 00    reset stack pointer
    init                                    // 01    Reset program counter
};

static inline void nop()
{
	asm volatile ("nop \r\n");
}

/** Atomically update the value of bit [0,7] to v [0,1]. basereg
 * should be GPIO_PORTA_DATA_BITS_R, not GPIO_PORTA_DATA_R.**/
static inline void gpio_set_bit(volatile uint32_t *basereg, int bit, int v)
{
    volatile uint32_t *bitreg = (volatile uint32_t*) (((uint32_t) basereg) + (1<<(bit+2)));
    *bitreg = (v << bit);
}

void status_led(int v)
{
    gpio_set_bit(GPIO_PORTF_DATA_BITS_R, 0, v);
}

static inline uint16_t ntohs(uint16_t v)
{
    return ((v & 0x00ff) << 8) | ((v & 0xff00) >> 8);
}

#define htons ntohs

static inline uint32_t ntohl(uint32_t v)
{
    return ((v & 0x000000ff) << 24) | ((v & 0x0000ff00) << 8) |
      ((v & 0x00ff0000) >> 8) | ((v & 0xff000000) >> 24);
}

void net_encode_u32(uint8_t *p, uint32_t v)
{
    p[3] = v & 0xff;
    v >>= 8;
    p[2] = v & 0xff;
    v >>= 8;
    p[1] = v & 0xff;
    v >>= 8;
    p[0] = v & 0xff;
}

uint32_t net_decode_u32(uint8_t *p)
{
    uint32_t v = 0;

    for (int i = 0; i < 4; i++)
        v = (v << 8) | p[i];

    return v;
}

struct eth_packet
{
    uint8_t *buf;
    uint32_t length;
    uint32_t maxlen;

    uint32_t type;
    eth_addr_t src, dest;
};

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

int ethernet_tx(struct eth_packet *p)
{
    while(1) {
        if (MAC_TR_R & (1<<0)) {
            continue; //MAC_IM_R |= (1<<2); // enable TX FIFO empty IRQ
        } else {
            break;
        }
    }

    uint32_t *data = (uint32_t*) p->buf;

    // Send the packet.
    uint8_t octets[6];
    net_encode_eth_addr(octets, p->dest);
    MAC_DATA_R = (p->length) | (octets[0]<<16) | (octets[1]<<24);
    MAC_DATA_R = octets[2] | (octets[3]<<8) | (octets[4]<<16) | (octets[5]<<24);

    net_encode_eth_addr(octets, p->src);
    MAC_DATA_R = octets[0] | (octets[1]<<8) | (octets[2]<<16) | (octets[3]<<24);
    MAC_DATA_R = octets[4] | (octets[5]<<8) | (htons(p->type)<<16);

    for (uint32_t idx = 0; idx < p->length; idx+=4)
        MAC_DATA_R = *data++;

    MAC_TR_R = (1<<0);

    return 0;
}

// 0-1: length
// 2-7: dest eth addr
// 8-13: source eth addr
// 14-15: frame_type
int ethernet_rx(struct eth_packet *p)
{
    while (1) {
        uint32_t npackets = (MAC_NP_R & 0xff);
        if (npackets == 0)
            continue;
        break;
    }

    uint32_t hdr32[4];
    uint8_t *hdr8 = (uint8_t*) hdr32;

//    assert(nwords >= 4);

    for (int i = 0; i < 4; i++)
        hdr32[i] = MAC_DATA_R;

    // the hardware includes length field, header, and FCS in length
    // computation. We've already read 16 bytes, subtract those. We
    // still need to read the data payload and FCS.
    p->length  = (hdr8[0]) + (hdr8[1]<<8) - 2 - 14 - 4;
    uint32_t nwords = (p->length + 3) / 4;

    p->dest    = (((eth_addr_t) hdr8[2])<<40) +
        (((eth_addr_t) hdr8[3])<<32) +
        (((eth_addr_t) hdr8[4])<<24) +
        (((eth_addr_t) hdr8[5])<<16) +
        (((eth_addr_t) hdr8[6])<<8) +
        (((eth_addr_t) hdr8[7])<<0);

    p->src    = (((eth_addr_t) hdr8[8])<<40) +
        (((eth_addr_t) hdr8[9])<<32) +
        (((eth_addr_t) hdr8[10])<<24) +
        (((eth_addr_t) hdr8[11])<<16) +
        (((eth_addr_t) hdr8[12])<<8) +
        (((eth_addr_t) hdr8[13])<<0);

    p->type    = (hdr8[14]<<8) + (hdr8[15]<<0);

    uint32_t *d = (uint32_t*) p->buf;

    for (uint32_t i = 0; i < nwords; i++) {
        uint32_t w = MAC_DATA_R;
        if (i < p->maxlen/4)
            d[i] = w;
    }

    uint32_t fcs = MAC_DATA_R;
    return 0;
}

void init()
{
    // copy .data section
    for (uint32_t *src = _etext, *dst = _data ; dst != _edata ; src++, dst++)
        (*dst) = (*src);

    // zero .bss section
    for (uint32_t *p = __bss_start__; p != __bss_end__; p++)
        (*p) = 0;

    // sign the kernel stack; this assumes we're NOT called from the kernel stack.
    for (uint32_t *p =  _nkern_stack; p != _nkern_stack_top; p++) {
        *p = NKERN_STACK_MAGIC_NUMBER;
    }

    // Set the process stack to the top of the stack
    asm volatile("msr PSP, %0                      \r\n\t"
                 "msr MSP, %0                      \r\n\t"
                 :: "r" (_nkern_stack_top));

    // we've tampered with the stack pointers which will make local
    // variables unreliable. We'll finish our initialization from a
    // new stack frame.
    main();
}

void ethernet_init()
{
    uint32_t pclk = CPU_HZ;

    // power on peripheral
    SYSCTL_RCGC2_R |= (1<<28) | (1<<30); // emac0 and ephy0
    SYSCTL_RCGC2_R |= (1<<5);            // port F gpio (leds)

    // reset peripheral
    SYSCTL_SRCR2_R |= (1<<28) | (1<<30);
    SYSCTL_SRCR2_R &= ~((1<<28) | (1<<30));

    // set pin functions (note: actual ethernet pins are dedicated,
    // don't need to be configured.)
    GPIO_PORTF_DIR_R  &= ~((1<<2) | (1<<3)); // 1 = output
    GPIO_PORTF_AFSEL_R |= (1<<2) | (1<<3);   // 1 = hw func
    GPIO_PORTF_DEN_R   |= (1<<2) | (1<<3);   // 1 = gpio enabled

    // set the clock divider
    uint32_t div = pclk / 2500000;
    MAC_MDV_R = div & MAC_MDV_DIV_M;

    uint32_t tmp;

    tmp = MAC_TCTL_R;
    tmp |= (1<<4) | (1<<2) | (1<<1);
    MAC_TCTL_R = tmp;

    tmp = MAC_RCTL_R;
    tmp |= (1<<4) | (1<<3) | (1<<1); // promiscuous 1<<2
    MAC_RCTL_R = tmp;

    // IEEE1588 support
    // MAC_TS_R |= (1<<0);

    uint8_t octets[8];
    net_encode_eth_addr(octets, eth_addr);

    MAC_IA0_R = octets[0] | (octets[1]<<8) | (octets[2]<<16) | (octets[3]<<24);
    MAC_IA1_R = octets[4] | (octets[5]<<8);

    // turn everything on!
    MAC_RCTL_R |= (1<<0);
    MAC_TCTL_R |= (1<<0);

    // reset the rx fifo
    MAC_RCTL_R |= (1<<4);

    // reset phy autonegotiation
//    phy_write(0, phy_read(0) | (1<<9));
//    phy_write(0, (1<<15) | (1<<13) | (1<<12) | (1<<9) | (1<<8));

//    phy_write(24, 0); // disable MDIX: WORKAROUND FOR UORC R2a

    MAC_IM_R = 0;

    // enable interrupts
    NVIC_EN1_R |= (1<<10);

}

/** addr1k is a 1kb-aligned address. **/
void flash_erase(uint32_t addr1k)
{
    FLASH_FMA_R = addr1k;

    FLASH_FMC_R = 0xA4420002; // magic key + erase

    while (FLASH_FMC_R & FLASH_FMC_ERASE);
}

/** Assume addr is 4-byte aligned. **/
void flash_write(uint32_t addr, uint32_t value)
{
    if (*((uint32_t*)addr)==value)
        return;

    FLASH_FMD_R = value;
    FLASH_FMA_R = addr;

    FLASH_FMC_R = 0xA4420001; // magic key + write byte

    while (FLASH_FMC_R & FLASH_FMC_WRITE);
}

int main() __attribute__((noinline));
int main()
{
    SYSCTL_RCGC2_R |= 0x7f;    // enable all GPIO, PORTS A-G
    SYSCTL_RCGC1_R |= (1<<0);  // UART0
    SYSCTL_RCGC2_R |= (1<<28) | (1<<30); // emac0 and ephy0

    // set up pll
    if (1) {
        // rev a2 errata, bump LDO up to 2.75
        SYSCTL_LDOPCTL_R = SYSCTL_LDOPCTL_2_75V;

        if (CPU_HZ==50000000) {

            SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                           SYSCTL_XTAL_8MHZ);
        } else {
            // 8 MHz?
            SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                           SYSCTL_XTAL_8MHZ);
        }
    }

    ////////////////////////////////////
    // set up pins

    // UART0:  RX = PA0/U0RX
    //         TX = PA1/U0TX
    // handled by serial.c

    // UART1:  RX = PD2/U1RX
    //         TX = PD3/U1TX
    // handled by serial.c

    // LED_MOTORFAULT: PF1/DIX1
    GPIO_PORTF_DIR_R |= (1<<0);  // high current digital output, slow slew
    GPIO_PORTF_DEN_R |= (1<<0);
    GPIO_PORTF_DR8R_R |= (1<<0);
    GPIO_PORTF_SLR_R |= (1<<0);
    GPIO_PORTF_DATA_R &= ~(1<<0); // off for now.

    /// initialize brown-out reset (BOR)
    SYSCTL_PBORCTL_R |= SYSCTL_PBORCTL_BORIOR;

    /// initialize brown-out reset (BOR)
    SYSCTL_PBORCTL_R |= SYSCTL_PBORCTL_BORIOR;

    ethernet_init();

    struct eth_packet rxpacket;
    memset(&rxpacket, 0, sizeof(rxpacket));
    rxpacket.buf = rxbuf;
    rxpacket.maxlen = PACKET_SIZE;

    struct eth_packet txpacket;
    memset(&txpacket, 0, sizeof(txpacket));
    txpacket.buf = txbuf;
    txpacket.maxlen = PACKET_SIZE;
    txpacket.type = ETH_TYPE;
    txpacket.src = eth_addr;

    // very simple loop: read a packet, do something.
    for (int __cnt = 0; 1; __cnt++) {

        int res = ethernet_rx(&rxpacket);
        if (res < 0)
            continue;

        if (rxpacket.length < 8)
            continue;

        ///////////////////////////////
        // handle the command
        if (rxpacket.type != ETH_TYPE)
            continue;

        status_led(__cnt&1);

        uint32_t tid = net_decode_u32(&rxpacket.buf[0]);
        uint32_t cmd = net_decode_u32(&rxpacket.buf[4]);

        // all responses begin the same way.
        net_encode_u32(&txpacket.buf[0], tid);
        net_encode_u32(&txpacket.buf[4], 0x80000000 | cmd);
        txpacket.dest = rxpacket.src;

        switch (cmd)  {
        case 0: {
            // PING
            memcpy(&txpacket.buf[8], &rxpacket.buf[8], rxpacket.length-8);

            txpacket.length = rxpacket.length;
            ethernet_tx(&txpacket);
            break;
        }

        case 1: {
            // ERASE
            uint32_t addr = net_decode_u32(&rxpacket.buf[8]);

            if (addr == (addr & 0xfffffc00)) {
                // save erase cycles: only erase if we need to.
                uint8_t *p = (uint8_t*) addr;
                int dirty = 0;
                for (int i = 0; i < 1024; i++) {
                    if (p[i] != 0xff)
                        dirty = 1;
                }
                if (dirty)
                    flash_erase(addr);

                net_encode_u32(&txpacket.buf[8], 0); // success
            } else {
                net_encode_u32(&txpacket.buf[8], 1); // bad addr
            }

            txpacket.length = 12;
            ethernet_tx(&txpacket);
            break;
        }

        case 2: {
            // WRITE + VERIFY
            uint32_t addr = net_decode_u32(&rxpacket.buf[8]);
            uint32_t len = net_decode_u32(&rxpacket.buf[12]);

            if ((len&3)==0 && rxpacket.length == (len + 16) && len <= 1024) {
                uint32_t *d = (uint32_t*) &rxpacket.buf[16];
                for (uint32_t i = 0; i < len/4; i++) {
                    uint32_t v = d[i]; // endianess?
                    flash_write(addr, v);
                    addr += 4;
                }
                net_encode_u32(&txpacket.buf[8], 0); // success
            } else {
                net_encode_u32(&txpacket.buf[8], 1); // bad addr
            }

            txpacket.length = 12;
            ethernet_tx(&txpacket);
            break;
        }

        case 3: {
            // READ
            uint32_t addr = net_decode_u32(&rxpacket.buf[8]);
            uint32_t len = net_decode_u32(&rxpacket.buf[12]);

            if (len <= 1024) {
                net_encode_u32(&txpacket.buf[8], 0); // success

                uint8_t *p = (uint8_t*) addr;
                for (uint32_t i = 0; i < len; i++)
                    txpacket.buf[12+i] = p[i];

                txpacket.length = 12 + len;
            } else {
                net_encode_u32(&txpacket.buf[8], 1); // bad param
                txpacket.length = 12;
            }

            ethernet_tx(&txpacket);
            break;
        }

        case 4: {
            // RESET
            net_encode_u32(&txpacket.buf[8], 0); // success
            txpacket.length=12;
            ethernet_tx(&txpacket);

            // wait for tx to finish.
            for (int i = 0; i < 10E6; i++) {
                asm volatile ("nop");
            }

            NVIC_APINT_R = (0x5fa<<16) | (1<<2); // VECTRESET
            while(1);
            break;
        }

        default: {
            // send error (XXX or do nothing?)
            net_encode_u32(&txpacket.buf[8], -1); // bad cmd
            txpacket.length = 12;
            ethernet_tx(&txpacket);
            break;
        }

        }
    }
}

