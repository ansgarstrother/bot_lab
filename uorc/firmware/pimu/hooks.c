#include <nkern.h>
#include "lm3s8962.h"
#include "config.h"

static uint32_t systick_count;

extern void serial0_putc_spin(char c);

void interrupt_systick(void) __attribute__((naked));
void interrupt_systick()
{
    IRQ_TASK_SAVE;
    NKERN_IRQ_ENTER;

    systick_count++;
    if (systick_count == (1<<11)) {
        systick_count = 0;
    }

    _nkern_periodic(); // we could call this less often, perhaps.
    _nkern_schedule();

    // give this task a whole quantum
    NVIC_ST_RELOAD_R = (CPU_HZ / SCHEDULER_HZ) - 1;
    NKERN_IRQ_EXIT;
    IRQ_TASK_RESTORE;
}

void interrupt_systick_once(void) __attribute__((naked));
void interrupt_systick_once()
{
    nvic_set_handler(15, interrupt_systick);
    IRQ_TASK_RESTORE;
}

void nkern_fatal(uint32_t v)
{
    while(1) {
        pprintf_putc(serial0_putc_spin, "NKERN FATAL %04x\n", (unsigned int) v);
    }
}


static uint32_t last_cyccnt;
static uint64_t utime;

// Guaranteed to be called at least once every scheduler quantum.
uint64_t nkern_utime()
{
    irqstate_t flags;

    interrupts_disable(&flags);

    uint32_t cyccnt = DWT_CYCCNT_R; // counts at CPU_HZ

    uint32_t ncycs = cyccnt - last_cyccnt;
    uint32_t usecs = ncycs / (CPU_HZ / 1000000);
    last_cyccnt += usecs * (CPU_HZ / 1000000);

    utime += usecs;

    interrupts_restore(&flags);

    return utime;
}


// This function is called by nkern_start() in order to transition to
// pre-emptive multi-tasking mode
void _nkern_bootstrap()
{
    irqstate_t flags;
    interrupts_disable(&flags);

    DEMCR_R |= (1<<24);      // enable TRCEN (needed for DWT)
    DWT_CTRL_R |= 1;         // enable CYCCNT
    DWT_CTRL_R |= (1<<12);   // enable PC sampling (for profiling)

    nvic_set_handler(15, interrupt_systick_once);

    // enable systick
    NVIC_ST_CTRL_R |= NVIC_ST_CTRL_CLK_SRC | NVIC_ST_CTRL_ENABLE | NVIC_ST_CTRL_INTEN;   
    NVIC_ST_RELOAD_R = 1; 

    interrupts_force_enable();
    while(1);
}


