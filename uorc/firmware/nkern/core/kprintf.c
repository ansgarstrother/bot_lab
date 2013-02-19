#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include <nkern.h>

// must be power of two
#define KPRINTF_BUFFER_SIZE 1024

static char kprintf_buffer[KPRINTF_BUFFER_SIZE]; // circular buffer
volatile int  kprintf_writepos;           
volatile int  kprintf_readpos;
static volatile int  kprintf_initted;

nkern_wait_list_t kprintf_wait_list;

static iop_t *fifo_iop; // iop that writes into our internal FIFO (non-blocking)
static iop_t *output_iop; // iop (provided by user) that actually sends the output somewhere.

void kprintf_wakeup()
{
    if (!kprintf_initted)
        return;

    nkern_wake_all(&kprintf_wait_list);
}

void kprintf_flush()
{
    while (kprintf_writepos != kprintf_readpos)
        nkern_yield();
}

int kprintf_fifo_write(iop_t *iop, const void *buf, uint32_t len)
{
    irqstate_t state;

    for (uint32_t i = 0; i < len; i++) {
        char c = ((char*) buf)[i];

        // don't wrap around in the buffer.
        if (((kprintf_writepos + 1) & (KPRINTF_BUFFER_SIZE - 1)) == kprintf_readpos) {
            kprintf_buffer[(kprintf_writepos - 1) & (KPRINTF_BUFFER_SIZE -1)] = '*';
            return len; // XXX technically not right.
        }
        
        kprintf_buffer[kprintf_writepos] = c;
        
        interrupts_disable(&state);
        kprintf_writepos = (kprintf_writepos + 1) & (KPRINTF_BUFFER_SIZE - 1);
        interrupts_restore(&state);
    }

    return len;
}

void nkern_kprintf_task(void *user)
{
    (void) user;

    while (1)
    {
        int writepos;
        irqstate_t state;

        interrupts_disable(&state);
        writepos =  kprintf_writepos;
        if (writepos == kprintf_readpos)
            nkern_wait(&kprintf_wait_list);
        interrupts_restore(&state);

        while (writepos != kprintf_readpos) {
            if (output_iop != NULL)
                putc_iop(output_iop, kprintf_buffer[kprintf_readpos]);
            kprintf_readpos = (kprintf_readpos + 1) & (KPRINTF_BUFFER_SIZE - 1);
        } 
    }
}

void kprintf(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);

    vpprintf(fifo_iop, fmt, ap);
    kprintf_wakeup();
    
    va_end(ap);
}

void nkern_kprintf_init(iop_t *iop)
{
    fifo_iop = calloc(1, sizeof(iop_t));
    fifo_iop->write = kprintf_fifo_write;

    output_iop = iop;

    nkern_wait_list_init(&kprintf_wait_list, "kprintf");

    nkern_task_create("kprintf", nkern_kprintf_task, NULL, NKERN_PRIORITY_IDLE, 512);

    kprintf_initted = 1;
}
