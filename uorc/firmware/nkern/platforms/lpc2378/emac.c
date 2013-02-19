#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <lpc23xx.h>
#include <nkern.h>
#include <arch.h>
#include <net.h>

#include "emac.h"
#include "emac_impl.h"

typedef struct 
{
    uint32_t addr;
    uint32_t control;
} rx_descriptor_t;

typedef struct
{
    uint32_t status;
    uint32_t hashcrc;
} rx_status_t;

typedef struct
{
    uint32_t addr;
    uint32_t control;
} tx_descriptor_t;

typedef struct
{
    uint32_t status;
} tx_status_t;

static void delay()
{
    for (int i = 0; i < 500; i++)
        nop();
}

static int phy_speed;
static int phy_full_duplex;

static void emac_link_task(void *user);
static void fragment_rx_task(void *user);

static void chunk_free(net_chunk_t *c);
static net_chunk_t *chunk_get(int block);

static void packet_free(net_packet_t *p);
static void packet_and_chunk_free(net_packet_t *p);
static net_packet_t *packet_get();
static net_packet_t *packet_alloc(net_dev_t *netdev, int needed_size, int *offset);

static int packet_send(net_dev_t *netdev, net_packet_t *p, uint32_t ip_dest, int type);

// CHUNK_SIZE must be large enough to hold all applicable transport
// headers, i.e., ethernet + IP + TCP (62 bytes, not including options)
#define CHUNK_SIZE          1600

// LPC23xx has a bug if RX_DESCRIPTORS > 4
#define RX_DESCRIPTORS      4
#define TX_DESCRIPTORS      16

#define NUM_PACKETS         16
#define NUM_CHUNKS          ((16384 - RX_DESCRIPTORS*(sizeof(rx_descriptor_t)+sizeof(rx_status_t)) - \
                              TX_DESCRIPTORS*(sizeof(tx_descriptor_t)+sizeof(tx_status_t))) / CHUNK_SIZE)

#define ETHERNET_HEADER_SIZE 14

#define min(a,b) ( (a < b) ? (a) : (b))

/*
  #if (NUM_CHUNKS > RX_DESCRIPTORS)
  #error Not enough chunks
  #endif
*/

nkern_wait_list_t  fragment_rx_waitlist;

// statically allocate all of our chunks and packets
static net_chunk_t   _chunk[NUM_CHUNKS];
static net_chunk_t  * volatile free_chunks_head;

static net_packet_t  _packets[NUM_PACKETS];
static net_packet_t * volatile free_packets_head;

// a look-up table allowing us to find a chunk corresponding to a
// particular RX descriptor
static net_chunk_t *rx_chunks[RX_DESCRIPTORS]; 
static net_chunk_t *tx_chunks[TX_DESCRIPTORS]; 

static nkern_semaphore_t  chunk_semaphore;
static nkern_semaphore_t  packet_semaphore;
static nkern_semaphore_t  send_semaphore;
static nkern_mutex_t      send_mutex;

static eth_dev_t  ethdev;

////////////////////////////////////////////////////////////////////////

// we're putting these variables in a custom section so the memory can
// be DMA'd; they will not be zeroed!

static rx_descriptor_t  __attribute__ ((section ("enet")))  rx_descriptors[RX_DESCRIPTORS];
static rx_status_t      __attribute__ ((section ("enet")))  __attribute__ ((aligned(8))) rx_statuses[RX_DESCRIPTORS];

static tx_descriptor_t  __attribute__ ((section ("enet")))  tx_descriptors[TX_DESCRIPTORS];
static tx_status_t      __attribute__ ((section ("enet")))  tx_statuses[TX_DESCRIPTORS];
static uint32_t         __attribute__ ((section ("enet")))  chunk_buffer[NUM_CHUNKS][CHUNK_SIZE/4];

////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////

uint32_t tx_consumed_idx;

