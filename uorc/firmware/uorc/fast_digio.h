#ifndef _SERVO_H
#define _SERVO_H

#include <stdint.h>
#include "nkern.h"
#include "lm3s8962.h"

#define FAST_DIGIO_MODE_IN 1
#define FAST_DIGIO_MODE_OUT 2
#define FAST_DIGIO_MODE_SERVO 3
#define FAST_DIGIO_MODE_SLOW_PWM 4

void fast_digio_init();

void fast_digio_get_configuration(uint32_t port, uint32_t *mode, uint32_t *value);
void fast_digio_set_configuration(uint32_t port, uint32_t mode, uint32_t value);
void servo_set(int servo, uint32_t usec);
uint32_t servo_get_us(int servo);

// the mode must already be set!
static inline void fast_digio_set_digital_out(uint32_t port, uint32_t v)
{
    if (v != 0)
        v = 1;

    switch (port)
    {
    case 0:
        gpio_set_bit(GPIO_PORTD_DATA_BITS_R, 4, v);
        break;

    case 1:
        gpio_set_bit(GPIO_PORTA_DATA_BITS_R, 6, v);
        break;

    case 2:
        gpio_set_bit(GPIO_PORTA_DATA_BITS_R, 7, v);
        break;

    case 3:
        gpio_set_bit(GPIO_PORTB_DATA_BITS_R, 4, v);
        break;

    case 4:
        gpio_set_bit(GPIO_PORTB_DATA_BITS_R, 5, v);
        break;

    case 5:
        gpio_set_bit(GPIO_PORTB_DATA_BITS_R, 6, v);
        break;

    case 6:
        gpio_set_bit(GPIO_PORTC_DATA_BITS_R, 5, v);
        break;

    case 7:
        gpio_set_bit(GPIO_PORTC_DATA_BITS_R, 7, v);
        break;
    }
}

#endif
