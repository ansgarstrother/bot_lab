#include <stdint.h>

#include <nkern.h>

#include "lpc23xx.h"

// LPC internal ADC

void adc_init(uint32_t portmask)
{
    AD0CR &= ~(1<<21); // disable ADC
    PCONP |= (1<<12);  // power on ADC

    if (portmask & (1<<0))
        PINSEL1 = (PINSEL1 & ~(3<<14)) | (1<<14);
    if (portmask & (1<<1))
        PINSEL1 = (PINSEL1 & ~(3<<16)) | (1<<16);
    if (portmask & (1<<2)) 
        PINSEL1 = (PINSEL1 & ~(3<<18)) | (1<<18);
    if (portmask & (1<<3)) 
        PINSEL1 = (PINSEL1 & ~(3<<20)) | (1<<20);

    if (portmask & (1<<4))
        PINSEL3 = (PINSEL3 & ~(3<<28)) | (3<<28);
    if (portmask & (1<<5))
        PINSEL3 = (PINSEL3 & ~(3<<30)) | (3<<30);
    
    if (portmask & (1<<6))
        PINSEL0 = (PINSEL0 & ~(3<<24)) | (3<<24);
    if (portmask & (1<<7))
        PINSEL0 = (PINSEL0 & ~(3<<26)) | (3<<26);
    
    AD0INTEN = 0;

    int clockdiv = 20;
    int burst = 1;
    int on = 1;
    int start = 0;

    AD0CR = (portmask & 0xff) | (clockdiv << 8) | (burst<<16) | (on<<21) | (start<<24);

}

// return [0x0000, 0xffff]
uint32_t adc_read(int port)
{
    uint32_t v = 0;

loop:
    switch (port) 
    {
    case 0:
        v = AD0DR0;
        break;
    case 1:
        v = AD0DR1;
        break;
    case 2:
        v = AD0DR2;
       break;
    case 3:
        v = AD0DR3;
       break;
    case 4:
        v = AD0DR4;
       break;
    case 5:
        v = AD0DR5;
        break;
    case 6:
        v = AD0DR6;
        break;
    case 7:
        v = AD0DR7;
        break;
   }

    // no data yet?
    if (!(v & (1<<31))) {
        nkern_yield();
        goto loop;
    }


    return v & 0xffff;
}