static void emac_irq_real(void) __attribute__ ((noinline));
static void emac_irq_real()
{
    uint32_t int_status = MAC_INTSTATUS;
    int reschedule = 0;

    MAC_INTCLEAR = int_status;

    if (int_status & INT_RX_OVERRUN) {
//        MAC_COMMAND |= (1<<5);  // soft RX reset
        kprintf("RX overrun\n");
    }

    if (int_status & INT_RX_ERR) {
//        kprintf("RX error\n");
        // almost always an RX error, because the ethernet type field is
        // mis-interpreted as a length field.
    }

    if (int_status & INT_RX_FIN) {
        // no outstanding rx descriptors (one-shot)
        // not useful to us; we use RXDONE
    }

    if (int_status & INT_RX_DONE) {
        reschedule |= _nkern_wake_all(&fragment_rx_waitlist);
    }

    if (int_status & INT_TX_UNDERRUN) {
        kprintf("TX underrun (really bad hard reset tx!)\n");
        MAC_COMMAND |= (1<<4);  // soft TX reset
    }

    if (int_status & INT_TX_ERR) {
        kprintf("TX error %08x %08x\n", (int) int_status, (int) tx_statuses[MAC_TXCONSUMEINDEX].status);
    }

    if (int_status & INT_TX_FIN) {
        // all tx descriptors have been sent.
        // not useful to us; we use TXDONE
    }

    if (int_status & INT_TX_DONE) {
        // a tx descriptor was just sent
        
        while (tx_consumed_idx != MAC_TXCONSUMEINDEX) {

//           kprintf("TX status: %08x\n", (int) tx_statuses[tx_consumed_idx].status);
           
            chunk_free(tx_chunks[tx_consumed_idx]);
            tx_chunks[tx_consumed_idx] = NULL;
            tx_consumed_idx++;
            if (tx_consumed_idx == TX_DESCRIPTORS)
                tx_consumed_idx = 0;
           
            reschedule |= nkern_semaphore_up(&send_semaphore);
        }
    }

    if (int_status & INT_WAKEUP) {
        // wake on lan
    }

    if (reschedule)
        _nkern_schedule();

    VICVectAddr = 0;
}

static void emac_irq(void) __attribute__ ((naked));
static void emac_irq(void)
{
    IRQ_TASK_SAVE;
    NKERN_IRQ_ENTER;
    emac_irq_real();
    NKERN_IRQ_EXIT;
    IRQ_TASK_RESTORE;
}

////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////
static void write_phy( uint32_t reg, uint32_t data )
{
    MAC_MADR = DP83848C_DEF_ADR | reg;  /* [12:8] == PHY addr, [4:0]=0x00(BMCR) register addr */
    MAC_MWTD = data;
    // no MAC_MCMD?

    for (int tout = 0; tout < MII_WR_TOUT; tout++) {
        if ((MAC_MIND & MIND_BUSY) == 0) {
            break;
        }
    }
}

static uint32_t read_phy( uint32_t reg )
{
    MAC_MADR = DP83848C_DEF_ADR | reg;  /* [12:8] == PHY addr, [4:0]=0x00(BMCR) register addr */ 
    MAC_MCMD = MCMD_READ;

    for (int tout = 0; tout < MII_RD_TOUT; tout++) {
        if ((MAC_MIND & MIND_BUSY) == 0) {
            break;
        }
    }
 
    MAC_MCMD = 0;
    return MAC_MRDD;
}

eth_dev_t *emac_getdev()
{
    return &ethdev;
}

