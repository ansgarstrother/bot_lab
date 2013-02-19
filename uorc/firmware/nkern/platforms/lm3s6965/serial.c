#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <nkern.h>
#include <lm3s6965.h>
#include "serial.h"

/** NOTE:
    Luminary UART is stupid, you must choose between:

    A) a nice 16byte FIFO, but high latency between characters (32 bit periods)
    B) a 1 byte FIFO with low latency (and a risk of losing characters)

    We default to option A. See code below for how to go with option B.
**/
#define NUM_PORTS 2

static nkern_wait_list_t rx_waitlist[NUM_PORTS];
static nkern_wait_list_t tx_waitlist[NUM_PORTS];
static nkern_mutex_t tx_mutex[NUM_PORTS];
static nkern_mutex_t rx_mutex[NUM_PORTS];

#define PR(x,port) (*(volatile unsigned long*)(((uint32_t) x) + port<<12))

#define UART_CONFIG_WLEN_8      0x00000060  // 8 bit data
#define UART_CONFIG_WLEN_7      0x00000040  // 7 bit data
#define UART_CONFIG_WLEN_6      0x00000020  // 6 bit data
#define UART_CONFIG_WLEN_5      0x00000000  // 5 bit data
#define UART_CONFIG_STOP_MASK   0x00000008  // Mask for extracting stop bits
#define UART_CONFIG_STOP_ONE    0x00000000  // One stop bit
#define UART_CONFIG_STOP_TWO    0x00000008  // Two stop bits
#define UART_CONFIG_PAR_MASK    0x00000086  // Mask for extracting parity
#define UART_CONFIG_PAR_NONE    0x00000000  // No parity
#define UART_CONFIG_PAR_EVEN    0x00000006  // Even parity
#define UART_CONFIG_PAR_ODD     0x00000002  // Odd parity
#define UART_CONFIG_PAR_ONE     0x00000086  // Parity bit is one
#define UART_CONFIG_PAR_ZERO    0x00000082  // Parity bit is zero

#define TX_IRQ 5
#define RX_IRQ 4
#define RX_TIMEOUT_IRQ 6

// UART0_FR: RX fifo is empty
#define RXFE 4

static iop_t serial0_iop;
static iop_t serial1_iop;

/////////////////////////////////////////////////////////
// Serial0
static void serial0_irq_real(void) __attribute__ ((noinline));
static void serial0_irq_real()
{
    uint32_t status = UART0_FR_R;
    uint32_t reschedule = 0;

    if (!(status & (1<<RXFE))) {
        // data is available.
        UART0_ICR_R = (1<<RX_IRQ) | (1<<RX_TIMEOUT_IRQ);
        reschedule |= _nkern_wake_all(&rx_waitlist[0]);
        UART0_IM_R &= ~((1<<RX_IRQ) | (1<<RX_TIMEOUT_IRQ));
    }

    if (!(status & (1<<TX_IRQ))) {
        // room to send
        UART0_ICR_R = 1<<TX_IRQ;
        reschedule |= _nkern_wake_all(&tx_waitlist[0]);
        UART0_IM_R &= ~(1<<TX_IRQ);
    }

    if (reschedule)
        _nkern_schedule();
}

static void serial0_irq(void) __attribute__ ((naked));
static void serial0_irq(void)
{
    IRQ_TASK_SAVE;
    NKERN_IRQ_ENTER;
    serial0_irq_real();
    NKERN_IRQ_EXIT;
    IRQ_TASK_RESTORE;
}

void serial0_putc(char c)
{
    irqstate_t flags;

    nkern_mutex_lock(&tx_mutex[0]);

    interrupts_disable(&flags);

    // wait until fifo is not full
    while (UART0_FR_R & UART_FR_TXFF) {
        UART0_IM_R |= (1<<TX_IRQ);
        nkern_wait(&tx_waitlist[0]);
    }

    // enqueue.
    UART0_DR_R = c;

    interrupts_restore(&flags);
    nkern_mutex_unlock(&tx_mutex[0]);
}

char serial0_getc()
{
    irqstate_t flags;

    nkern_mutex_lock(&rx_mutex[0]);

    interrupts_disable(&flags);

    while (UART0_FR_R & UART_FR_RXFE) {
        UART0_IM_R |= (1<<RX_IRQ) | (1<<RX_TIMEOUT_IRQ);
        nkern_wait(&rx_waitlist[0]);
    }

    char c = UART0_DR_R;

    interrupts_restore(&flags);

    nkern_mutex_unlock(&rx_mutex[0]);
    return c;
}

void serial0_putc_spin(char c)
{
    while (UART0_FR_R & UART_FR_TXFF);
    UART0_DR_R = c;
}

int serial0write(iop_t *iop, const void *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
        serial0_putc(((char*) buf)[i]);
    return len;
}

int serial0read(iop_t *iop, void *buf, uint32_t len)
{
    ((char*) buf)[0] = serial0_getc();
    return 1;
}

