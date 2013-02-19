#include <nkern.h>
#include <net.h>
#include "lm3s6965.h"

// XXX This doesn't seem to work properly.... perhaps our priority
// isn't actually high enough, or perhaps we're not decoding the
// interrupted PC properly (this might depend on the particular *type*
// of exception? i.e., which stack to look on?)

volatile uint32_t profile_data;

// To receive the profiling data, run this command from host 192.168.237.80:
//  nc -u -l -p 61000

static void profile_irq(void) __attribute__ ((naked));
static void profile_irq()
{
    asm volatile("mrs r1, psp            \r\n\t");
    asm volatile("ldr r0, [r1, #24]      \r\n\t" \
                 "ldr r1, =profile_data  \r\n\t" \
                 "str r0, [r1]           \r\n\t" \
                 "mov r0, #1             \r\n\t" \
                 "movw r3, #8228         \r\n\t" \
                 "movt r3, #16387        \r\n\t" \
                 "mov.w r2, #1           \r\n\t" \
                 "str   r2, [r3, #0]     \r\n\t" \
                 "bx  lr                 \r\n\t");
}

char hextochar(int v)
{
    if (v>=10)
        return 'a'+v-10;
    return '0'+v;
}

void profile_task(void *arg)
{
    while (1) {
        nkern_usleep(5000);

        udp_alloc_t udpalloc;
        int offset;

        if (udp_packet_alloc(&udpalloc, inet_addr("192.168.237.80"), &offset, 1024))
            continue;

        int len = 0;
        uint8_t *out = &udpalloc.ipalloc.p->data[offset];

        uint32_t addr = profile_data;

        out[len++] = hextochar((addr>>28)&0xf);
        out[len++] = hextochar((addr>>24)&0xf);
        out[len++] = hextochar((addr>>20)&0xf);
        out[len++] = hextochar((addr>>16)&0xf);
        out[len++] = hextochar((addr>>12)&0xf);
        out[len++] = hextochar((addr>>8)&0xf);
        out[len++] = hextochar((addr>>4)&0xf);
        out[len++] = hextochar((addr>>0)&0xf);
        out[len++] = '\n';

        udp_packet_send(&udpalloc, len, 61000, 61000);
    }
}

void profile_init()
{
    // use timer 2A for profiling. configure it to be super high priority.
    nvic_set_priority(23, 0x00);

    nvic_set_handler(23+16, profile_irq);

    TIMER2_CTL_R   &= (~(1<<0)); // disable.
    TIMER2_CFG_R   =  0;          // 32 bit timer configuration
    TIMER2_TAMR_R  =  0x02;      // periodic
    
    TIMER2_TAILR_R =  0x729;   // period
    TIMER2_IMR_R   |= (1<<0);  // enable interrupt
    TIMER2_CTL_R   |= (1<<0);    // enable timer.
    NVIC_EN0_R     |= (1<<(NVIC_TIMER2A&31));

    nkern_task_create("profile",
                      profile_task, NULL,
                      NKERN_PRIORITY_IDLE, 2048);
}