////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////
int emac_init(uint32_t mac_offset)
{
    nkern_wait_list_init(&fragment_rx_waitlist, "emac_rx");

    // locally administered addresses
    ethdev.eth_addr             = 0x020ed3456789ULL + mac_offset;

    ethdev.netdev.packet_alloc  = packet_alloc;
    ethdev.netdev.packet_send   = packet_send;

    PCONP |= (1<<30);  // power up emac

    // RM-II interface
    PINSEL2 &= ~(0xf03f030f);
    PINSEL2 |=   0x50151105;    // selects P1[0,1,4,8,9,10,14,15] 
    //                 ^  this extra bit must be set or the CPU resets

    PINSEL3 &= ~(0x0000000f);
    PINSEL3 |=   0x00000005;    // selects P1[17:16] 

    /* Reset all EMAC internal modules. */
    MAC_MCFG = 0x0018; // host clock/20
    
    MAC_MAC1 = MAC1_RES_TX | MAC1_RES_MCS_TX | MAC1_RES_RX | MAC1_RES_MCS_RX |
        MAC1_SIM_RES | MAC1_SOFT_RES;
    MAC_COMMAND = CR_REG_RES | CR_TX_RES | CR_RX_RES;
    
    delay();

    /* Initialize MAC control registers. */
    MAC_MAC1 = MAC1_PASS_ALL;
    MAC_MAC2 = MAC2_CRC_EN | MAC2_PAD_EN;
    MAC_MAXF = ETH_MAX_FLEN;
    MAC_CLRT = CLRT_DEF;
    MAC_IPGR = IPGR_DEF;
    
    /* Enable Reduced MII interface. */
    MAC_COMMAND = CR_RMII | CR_PASS_RUNT_FRM;
    
    /* Reset Reduced MII Logic. */
    MAC_SUPP = SUPP_RES_RMII;
    delay();
    MAC_SUPP = 0;
    
    /* Put the DP83848C in reset mode */
    write_phy (PHY_REG_BMCR, 0x8000);
    
    /* Wait for hardware reset to end. */
    for (int tout = 0; tout < 0x100000; tout++) {
        uint32_t regv = read_phy (PHY_REG_BMCR);
        if (!(regv & 0x8000)) {
            /* Reset complete */
            break;
        }
        nkern_yield();
    }
    
    /* Check if this is a DP83848C PHY. */
    int phy_id = (read_phy (PHY_REG_IDR1) << 16) |
        (read_phy(PHY_REG_IDR2) & 0xfff0);

    if (phy_id == DP83848C_ID) {
        /* Configure the PHY device */
        
        /* Use autonegotiation about the link speed. */
        //              write_phy (PHY_REG_BMCR, PHY_AUTO_NEG);

        // disable autonegotiation, forcing 10Mbit
//        write_phy (PHY_REG_BMCR, 0); // 10Mbit, half duplex
//        write_phy(PHY_REG_BMCR, 1<<8); // 10Mbit, full duplex
        // write ANAR

        // ANAR
        // 15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
        // 0  0  0  0  0  0  0  0  0  1  1  0  0  0  0  1
        write_phy(PHY_REG_ANAR, 0x61); // 10Mbit half/full duplex
        // possibly re-start autonegotiation

        kprintf("DP83848C ethernet phy\n");

    }

    // beware of the absolutely bizarre byte ordering in the SAx
    // registers!
    MAC_SA0 = ntohs((ethdev.eth_addr >> 0) & 0xffff);
    MAC_SA1 = ntohs((ethdev.eth_addr >> 16) & 0xffff);
    MAC_SA2 = ntohs((ethdev.eth_addr >> 32) & 0xffff);

    free_chunks_head = NULL;

    // initialize all the chunks and add them to the free list.
    for (uint32_t i = 0; i < NUM_CHUNKS; i++) {
        _chunk[i].buffer = (uint8_t*) chunk_buffer[i];
        _chunk[i].maxsize = CHUNK_SIZE;
        _chunk[i].size = 0;

        // don't use net_packet_free, because this function also
        // tries to resume reading
        _chunk[i].next_chunk = free_chunks_head;
        free_chunks_head = &_chunk[i];
    }

    free_packets_head = NULL;
    for (uint32_t i = 0; i < NUM_PACKETS; i++) {
        _packets[i].nchunks = 0;
        _packets[i].size = 0;
        _packets[i].first_chunk = NULL;
        _packets[i].next_packet = NULL;
        _packets[i].free = packet_and_chunk_free;
        _packets[i].next_packet = free_packets_head;
        free_packets_head = &_packets[i];
    }

    nkern_semaphore_init(&chunk_semaphore, "chunk_sem", NUM_CHUNKS);
    nkern_semaphore_init(&packet_semaphore, "packet_sem", NUM_PACKETS);
    nkern_semaphore_init(&send_semaphore, "send_sem", NUM_PACKETS);
    nkern_mutex_init(&send_mutex, "send_mtx");

    // init descriptors
    for (int i = 0; i < TX_DESCRIPTORS; i++) {
        tx_descriptors[i].addr = 0;
        tx_descriptors[i].control = 0;
        tx_statuses[i].status = 0;
    }

    for (int i = 0; i < RX_DESCRIPTORS; i++) {
        net_chunk_t *c = chunk_get(1); // must not fail
        rx_chunks[i] = c;
        rx_descriptors[i].addr = (uint32_t) c->buffer;
        rx_descriptors[i].control = (1<<31) | (c->maxsize - 1); // request irq
        rx_statuses[i].status = 0;
        rx_statuses[i].hashcrc = 0;
    }

    MAC_TXDESCRIPTORNUM = TX_DESCRIPTORS - 1;
    MAC_TXDESCRIPTOR    = (uint32_t) tx_descriptors;
    MAC_TXSTATUS        = (uint32_t) tx_statuses;
    MAC_TXPRODUCEINDEX  = MAC_TXCONSUMEINDEX; // nothing to send
    tx_consumed_idx     = MAC_TXCONSUMEINDEX;

    MAC_RXDESCRIPTORNUM = RX_DESCRIPTORS - 1;
    MAC_RXDESCRIPTOR    = (uint32_t) rx_descriptors;
    MAC_RXSTATUS        = (uint32_t) rx_statuses;
    MAC_RXCONSUMEINDEX  = 0; 

    nkern_task_create("emac_link", emac_link_task, NULL, 1, 512);
    nkern_task_create("enet_frag_rx", fragment_rx_task, NULL, NKERN_PRIORITY_NORMAL, 1024);

    return 0;
}