/////////////////////////////////////////////////////////
// Serial1
static void serial1_irq_real(void) __attribute__ ((noinline));
static void serial1_irq_real()
{
    uint32_t status = UART1_FR_R;
    uint32_t reschedule = 0;

    if (!(status & (1<<RXFE))) {
        // data is available.
        UART1_ICR_R = (1<<RX_IRQ) | (1<<RX_TIMEOUT_IRQ);
        reschedule |= _nkern_wake_all(&rx_waitlist[1]);
        UART1_IM_R &= ~((1<<RX_IRQ) | (1<<RX_TIMEOUT_IRQ));
    }

    if (!(status & (1<<TX_IRQ))) {
        // room to send
        UART1_ICR_R = 1<<TX_IRQ;
        reschedule |= _nkern_wake_all(&tx_waitlist[1]);
        UART1_IM_R &= ~(1<<TX_IRQ);
    }

    if (reschedule)
        _nkern_schedule();
}

static void serial1_irq(void) __attribute__ ((naked));
static void serial1_irq(void)
{
    IRQ_TASK_SAVE;
    NKERN_IRQ_ENTER;
    serial1_irq_real();
    NKERN_IRQ_EXIT;
    IRQ_TASK_RESTORE;
}

void serial1_putc(char c)
{
    irqstate_t flags;

    nkern_mutex_lock(&tx_mutex[1]);

    interrupts_disable(&flags);

    // wait until fifo is not full
    while (UART1_FR_R & UART_FR_TXFF) {
        UART1_IM_R |= (1<<TX_IRQ);
        nkern_wait(&tx_waitlist[1]);
    }

    // enqueue.
    UART1_DR_R = c;

    interrupts_restore(&flags);
    nkern_mutex_unlock(&tx_mutex[1]);
}

char serial1_getc()
{
    irqstate_t flags;

    nkern_mutex_lock(&rx_mutex[1]);

    interrupts_disable(&flags);

    while (UART1_FR_R & UART_FR_RXFE) {
        UART1_IM_R |= (1<<RX_IRQ) | (1<<RX_TIMEOUT_IRQ);
        nkern_wait(&rx_waitlist[1]);
    }

    char c = UART1_DR_R;

    interrupts_restore(&flags);

    nkern_mutex_unlock(&rx_mutex[1]);
    return c;
}

void serial1_putc_spin(char c)
{
    while (UART1_FR_R & UART_FR_TXFF);
    UART1_DR_R = c;
}

int serial1write(iop_t *iop, const void *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
        serial1_putc(((char*) buf)[i]);
    return len;
}

int serial1read(iop_t *iop, void *buf, uint32_t len)
{
    ((char*) buf)[0] = serial1_getc();
    return 1;
}

/////////////////////////////////////////////////////////

/** This will enable interrupts. If called before nkern_start(),
 * interrupts should be disabled. **/
iop_t *serial0_configure(int pclk, int baud)
{
    nkern_wait_list_init(&rx_waitlist[0], "uart0_rx");
    nkern_mutex_init(&rx_mutex[0], "uart0_rx");

    nkern_wait_list_init(&tx_waitlist[0], "uart0_tx");
    nkern_mutex_init(&tx_mutex[0], "uart0_tx");

    SYSCTL_RCGC1_R |= (1<<0);  // UART0
    GPIO_PORTA_DIR_R   &= ~((1<<0) | (1<<1)); // 1 = output
    GPIO_PORTA_AFSEL_R |= (1<<0) | (1<<1);    // 1 = hw func
    GPIO_PORTA_DEN_R   |= (1<<0) | (1<<1);    // 1 = gpio enabled

    UART0_LCRH_R &= ~UART_LCRH_FEN;
    UART0_CTL_R &= ~(UART_CTL_UARTEN | UART_CTL_TXE | UART_CTL_RXE);

    // configure
    uint32_t div = ((pclk*8/baud) + 1) / 2;
    UART0_IBRD_R = div / 64;
    UART0_FBRD_R = div % 64;

    UART0_LCRH_R = UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
        UART_CONFIG_PAR_NONE;

    UART0_FR_R = 0;   // enable, clear flags

    // for lower latency (at the expense of losing the FIFO
    // altogether!)  set low_latency=1.
    // XXX BUG: low_latency=0 doesn't actually work.
    int low_latency = 0;
    if (low_latency) {
        UART0_LCRH_R &= ~UART_LCRH_FEN;  // fifo disabled
    } else {
        UART0_LCRH_R |= UART_LCRH_FEN;  // fifo enabled
    }

    UART0_CTL_R |= (UART_CTL_UARTEN | UART_CTL_TXE | UART_CTL_RXE);

    nvic_set_handler(21, serial0_irq);

    UART0_IM_R = 0;  // no interrupts initially.

    NVIC_EN0_R |= (1<<NVIC_UART0);          // enable UART0 interrupts

    memset(&serial0_iop, 0, sizeof(serial0_iop));
    serial0_iop.user = NULL;
    serial0_iop.write = serial0write;
    serial0_iop.read = serial0read;

    return &serial0_iop;
}

