#include "config.h"
#include "ssi.h"
#include "lm3s8962.h"

static nkern_mutex_t ssi_mutex;

static nkern_wait_list_t ssi_wait_list;

static uint32_t *rxbuf;
static const uint32_t *txbuf;
static uint32_t rxlen, txlen;
static uint32_t rxpos, txpos;

void ssi_irq_real(void) __attribute__((noinline));
void ssi_irq_real()
{
    uint32_t status = SSI0_SR_R;
    uint32_t reschedule = 0;

    if ((status & (1<<1)) && txpos < txlen) {
        SSI0_DR_R = txbuf[txpos++];
        if (txpos == txlen)
            SSI0_IM_R = (1<<1) | (1<<2); // only read irqs from now on.
    }

    if ((status & (1<<2)) && rxpos < rxlen) {
        rxbuf[rxpos++] = SSI0_DR_R;
        SSI0_ICR_R = (1<<1); // clear rx timeout irq.
    }

    if (rxpos == rxlen && txpos == txlen) {
        SSI0_IM_R = 0; // mask all interrupts.
        reschedule |= _nkern_wake_all(&ssi_wait_list);
    }

    if (reschedule)
        _nkern_schedule();
}

static void ssi_irq(void) __attribute ((naked));
static void ssi_irq(void) 
{
    IRQ_TASK_SAVE;
    NKERN_IRQ_ENTER;
    ssi_irq_real();
    NKERN_IRQ_EXIT;
    IRQ_TASK_RESTORE;
}


void ssi_init()
{
    nkern_wait_list_init(&ssi_wait_list, "ssi");
    nkern_mutex_init(&ssi_mutex,"ssi");

    nvic_set_handler(16+NVIC_SSI0, ssi_irq);
    NVIC_EN0_R |= (1<<NVIC_SSI0);
}

void ssi_lock()
{
    nkern_mutex_lock(&ssi_mutex);
}

void ssi_unlock()
{
    nkern_mutex_unlock(&ssi_mutex);
}


/** You'll need to call this for every transaction, since different
 * devices could reconfigure the port.
 **/
void ssi_config(uint32_t maxclk, int spo, int sph, int nbits)
{
    // ssiclk = sysclk / (cps * (1 + scr)
    // NB: cps must be even, [2, 254]
    //     scr is [0, 255]
    
    uint32_t div = CPU_HZ / maxclk;

    int cps_divider = 2;
    int scr_divider = div / 2;
    if (scr_divider > 255)
        scr_divider = 255;

    SSI0_CR1_R  &= ~(1<<1);    // disable SSI
    SSI0_CR1_R   = 0;          // master mode
    SSI0_CPSR_R  = cps_divider; 
    SSI0_CR0_R   = (scr_divider<<8) | (spo<<6) | (sph<<7) | (0<<4) | (nbits-1);
    SSI0_CR1_R  |= (1<<1);     // enable SSI
}

/** You should have called ssi_config already, and you should hold the
 * ssi_lock while you do your entire transaction (drop ss, config +
 * rxtx, raise ss).
 **/
int ssi_rxtx(const uint32_t *tx, uint32_t *rx, uint32_t nwords)
{
    // wait for any pending writes (shouldn't need to)
    while (!(SSI0_SR_R & (1<<0)))
        nkern_yield();

    // drain the rx FIFO. (shouldn't need to)
    while (SSI0_SR_R & (1<<2)) {
        uint32_t v = SSI0_DR_R;
        (void) v;
    }

    //////////////////////////////////////////////////
    rxbuf = rx;
    txbuf = tx;
    rxlen = nwords;
    txlen = nwords;
    rxpos = 0;
    txpos = 0;

    irqstate_t flags;
    interrupts_disable(&flags);
    
    SSI0_IM_R = (1<<1) | (1<<2) | (1<<3); // rx fifo, rx timeout, tx fifo
    NVIC_PEND0_R = (1<<7);
    nkern_wait(&ssi_wait_list);
    interrupts_restore(&flags);

    return 0;
}