// finish initialization after link
static void emac_finish_initialization()
{
    // Link established from here on 
    uint32_t phy_stat = read_phy(PHY_REG_STS);

    phy_full_duplex = (phy_stat & 0x04) ? 1 : 0;
    phy_speed = (phy_stat & 0x02) ? 10 : 100;

    if (phy_full_duplex) {
        MAC_MAC2 |= MAC2_FULL_DUP;
        MAC_COMMAND |= CR_FULL_DUP;
        MAC_IPGT = IPGT_FULL_DUP;
    } else {
        MAC_IPGT = IPGT_HALF_DUP;
    }
    
    /*    if (phy_speed == 10) {
          MAC_SUPP = 0;
          } else if (phy_speed == 100) {
          MAC_SUPP = SUPP_SPEED;
          } else {
          kprintf("err: unknown mac negotiation\n");
          }
    */
    MAC_SUPP = 0;

    /* Receive Broadcast and Perfect Match Packets */
    MAC_RXFILTERCTRL =  RFC_BCAST_EN | RFC_PERFECT_EN; // | RFC_UCAST_EN ;
    MAC_HASHFILTERL = 0;
    MAC_HASHFILTERH = 0;

    /* Enable EMAC interrupts. */
    MAC_INTENABLE = INT_RX_DONE | INT_TX_DONE;
    
    /* Reset all interrupts */
    MAC_INTCLEAR  = 0xFFFF;
    
    /* Enable receive and transmit mode of MAC Ethernet core */
    MAC_COMMAND  |= (CR_RX_EN | CR_TX_EN); // | CR_PASS_RX_FILT;
    MAC_MAC1     |= MAC1_REC_EN;

    VICVectAddr21 = (uint32_t) emac_irq;
    VICIntEnable |= (1<<21);
    VICVectCntl21  = 14;     // we have fairly large amounts of buffer space; fairly low priority

    MAC_INTENABLE = 0x00FF; // Enable all interrupts except SOFTINT and WOL 
}

static void emac_link_task(void *user)
{
    (void) user;

    int link = 0;

    while (1) {
        nkern_usleep(250000);

        uint32_t regv = read_phy (PHY_REG_STS);
        int  newlink = regv & (1 << 0);
        
        if (newlink && !link) {
            // just connected
            emac_finish_initialization();
        }
        if (!newlink && link) {
            // just disconnected
        }

        link = newlink;
    }
}

/** Task which services data (in ethernet fragment form) and calls
 * successive processing layers **/
