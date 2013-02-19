#include <stdio.h>
#include <nkern.h>
#include <arch.h>
#include <lpc23xx.h>
#include <string.h>

#include "can.h"

/* Acceptance filter mode in AFMR register */
#define ACCF_OFF                                0x01
#define ACCF_BYPASS                             0x02
#define ACCF_ON                                 0x00
#define ACCF_FULLCAN                    0x04

/* This number applies to all FULLCAN IDs, explicit STD IDs, group STD IDs, 
   explicit EXT IDs, and group EXT IDs. */ 
#define ACCF_IDEN_NUM                   4

/* Identifiers for FULLCAN, EXP STD, GRP STD, EXP EXT, GRP EXT */
#define FULLCAN_ID                              0x100
#define EXP_STD_ID                              0x100
#define GRP_STD_ID                              0x200
#define EXP_EXT_ID                              0x100000
#define GRP_EXT_ID                              0x200000

#define CAN_CHANNELS        2

// must be a power of two
#define RX_MESSAGE_BUFFERS 32

static nkern_wait_list_t  rx_waitlist[CAN_CHANNELS];  // wait list for specific channels
static can_message_t      rx_buffer[CAN_CHANNELS][RX_MESSAGE_BUFFERS];
static volatile uint32_t  rx_buffer_write_pos[CAN_CHANNELS], rx_buffer_read_pos[CAN_CHANNELS];
static uint32_t           rx_overflow[CAN_CHANNELS];

static inline uint32_t min(uint32_t a, uint32_t b)
{
    return a < b ? a : b;
}

static inline void can_enqueue_message(int can_port, uint32_t frame, uint32_t id, uint32_t data0, uint32_t data1)
{
    int write_pos = rx_buffer_write_pos[can_port], read_pos = rx_buffer_read_pos[can_port];

    // if full, it's an overflow. Discard the data.
    if (((write_pos+1)&(RX_MESSAGE_BUFFERS-1)) == read_pos) {
        rx_overflow[can_port]++;
        return;
    }

    rx_buffer[can_port][write_pos].timestamp = nkern_utime();
    rx_buffer[can_port][write_pos].id = id;
    rx_buffer[can_port][write_pos].len = min((frame >> 16) & 0x0f, 8);
    rx_buffer[can_port][write_pos].data[0] = data0;
    rx_buffer[can_port][write_pos].data[1] = data1;

    rx_buffer_write_pos[can_port] = (write_pos + 1) & (RX_MESSAGE_BUFFERS-1);
}

static void can_irq_real(void) __attribute__ ((noinline));
static void can_irq_real()
{
    int reschedule = 0;
    uint32_t can_status  = CAN_RX_SR;

    if ((can_status & (1<<8))) {

        if (can_status & (1<<16)) {
            rx_overflow[0]++;
            CAN1CMR = 1<<3;  // clear error condition
        }

        can_enqueue_message(0, CAN1RFS, CAN1RID, CAN1RDA, CAN1RDB);
        CAN1CMR = 0x04;      // release read buffer

        reschedule |= _nkern_wake_all(&rx_waitlist[0]);
//        CAN1IER = 0;
    }

    if ((can_status & (1<<9))) {

        if (can_status & (1<<17)) {
            rx_overflow[1]++;
            CAN2CMR = 1<<3;  // clear error condition
        }

        can_enqueue_message(1, CAN2RFS, CAN2RID, CAN2RDA, CAN2RDB);
        CAN2CMR = 0x04;      // release read buffer

        reschedule |= _nkern_wake_all(&rx_waitlist[1]);
//        CAN2IER = 0;
    }

    if (CAN1GSR & (1<<6)) {
//        uint32_t errcnt = CAN1GSR >> 16;
        // error cnt
    }

    if (CAN2GSR & (1<<6)) {
//        uint32_t errcnt = CAN2GSR >> 16;
    }

    VICVectAddr = 0;

    if (reschedule)
        _nkern_schedule();
}

void can_irq(void) __attribute__ ((naked));
void can_irq(void)
{
    IRQ_TASK_SAVE;
    NKERN_IRQ_ENTER;
    can_irq_real();
    NKERN_IRQ_EXIT;
    IRQ_TASK_RESTORE;
}

