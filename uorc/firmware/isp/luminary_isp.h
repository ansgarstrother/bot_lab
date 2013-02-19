#ifndef _LUMINARY_H
#define _LUMINARY_H

#define SYSCTL_SYSDIV_1         0x07800000  // Processor clock is osc/pll /1
#define SYSCTL_SYSDIV_4         0x01C00000  // Processor clock is osc/pll /
#define SYSCTL_USE_PLL          0x00000000  // System clock is the PLL clock
#define SYSCTL_USE_OSC          0x00003800  // System clock is the osc clock
#define SYSCTL_XTAL_8MHZ        0x00000380  // External crystal is 8MHz
#define SYSCTL_XTAL_6MHZ        0x000002C0  // External crystal is 6MHz
#define SYSCTL_OSC_MAIN         0x00000000  // Oscillator source is main osc

#define SYSCTL_INT_PLL_LOCK     0x00000040  // PLL lock interrupt
#define SYSCTL_RCC_OEN          0x00001000  // PLL Output Enable.
#define SYSCTL_LDOPCTL_2_75V    0x0000001B  // LDO output of 2.75V

void SysCtlClockSet(unsigned long ulConfig);

#endif