static void fragment_rx_task(void *user)
{
    (void) user;

    net_chunk_t *tail_chunk = NULL;
    net_packet_t *packet = NULL;

    while (1) {
        while (MAC_RXCONSUMEINDEX != MAC_RXPRODUCEINDEX) {

            int idx = MAC_RXCONSUMEINDEX;

            // assemble a whole net_packet_t from a chain of net_chunks
            uint32_t status = rx_statuses[idx].status;

            // possible work-around for wrap-around bug in lpc2368/2378

            //	    kprintf("RX IDX %4d %4d", (int) MAC_RXCONSUMEINDEX, (int) MAC_RXPRODUCEINDEX);
            /*
              workaround:

              if (status == 0) {
              kprintf("(skip %i) ", idx);
              idx = (idx+1 == RX_DESCRIPTORS) ? 0 : idx+1;
              goto workaround;
              }
              kprintf("\n");
            */

            net_chunk_t *chunk = rx_chunks[idx];

            //     kprintf("%4i %4i %08x\n",idx, (int) MAC_RXPRODUCEINDEX, (int) status);

            chunk->size = (status & 0x7ff) + 1;

            if (tail_chunk == NULL) {
                packet = packet_get();
                tail_chunk = chunk;
                packet->first_chunk = chunk;
            } else {
                tail_chunk->next_chunk = chunk;
                tail_chunk = chunk;
            }

            packet->size += chunk->size;
            packet->nchunks ++;
            chunk->next_chunk = NULL;

            if (status & ((1<<20) | (1<<23) | (1<<24) | (1<<25))) {

                // did fragment fail the filter or have a bad CRC, or bad
                // symbol, or bad length?
                kprintf("emac rx err %08x\n", (int) status);

                packet_and_chunk_free(packet);
                tail_chunk = NULL;
                packet = NULL;

            } else if (status & (1<<30)) {

                // if last fragment of a whole ethernet frame, send this
                // packet up to the next layer. they must free it.
                net_ethernet_receive(packet, 0);
                    
                tail_chunk = NULL;
                packet = NULL;
            }

            net_chunk_t *newchunk = chunk_get(1); // block until a new chunk is available
            rx_chunks[idx] = newchunk;
            rx_descriptors[idx].addr = (uint32_t) newchunk->buffer;
            rx_descriptors[idx].control = (1<<31) | (newchunk->maxsize - 1); // request irq
            rx_statuses[idx].status = 0;

            MAC_RXCONSUMEINDEX = (MAC_RXCONSUMEINDEX+1) % RX_DESCRIPTORS;
        }
        
        // if no data, sleep.
        irqstate_t flags;
        disable_interrupts(&flags);
        if (MAC_RXCONSUMEINDEX == MAC_RXPRODUCEINDEX)
            nkern_wait(&fragment_rx_waitlist);
        enable_interrupts(&flags);
    }
}

static void chunk_free(net_chunk_t *c)
{
    assert (c != NULL);

    irqstate_t flags;
    disable_interrupts(&flags);

    assert (c->size != 0); // protect against double free
    c->size = 0;

    c->next_chunk = free_chunks_head; 
    free_chunks_head = c;

    enable_interrupts(&flags);

    // this debugging measure makes failures easier to detect
    //    memset(c->buffer, 0x5a, c->maxsize);
    // uncomment either the line above or the line below
    //    *((uint32_t*) c->buffer) = 0x5a5a5a5a;

    nkern_semaphore_up(&chunk_semaphore);
}

static net_chunk_t *chunk_get(int block)
{
    if (block)
        nkern_semaphore_down(&chunk_semaphore);
    else if (nkern_semaphore_down_try(&chunk_semaphore)) {
        kprintf("out of chunks\n");
        return NULL;
    }

    irqstate_t flags;
    disable_interrupts(&flags);
    
    net_chunk_t *c = free_chunks_head;
    
    assert(c != NULL);

    free_chunks_head = free_chunks_head->next_chunk;

    enable_interrupts(&flags);

    c->next_chunk = NULL;
    c->size = 0;

    // this debugging measure makes failures easier to detect
    //    memset(c->buffer, 0x5a, c->maxsize);

    return c;
}