void can_init(uint32_t btr)
{
    PCONP |= (1 << 13) | (1 << 14); // power it up
    PINSEL0  &= ~0x00000f0f; // enable both can channels
    PINSEL0  |= 0x00000a05;

    CAN1MOD = CAN2MOD = 1;   // Reset CAN
    CAN1IER = CAN2IER = 0;   // Disable Receive Interrupt
    CAN1GSR = CAN2GSR = 0;   // Reset error counter when CANxMOD is in reset
     
    CAN1BTR = CAN2BTR = btr;
    CAN1MOD = CAN2MOD = 0x0; // CAN in normal operation mode
     
    VICVectAddr23 = (uint32_t) can_irq;
    VICIntEnable |= (1<<23);
    VICVectCntl23 = 2;

    // RX interrupts are initially off until we have a read pending.
    CAN1IER = CAN2IER = 0; 

    CAN_AFMR = ACCF_BYPASS;   // no acceptance filter

    for (int i = 0; i < CAN_CHANNELS; i++) 
        nkern_wait_list_init(&rx_waitlist[i], "can_rx");

    CAN1IER = 1;
    CAN2IER = 1;
}

int can_0_send_message(can_message_t *m)
{
    int32_t id = m->id;
    int32_t frame = m->len << 16;

    uint32_t can_status = CAN1SR;

    if (can_status & 0x00000004) {
        CAN1TFI1 = frame;
        CAN1TID1 = id;
        CAN1TDA1 = m->data[0];
        CAN1TDB1 = m->data[1];
        CAN1CMR  = 0x21;

        return 0;
    }

    if (can_status & 0x00000400) {
        CAN1TFI2 = frame;
        CAN1TID2 = id;
        CAN1TDA2 = m->data[0];
        CAN1TDB2 = m->data[1];
        CAN1CMR  = 0x41;

        return 0;
    }

    if (can_status & 0x00040000) {
        CAN1TFI3 = frame;
        CAN1TID3 = id;
        CAN1TDA3 = m->data[0];
        CAN1TDB3 = m->data[1];
        CAN1CMR  = 0x81;

        return 0;
    }

    return -1;
}

int can_1_send_message(can_message_t *m)
{
    int32_t id = m->id;
    int32_t frame = m->len << 16;

    uint32_t can_status = CAN2SR;

    if (can_status & 0x00000004) {
        CAN2TFI1 = frame;
        CAN2TID1 = id;
        CAN2TDA1 = m->data[0];
        CAN2TDB1 = m->data[1];
        CAN2CMR  = 0x21;

        return 0;
    }

    if (can_status & 0x00000400) {
        CAN2TFI2 = frame;
        CAN2TID2 = id;
        CAN2TDA2 = m->data[0];
        CAN2TDB2 = m->data[1];
        CAN2CMR  = 0x41;

        return 0;
    }

    if (can_status & 0x00040000) {
        CAN2TFI3 = frame;
        CAN2TID3 = id;
        CAN2TDA3 = m->data[0];
        CAN2TDB3 = m->data[1];
        CAN2CMR  = 0x81;

        return 0;
    }

    return -1;
}

int can_write(int port, can_message_t *m)
{
    if (port == 0)
        return can_0_send_message(m);
    if (port == 1)
        return can_1_send_message(m);
    return -1;
}

/** port [0,1] **/
int can_read(int can_port, can_message_t *m)
{
    irqstate_t flags;

    disable_interrupts(&flags);

wait:
    // wait for there to be data.
    
    if (rx_buffer_write_pos[can_port] == rx_buffer_read_pos[can_port]) {
        nkern_wait(&rx_waitlist[can_port]);
        goto wait;
    }
    
    // we have data.
    
    int read_pos = rx_buffer_read_pos[can_port];
    memcpy(m, &rx_buffer[can_port][read_pos], sizeof(can_message_t));
    
    rx_buffer_read_pos[can_port] = (read_pos + 1) & (RX_MESSAGE_BUFFERS-1);

    enable_interrupts(&flags);

    return 0;
}

int can_overflow_count(int can_port)
{
    irqstate_t flags;

    disable_interrupts(&flags);

    uint32_t count = rx_overflow[can_port];

    enable_interrupts(&flags);
    
    return count;
}