void serial1_halfduplex_enable_tx(int enable)
{
    if (enable) {
        GPIO_PORTD_AFSEL_R |= (1<<3);
    } else {
        GPIO_PORTD_AFSEL_R &= ~(1<<3);
    }
}

iop_t *serial1_configure_halfduplex(int pclk, int baud)
{
    nkern_wait_list_init(&rx_waitlist[1], "uart1w_rx");
    nkern_mutex_init(&rx_mutex[1], "uart1m_rx");

    nkern_wait_list_init(&tx_waitlist[1], "uart1w_tx");
    nkern_mutex_init(&tx_mutex[1], "uart1m_tx");

    SYSCTL_RCGC1_R |= (1<<1);  // UART1 (D2=RX, D3=TX)
    // configure as inputs. afsel will override.
    GPIO_PORTD_DIR_R   &= ~((1<<2) | (1<<3));
    GPIO_PORTD_AFSEL_R |= (1<<2) | (1<<3);
    GPIO_PORTD_DEN_R   |= (1<<2) | (1<<3);
    GPIO_PORTD_PUR_R   |= (1<<2); // pull up on RX

    serial1_halfduplex_enable_tx(0);

    UART1_LCRH_R &= ~UART_LCRH_FEN;
    UART1_CTL_R &= ~(UART_CTL_UARTEN | UART_CTL_TXE | UART_CTL_RXE);

    // configure
    uint32_t div = ((pclk*8/baud) + 1) / 2;
    UART1_IBRD_R = div / 64;
    UART1_FBRD_R = div % 64;

    UART1_LCRH_R = UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
        UART_CONFIG_PAR_NONE;

    UART1_FR_R = 0;   // enable, clear flags

    // for lower latency (at the expense of losing the FIFO
    // altogether!)  set low_latency=1.
    // XXX BUG: low_latency=0 doesn't actually work.
    int low_latency = 0;
    if (low_latency) {
        UART1_LCRH_R &= ~UART_LCRH_FEN;  // fifo disabled
    } else {
        UART1_LCRH_R |= UART_LCRH_FEN;  // fifo enabled
    }

    UART1_CTL_R |= (UART_CTL_UARTEN | UART_CTL_TXE | UART_CTL_RXE);

    nvic_set_handler(22, serial1_irq);

    UART1_IM_R = 0;  // no interrupts initially.

    NVIC_EN0_R |= (1<<NVIC_UART1);          // enable UART0 interrupts (guess)

    memset(&serial1_iop, 0, sizeof(serial1_iop));
    serial1_iop.user = NULL;
    serial1_iop.write = serial1write;
    serial1_iop.read = serial1read;

    return &serial1_iop;
}

int serial1_halfduplex_write(const char *msg, int len)
{
    serial1_halfduplex_enable_tx(1);

    for (int i = 0; i < len; i++) {
        serial1_putc(msg[i]);

        // consume the character but use a timeout so that if the
        // half-duplex circuitry is not installed correctly, we won't
        // stall forever.
        while (1) {
            nkern_wait_t waits[1];
            uint64_t endtime = nkern_utime() + 1000;
            serial1_read_wait_setup(&waits[0]);
            int ret = nkern_wait_many_until(waits, 1, endtime);

            if (ret < 0) {
                // timeout!
                goto bail;
            } else {
                char c = serial1_getc_nonblock();
                // not the character we transmitted? read again.
                if (c != msg[i])
                    continue;
                else
                    break;
            }
        }

    }

    // Wait for transmission to actually finish before disabling
    // output. We shouldn't actually spin at all, however, because the
    // getc() above will block until transmission is complete.

bail:
    while (UART1_FR_R & (1<<3));

    serial1_halfduplex_enable_tx(0);

    return len;
}

void serial1_read_wait_setup(nkern_wait_t *w)
{
    irqstate_t flags;

    nkern_mutex_lock(&rx_mutex[1]);

    interrupts_disable(&flags);

    UART1_IM_R |= (1<<RX_IRQ) | (1<<RX_TIMEOUT_IRQ);

    // XXX: what if the character has already been received?
    // XXX: Don't use RX_TIMEOUT_IRQ ?

    nkern_init_wait_event(w, &rx_waitlist[1]);

    interrupts_restore(&flags);

    nkern_mutex_unlock(&rx_mutex[1]);
}

int serial1_read_available()
{
    return (UART1_FR_R & UART_FR_RXFE) ? 0 : 1;
}

int serial1_getc_nonblock()
{
    irqstate_t flags;

    nkern_mutex_lock(&rx_mutex[1]);

    interrupts_disable(&flags);

    while (UART1_FR_R & UART_FR_RXFE) {
        interrupts_restore(&flags);
        nkern_mutex_unlock(&rx_mutex[1]);
        return -1;
    }

    char c = UART1_DR_R;

    interrupts_restore(&flags);

    nkern_mutex_unlock(&rx_mutex[1]);
    return c;
}
