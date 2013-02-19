#include <stdint.h>

#include "main_eth_inc.c"

typedef void (*runfunc_t)(void);

void isp_eth()
{
    // before calling:
    //   disable interrupts
    //   disable watchdog

    // copy main_eth image to sram. this address must match base addr
    // in link-ram.cmd
    register const void *destaddr = (void*) 0x20000000UL;

    if (1) {
        register char *src = (char*) __MAIN_ETH;
        register char *dest = (char*) destaddr;
        register int32_t cnt = __MAIN_ETH_SIZE;

        for ( ; cnt >= 0; cnt--) {
            *dest++ = *src++;
        }
    }

    // grab initial SP and PC from the vector table (which must be at
    // the beginning of the image!)
    register uint32_t sp = ((uint32_t*) destaddr)[0];

    register runfunc_t runfunc = (runfunc_t) ((runfunc_t*) destaddr)[1];

    asm volatile("msr PSP, %0          \r\n\t"
                 "msr MSP, %0          \r\n\t"
                 :: "r" (sp));
    runfunc();

    // should never reach this.
    while(1);
}
