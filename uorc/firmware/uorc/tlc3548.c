#include "mcp23s17.h"
#include "lm3s8962.h"

uint32_t tlc3548_command(uint32_t command)
{
    ssi_lock();

    ssi_config(2500000, 0, 1, 16);

    gpio_set_bit(GPIO_PORTD_DATA_BITS_R, 6, 0); // assert adc SS

    // after command, we must provide for sampling period and conversion period.
    // sampling period: 44 ssi clks for long sampling
    // conversion period: 3us for internal oscillator
    //                    
    uint32_t tx[5], rx[5];
    tx[0] = command; tx[1] = 0; tx[2] = 0; tx[3] = 0; tx[4] = 0;

    ssi_rxtx(tx, rx, 5);

    gpio_set_bit(GPIO_PORTD_DATA_BITS_R, 6, 1); // de-assert adc SS

    ssi_unlock();

    return rx[0];
}

void tlc3548_init()
{
    int vref = 0; // 0 = 4v, 1 = external

    tlc3548_command(0xa000); // initialization
    nkern_usleep(1000);
    tlc3548_command(0xa000 | (vref<<11) | (0<<9) | (0<<8)); // long sampling, internal conversion osc
    tlc3548_command(0xa000 | (vref<<11) | (0<<9) | (0<<8)); // long sampling, internal conversion osc
}

