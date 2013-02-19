#include <nkern.h>
#include "lm3s8962.h"

static int led_periodic_state = 0;

void status_led(int v)
{
    gpio_set_bit(GPIO_PORTA_DATA_BITS_R, 3, v);
}

int32_t led_periodic(void *arg)
{
    led_periodic_state = 1-led_periodic_state;

    if (led_periodic_state==0) {
        status_led(1);
        return 2000; // "on" pulse width
    }
    
    status_led(0);
    return 0;
}

void heartbeat_init()
{
    nkern_task_create_periodic("led",
                               led_periodic, NULL,
                               100000);

}