static void packet_free(net_packet_t *p)
{
    assert (p != NULL);

    assert (p->size != 0); // protect against double free
    p->size = 0;

    irqstate_t flags;
    disable_interrupts(&flags);

    p->next_packet = free_packets_head;
    free_packets_head = p;

    enable_interrupts(&flags);

    nkern_semaphore_up(&packet_semaphore);
}

static void packet_and_chunk_free(net_packet_t *p)
{
    assert(p->next_packet == NULL);
  
    net_chunk_t *c = p->first_chunk;

    // be careful: net_chunk_free will modify c->next_chunk.

    // if we maintained the chunk tail ptr, we could release all the
    // chunks in O(1)

    while (c != NULL) {
        net_chunk_t *nc = c->next_chunk;
        chunk_free(c);
        c = nc;
    }

    packet_free(p);
}

static net_packet_t *packet_get()
{
    if (nkern_semaphore_down_try(&packet_semaphore))
        return NULL;

    irqstate_t flags;
    disable_interrupts(&flags);
    net_packet_t *p = free_packets_head;
    free_packets_head = free_packets_head->next_packet;
    enable_interrupts(&flags);

    p->first_chunk = NULL;
    p->next_packet = NULL;
    p->size = 0;
    p->nchunks = 0;
    p->netdev = &ethdev.netdev;
    return p;
}

static net_packet_t *packet_alloc(net_dev_t *netdev, int needed_size, int *offset)
{
    (void) netdev;

    net_packet_t *p = packet_get();
    if (p == NULL)
        return NULL;

    p->size = needed_size + ETHERNET_HEADER_SIZE;

    net_chunk_t **tailptr = &p->first_chunk;
    int need_alloc = p->size;

    while (need_alloc) {
        net_chunk_t *chunk = chunk_get(0); // don't block: it's okay to fail
        if (!chunk)
            goto abort;

        chunk->size = min(chunk->maxsize, need_alloc);
        need_alloc -= chunk->size;
        
        *tailptr = chunk;
        tailptr = &chunk->next_chunk;
        p->nchunks++;
    }
    *tailptr = NULL;
    *offset = ETHERNET_HEADER_SIZE;

    return p;

abort:
    p->free(p);
    return NULL;
}

static int packet_send(net_dev_t *netdev, net_packet_t *p, uint32_t ip_dest, int type)
{
    eth_addr_t eth_addr_dest;
    eth_dev_t *eth = (eth_dev_t*) netdev;

    if (net_arp_query(eth, ip_dest, &eth_addr_dest)) {
        p->free(p);
        return -1;
    }

    // packet too big?
    if (p->nchunks >= TX_DESCRIPTORS) {
        p->free(p);
        return -1;
    }

    nkern_mutex_lock(&send_mutex);

    net_encode_eth_addr(&p->first_chunk->buffer[0], eth_addr_dest);
    net_encode_eth_addr(&p->first_chunk->buffer[6], eth->eth_addr);
    net_encode_u16(&p->first_chunk->buffer[12], type);

    // as we queue these for the MAC, they can be transmitted and the
    // chunks freed. this will alter the next_chunk links, so we must
    // sample first.
    net_chunk_t *c = p->first_chunk;

    int idx = MAC_TXPRODUCEINDEX;

    while (c != NULL) {
        //        kprintf("send chunk %08x size %4d idx %4d\n", (int) c->buffer, c->size, (int) MAC_TXPRODUCEINDEX);
        net_chunk_t *nextc = c->next_chunk;

        nkern_semaphore_down(&send_semaphore); // wait for room to transmit

        assert (tx_chunks[idx] == NULL);
        tx_chunks[idx] = c;
        tx_descriptors[idx].addr = (uint32_t) c->buffer;

        // override, pad, crc, interrupt
        tx_descriptors[idx].control = (c->size-1) | (1<<26) | (1<<28) | (1<<29) | (1<<31); 
        if (c->next_chunk == NULL)
            tx_descriptors[idx].control |= (1<<30); // last
        
        c = nextc;
        idx = (idx + 1 == TX_DESCRIPTORS) ? 0 : idx+1;
    }

    MAC_TXPRODUCEINDEX = idx;

    // free only the packet container. The chunks will be freed by the TX isr
    packet_free(p);

    nkern_mutex_unlock(&send_mutex);

    return 0;
}
