#include <stdint.h>
#include <nkern.h>
#include <lpc23xx.h>

#include "serial.h"

// config register fields
#define BIT_ENABLE (1<<2)
#define CPHA (1<<3)
#define CPOL (1<<4)
#define MASTER (1<<5)
#define LSBF (1<<6)
#define SPIE (1<<7)
#define BITS_SHIFT 8

// status register fields
#define ABRT (1<<3)
#define MODF (1<<4)
#define ROVR (1<<5)
#define WCOL (1<<6)
#define SPIF (1<<7)

static nkern_wait_list_t spi_waitlist;

void spi_init()
{
    int spi_pwr = 0, ss0_pwr = 0, ss1_pwr = 1;

    PCONP |= (spi_pwr<<8) | (ss0_pwr<<21) | (ss1_pwr<<10);

    // P0.7 - P0.9 
    if (ss1_pwr) {
        PINSEL0 = (PINSEL0 & ~(3<<14)) | (2<<14);
        PINSEL0 = (PINSEL0 & ~(3<<16)) | (2<<16);
        PINSEL0 = (PINSEL0 & ~(3<<18)) | (2<<18);

        SSP1IMSC = 0; // no interrupts, please.
    }

    nkern_wait_list_init(&spi_waitlist, "spi");
}

static void spi_irq_real(void) __attribute__ ((noinline));
static void spi_irq_real()
{
}

static void spi_irq(void) __attribute__ ((naked));
static void spi_irq(void)
{
    IRQ_TASK_SAVE;
    NKERN_IRQ_ENTER;
    spi_irq_real();
    NKERN_IRQ_EXIT;
    IRQ_TASK_RESTORE;
}

#define FRAME_SPI 0

int spi_write(uint32_t data, int len)
{
/*
    S0SPCR = BIT_ENABLE | MASTER | (len<<BITS_SHIFT);
    S0SPCCR = 16; // PCLK divider, must be even, must be >= 8
    S0SPDR = data;
*/

    int cpol = 0;
    int cphase = 0;
    int clkdiv = 0;
    SSP1CR0 = ((len-1) << 0) | (FRAME_SPI << 4) | (cpol << 6) | (cphase << 7) | (clkdiv << 8);

    int loopback = 0;
    int enable = 1;
    int slave_mode = 0;
    int slave_out_disable = 0;
    SSP1CR1 = (loopback << 0) | (enable << 1) | (slave_mode << 2) | (slave_out_disable << 3);
    SSP1DR = data;

    while (SSP1SR & (1<<4))
      nkern_usleep(200);
    //      nkern_yield();

    return 0;
}

