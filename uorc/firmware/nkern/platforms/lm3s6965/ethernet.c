#include <nkern.h>
#include <net.h>
#include "lm3s6965.h"
#include <stdint.h>

// If non-zero, we will drop some fraction of our incoming and outgoing packets
#define DEBUG_DROP_PACKETS     0

// The mask determines which power-of-two fraction we drop. 1 = half, 3 = 25%, 7 = 12.5%, etc.
#define DEBUG_DROP_MASK        (7)

static uint32_t prng_state = 0x12345678;
static uint32_t next_prng()
{
    uint32_t v = prng_state;
    uint32_t bit = ((v >> 0) ^ (v >> 2) ^ (v >> 3) ^ (v >> 5));
    prng_state = (v << 1) | (bit & 1);

    return prng_state;
}


static uint32_t drop_packet_counter = 0;

#define ETH_CFG_TS_TSEN         0x010000    // Enable Timestamp (CCP)
#define ETH_CFG_RX_BADCRCDIS    0x000800    // Disable RX BAD CRC Packets
#define ETH_CFG_RX_PRMSEN       0x000400    // Enable RX Promiscuous
#define ETH_CFG_RX_AMULEN       0x000200    // Enable RX Multicast
#define ETH_CFG_TX_DPLXEN       0x000010    // Enable TX Duplex Mode
#define ETH_CFG_TX_CRCEN        0x000004    // Enable TX CRC Generation
#define ETH_CFG_TX_PADEN        0x000002    // Enable TX Padding

eth_dev_t eth;

static void ethernet_rx_task(void *arg);

static nkern_wait_list_t rx_waitlist; // either empty or contains the ethernet_rx_task
static nkern_wait_list_t tx_waitlist; // all tasks currently waiting to transmit
static nkern_mutex_t     tx_mutex; 
static nkern_mutex_t     phy_mutex;

#define NUM_PACKETS 12
#define PACKET_MAX_SIZE 1600

static nkern_queue_t packet_queue = NKERN_QUEUE_ALLOC(NUM_PACKETS);

// non-zero if we have a valid link:  10 = 10mbps, 
//                                    11 = 10mbps full duplex
//                                   100 = 100mbps,
//                                   101 = 100mbps full duplex
static int ethernet_link;

// assumes little-endian
static void ethernet_irq_real(void) __attribute__ ((noinline));
static void ethernet_irq_real()
{
    uint32_t reschedule = 0;

    // clear/ack all irqs
    MAC_IACK_R = (1<<6) | (1<<5) | (1<<4) | (1<<3) | (1<<2) | (1<<1) | (1<<0);

    uint32_t macris = MAC_RIS_R;

    if (~(MAC_TR_R & (1<<0))) {
        // clear to send...
        reschedule |= _nkern_wake_one(&tx_waitlist);
        MAC_IM_R &= ~(1<<2); // disable TX FIFO empty IRQs
    }

    if ((MAC_NP_R & 0xff) != 0) {
        // we have data!
        reschedule |= _nkern_wake_all(&rx_waitlist);
        MAC_IM_R &= ~(1<<0); // disable RX IRQ
    }

    if (reschedule)
        _nkern_schedule();
}

static void ethernet_irq(void) __attribute__ ((naked));
static void ethernet_irq(void)
{
    IRQ_TASK_SAVE;
    NKERN_IRQ_ENTER;
    ethernet_irq_real();
    NKERN_IRQ_EXIT;
    IRQ_TASK_RESTORE;
}


static net_packet_t *ethernet_packet_alloc(net_dev_t *dev, int needed_size, int *offset)
{
    if (needed_size > PACKET_MAX_SIZE)
        return NULL;

    net_packet_t *p = (net_packet_t*) nkern_queue_get(&packet_queue, 1);
    *offset = 0;
    p->size = needed_size;

    return p;
}

/* // phy_write should work, but has no users.

static void phy_write(uint32_t phy_addr, uint32_t value)
{
    nkern_mutex_lock(&phy_mutex);

    // wait for pending transaction to complete
    while (MAC_MCTL_R & MAC_MCTL_START)
        nkern_yield();

    // write PHY address, start transaction.
    MAC_MTXD_R = value & MAC_MTXD_MDTX_M;
    MAC_MCTL_R = (phy_addr << 3) | MAC_MCTL_WRITE | MAC_MCTL_START;

    // wait for transaction to finish
    while (MAC_MCTL_R & MAC_MCTL_START)
        nkern_yield();

    nkern_mutex_unlock(&phy_mutex);
}
*/

static uint32_t phy_read(uint32_t phy_addr)
{
    nkern_mutex_lock(&phy_mutex);

    // wait for pending transaction to complete
    while (MAC_MCTL_R & MAC_MCTL_START)
        nkern_yield();

    // write PHY address, start transaction.
    MAC_MCTL_R = (phy_addr << 3) | MAC_MCTL_START;

    // wait for transaction to finish
    while (MAC_MCTL_R & MAC_MCTL_START)
        nkern_yield();

    uint32_t v = MAC_MRXD_R & MAC_MRXD_MDRX_M;

    nkern_mutex_unlock(&phy_mutex);
    return v;
}

static int32_t ethernet_periodic_task(void *arg)
{
    uint32_t r1 = phy_read(1);
    uint32_t v18 = phy_read(18);
    
    int link_good = r1 & (1<<2);

    if (!link_good) {
        ethernet_link = 0;
    } else {
        uint32_t rate = 10; // 10mbps
        if (v18 & (1<<10))
            rate = 100;
        uint32_t duplex = 0;
        if (v18 & (1<<11))
            duplex = 1;

        ethernet_link = rate + duplex;
    }

    return 0; // run me again, please.
}

