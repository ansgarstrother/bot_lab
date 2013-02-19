#include "config.h"
#include "i2c.h"
#include "lm3s8962.h"
#include "udp_stream.h"

static nkern_mutex_t i2c_mutex;
static nkern_wait_list_t i2c_wait_list;

static uint8_t *rxbuf;
static const uint8_t *txbuf;
static int32_t rxlen, txlen;
static int32_t rxpos, txpos;
static int i2c_error;
static uint8_t i2c_addr;

#define I2C_RUN  (1<<0)
#define I2C_START (1<<1)
#define I2C_STOP (1<<2)
#define I2C_ACK (1<<3)

// NOTE: Assumption is that no other masters exist on bus.
//
// Entire Transmit occurs and then once the entire send buffer has
// been sent the I2C interface waits until entire receive buffer
// filled
//
// If an error flag rises the entire process ends and a 1 is
// returned by the i2c_transaction() function
void i2c_irq_real(void) __attribute__((noinline));
void i2c_irq_real()
{
    uint32_t status = I2C0_MASTER_MCS_R;
    uint32_t reschedule = 0;
    uint32_t start = 0;
    uint32_t delay;

    // Wait for Busy flag to end. XXX Not sure when we need to wait for this.
//    while (I2C0_MASTER_MCS_R & (1<<0));

    // acknowledge this interrupt.
    I2C0_MASTER_MICR_R = (1<<0);

    if (txpos < txlen) {

        if (txpos == 0) {  // If first byte of transmission, set START condition
            start = I2C_START;
            I2C0_MASTER_MSA_R = i2c_addr << 1;
        }

//        I2C0_MASTER_MCS_R = 0;  // Clear for new settings (XXX?)

        // stuff the output FIFO
        I2C0_MASTER_MDR_R = txbuf[txpos++];

        if (txpos < txlen) {
            // more to send...
            I2C0_MASTER_MCS_R = I2C_RUN | start;

        } else {
            // if we're not receiving, then send stop bit. Otherwise,
            // just run. (We'll send repeated start below).
            if (rxlen == 0 )  // If no receive
                I2C0_MASTER_MCS_R = I2C_STOP | I2C_RUN | start;
            else
                I2C0_MASTER_MCS_R = I2C_RUN | start;
        }

    } else if (rxpos < rxlen && rxlen > 0) {

        if (rxpos == -1) {
            // first byte: start the transaction, can't read until next time we're called.
            start = I2C_START; // could be either first start or repeated start.
            I2C0_MASTER_MSA_R = (i2c_addr << 1) | (1<<0); // receive mode
            rxpos++;

        } else {
            // we've got some data
            // Read value from last receive
            rxbuf[rxpos++] = I2C0_MASTER_MDR_R;
        }

        if (rxpos == rxlen) {
            // we've read everything, end the transaction
            I2C0_MASTER_MCS_R = I2C_STOP | I2C_RUN | start;
        } else {
            // need more data, set up another read.
            I2C0_MASTER_MCS_R = I2C_ACK | I2C_RUN | start;
        }

    } else {
        // no work left to do. We've tx'd and rx'd and finished the STOP.
        // are we all done?
        if (((rxlen == 0 || rxpos == rxlen) && txpos == txlen) || i2c_error) {

            reschedule |= _nkern_wake_all(&i2c_wait_list);

            // mask further interrupts
            I2C0_MASTER_MIMR_R = 0;

        } else {
            kprintf("weird i2c state");
        }

        if (reschedule)
            _nkern_schedule();
    }

    // anything bad happen?
    i2c_error |= (status & (1<<1));

}

static void i2c_irq(void) __attribute__ ((naked));
static void i2c_irq(void)
{
    IRQ_TASK_SAVE;
    NKERN_IRQ_ENTER;
    i2c_irq_real();
    NKERN_IRQ_EXIT;
    IRQ_TASK_RESTORE;
}

void i2c_init()
{
    nkern_wait_list_init(&i2c_wait_list, "i2c");
    nkern_mutex_init(&i2c_mutex,"i2c");

    nvic_set_handler(16+NVIC_I2C0, i2c_irq);
    NVIC_EN0_R |= (1<<NVIC_I2C0);
}

void i2c_lock()
{
    nkern_mutex_lock(&i2c_mutex);
}

void i2c_unlock()
{
    nkern_mutex_unlock(&i2c_mutex);
}

// Only master mode supported
void i2c_config(uint32_t i2c_hz)
{
    I2C0_MASTER_MCR_R = (1<<4);  // Master Mode Enable (datasheet says 0x20)

    // i2c_hz = cpu_hz / (20*(1+timer_prd))
    // extra term in numerator gives us correct rounding (round towards larger timer_prd)
    uint32_t timer_prd = (CPU_HZ + 20*i2c_hz - 1) / (20*i2c_hz) - 1;
    I2C0_MASTER_MTPR_R = timer_prd;
}

// Perform a split write/read transaction with a repeated start bit.
//
// addr: [0, 127]
// writebuf: what to write to i2c
// writelen: how many bytes to write
// readbuf: where to write data read from i2c
// readlen: how many bytes to read from i2c

// returns 0 on success.
int i2c_transaction(uint32_t addr, uint8_t *writebuf, uint32_t writelen, uint8_t *readbuf, uint32_t readlen)
{
    i2c_lock();

    while (I2C0_MASTER_MCS_R & (1<<0));

    i2c_addr = addr;

    rxbuf = readbuf;
    txbuf = writebuf;
    rxlen = readlen;
    txlen = writelen;
    rxpos = -1;
    txpos = 0;
    i2c_error = 0;

    irqstate_t flags;
    interrupts_disable(&flags);

    I2C0_MASTER_MIMR_R = (1<<0); // Promote interrupts to controller (enable interrupt)
    NVIC_PEND0_R = (1<<NVIC_I2C0); // force an interrupt now to start the ISR-driven transaction
    nkern_wait(&i2c_wait_list); // wait for transaction to complete
    interrupts_restore(&flags);

    int err = i2c_error;

    i2c_unlock();

    return err;  // Notify in return if an error occurred (1)
}
