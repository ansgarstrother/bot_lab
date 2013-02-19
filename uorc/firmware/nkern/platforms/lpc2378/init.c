#include <stdint.h>
#include <nkern.h>
#include "lpc23xx.h"

extern int32_t _edata[], _etext[], _data[];
extern int32_t __bss_start__[], __bss_end__[];

#define OSCRANGE      (1<<4)
#define OSCEN         (1<<5)
#define OSCSTAT       (1<<6)
#define PLLCON_PLLE   (1<<0)
#define PLLCON_PLLC   (1<<1)
#define PLLSTAT_M     (0x7fff<<0)
#define PLLSTAT_N     (0xff << 16)
#define PLLSTAT_PLOCK (1<<26)
#define PLLSTAT_PLLC  (1<<25)
#define PLLSTAT_PLLE  (1<<24)

void pllfeed(void)
{
    asm("LDR R0, =0xAA\r\n"
        "LDR R1, =0x55\r\n"
        "STR R0, [%0]\r\n"
        "STR R1, [%0]\r\n"
        : : "r" (&PLLFEED) : "r0", "r1");
}

void init_hardware(void)
{
    /////////////// PLL ////////////////
    uint32_t pll_mul = 0x0b;         // pll = (pll_mul +  1) * osc * 2

    SCS = OSCEN;                // enable oscillator
    while (!(SCS & OSCSTAT));   // wait for it to become ready
    SCS |= (1<<0);  // fast I/O on ports 0 and 1

    if (PLLSTAT & PLLSTAT_PLLC) {  
        // disconnect PLL if it is connected
        PLLCON = PLLCON_PLLE;                 
        pllfeed();
    }

    // disable the PLL; our CPU is now clocked by the external oscillator
    PLLCON = 0;
    pllfeed();

    CLKSRCSEL = 0x01;           // main oscillator is PLL clock source
    PLLCFG    = pll_mul;        // PLL multiplier
    pllfeed();

    PLLCON = PLLCON_PLLE;       // enable PLL, but leave it disconnected
    pllfeed();

    // cpu clock divider
    //    CCLKCFG = 0x03;  // cpu clk = pll / (cpu divider + 1)
    CCLKCFG = 0x05;  // cpu clk = pll / (cpu divider + 1)

    // wait for PLL to become ready
    while ((PLLSTAT & PLLSTAT_PLOCK)==0);  

    // make sure M and N are correct
    while ((PLLSTAT & (PLLSTAT_M | PLLSTAT_N)) != pll_mul); 

    // connect the PLL
    PLLCON = PLLCON_PLLE | PLLCON_PLLC;
    pllfeed();
    while ((PLLSTAT & PLLSTAT_PLLC)==0);

    USBCLKCFG = 0x5; // usb clk = pll / (usb divider + 1) MUST BE 48 MHz IF USING USB.
    PCLKSEL0 = 0;    // peripheral clocks, cclk/4
    PCLKSEL1 = 0;    // cclk/4


    /////////////// MAM /////////////////
    MAMCR = 0x00; // disable MAM
    //    MAMTIM = 4;   // CCLK cycles per flash read (roughly cclk/20mhz)
    MAMTIM = 4;
    MAMCR = 0x02;
    //    MAMCR = 0x02; // MAM fully enabled
    //    MAMCR = 0x01; // MAM partially enabled

    ////////////////////////////////////
    // this stuff is usually done in crt0.s, but we do it after
    // hardware initialization, because it'll take a fraction of the
    // time once we've switched the cpu clock.

    // copy .data section
    for (int32_t *src = _etext, *dst = _data ; dst != _edata ; src++, dst++)
        (*dst) = (*src);

    // zero .bss section
    for (int32_t *p = __bss_start__; p != __bss_end__; p++)
        (*p) = 0;
}