static void ethernet_rx_task(void *arg)
{
    while (1) {

        // wait until a packet is available in the FIFO
        irqstate_t flags;
        interrupts_disable(&flags);
        
        while (1) {
            uint32_t npackets = (MAC_NP_R & 0xff);
            if (npackets == 0) {
                MAC_IM_R |= (1<<0); // enable RX irqs
                nkern_wait(&rx_waitlist);
            } else {
                break;
            }
        }
        
        interrupts_restore(&flags);
        
        uint32_t tmp = MAC_DATA_R; // read the first word
        uint32_t frame_length = tmp & 0xffff;
        int nwords = (frame_length + 3) / 4;

        int offset;
        net_packet_t *packet = ethernet_packet_alloc((net_dev_t*) &eth, frame_length, &offset);
        if (packet == NULL) {
            // discard packet
            for (int i = 1; i < nwords; i++)
                tmp = MAC_DATA_R;
            continue;
        }
            
        packet->size = frame_length;
        assert(offset == 0);
        uint32_t *d = (uint32_t*) packet->data;
        d[0] = tmp;

        for (int i = 1; i < nwords; i++)
            d[i] = MAC_DATA_R;
        
        // byte order is backwards on these...
        eth_addr_t  dest_addr    = (d[0]>>16) | (d[1]<<16);
        eth_addr_t  src_addr     = d[2] | (((eth_addr_t) (d[3]&0xffff))<<32);

        uint32_t    frame_type   = d[3]>>16;

        if (DEBUG_DROP_PACKETS) {
            if ((next_prng()&DEBUG_DROP_MASK)==0) {
                packet->free(packet);
                continue;
            }
        }

        net_ethernet_receive(packet, 2);
    }
}

static int ethernet_packet_send(net_dev_t *netdev, net_packet_t *p, ip4_addr_t ip_via, int type)
{
    eth_addr_t eth_dest;
    assert(((void*) netdev) == ((void*) &eth));

    if (DEBUG_DROP_PACKETS)  {
        if ((next_prng()&DEBUG_DROP_MASK)==0) {
            p->free(p);
            return 0;
        }
    }

    if (net_arp_query(&eth, ip_via, &eth_dest)) {
        p->free(p);
        return -1;
    }

    if (ethernet_link == 0) {
        p->free(p);
        return -1;
    }

    // XXX race condition: if ethernet link goes down while we're sending, 
    // will we escape?
    nkern_mutex_lock(&tx_mutex);
    irqstate_t flags;
    interrupts_disable(&flags);
    
    while(1) {

        if (MAC_TR_R & (1<<0)) {
            MAC_IM_R |= (1<<2); // enable TX FIFO empty IRQ
            nkern_wait(&tx_waitlist);
        } else {
            break;
        }

        if (ethernet_link == 0)
            break;
    }

    interrupts_restore(&flags);

    uint32_t length = p->size;
    uint32_t *data = (uint32_t*) p->data;

    // Send the packet.
    uint8_t octets[6];
    net_encode_eth_addr(octets, eth_dest);
    MAC_DATA_R = (length) | (octets[0]<<16) | (octets[1]<<24);
    MAC_DATA_R = octets[2] | (octets[3]<<8) | (octets[4]<<16) | (octets[5]<<24);

    net_encode_eth_addr(octets, eth.eth_addr);
    MAC_DATA_R = octets[0] | (octets[1]<<8) | (octets[2]<<16) | (octets[3]<<24);
    MAC_DATA_R = octets[4] | (octets[5]<<8) | (htons(type)<<16);

    for (uint32_t idx = 0; idx < length; idx+=4)
        MAC_DATA_R = *data++;

    MAC_TR_R = (1<<0);

    nkern_mutex_unlock(&tx_mutex);
    
    p->free(p);

    return 0;
}

void free_packet(net_packet_t *packet)
{
    nkern_queue_put(&packet_queue, packet, 0);
}

net_dev_t *ethernet_init(uint32_t pclk, eth_addr_t eth_addr)
{
    nkern_wait_list_init(&rx_waitlist, "enet_rx");
    nkern_wait_list_init(&tx_waitlist, "enet_tx");
    nkern_mutex_init(&tx_mutex, "enet_tx");
    nkern_queue_init(&packet_queue, "enet_packet");
    nkern_mutex_init(&phy_mutex, "enet_phy");

    eth.eth_addr = eth_addr;

    for (int i = 0; i < NUM_PACKETS; i++) {
        net_packet_t *packet = (net_packet_t*) calloc(1, sizeof(net_packet_t));
        packet->data = malloc(PACKET_MAX_SIZE);
        packet->capacity = PACKET_MAX_SIZE;
        packet->size = 0;
        packet->netdev = (net_dev_t*) &eth;
        packet->free = free_packet;
        nkern_queue_put(&packet_queue, packet, 0);
    }

    nkern_task_create("enet_rx", ethernet_rx_task, NULL, NKERN_PRIORITY_HIGH, 1024);
    nkern_task_create_periodic("enet_status", ethernet_periodic_task, NULL, 500000); // 2Hz

    eth.netdev.packet_alloc = ethernet_packet_alloc;
    eth.netdev.packet_send = ethernet_packet_send;

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


/*    phy_write(0, (1<<15));
    while (phy_read(0)&(1<<15))
           nkern_usleep(1000);
*/

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

    nvic_set_handler(58, ethernet_irq);

    MAC_IM_R = 0;

    // enable interrupts
    NVIC_EN1_R |= (1<<10);

    return (net_dev_t*) &eth;
}
