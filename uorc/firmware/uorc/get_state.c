#include <nkern.h>
#include <net.h>

#include "digio.h"
#include "intadc.h"
#include "extadc.h"
#include "motor.h"
#include "qei.h"
#include "fast_digio.h"

nkern_mutex_t status_mutex;

void get_state_init()
{
    nkern_mutex_init(&status_mutex, "status");
}

int get_state(uint8_t *buf)
{
    // prevent other threads from updating the status state while
    // we're encoding it!
    nkern_mutex_lock(&status_mutex);

    net_encode_u32(&buf[0],                            // status flags
                   (digio_get_estop()<<0) |
                   (motor_get_watchdog_state()<<1) |
                   (motor_get_watchdog_count()<<24));

    net_encode_u16(&buf[4], 0);   // debug chars waiting

    for (int i = 0; i < 8; i++) {
        net_encode_u16(&buf[6 + i*6], extadc_get(i));
        net_encode_u16(&buf[8 + i*6], extadc_get_filtered(i));
        net_encode_u16(&buf[10 + i*6], extadc_get_filter_alpha(i));
    }

    for (int i = 0; i < 5; i++) {
        net_encode_u16(&buf[54 + i*6], intadc_get(i));
        net_encode_u16(&buf[56 + i*6], intadc_get_filtered(i));
        net_encode_u16(&buf[58 + i*6], intadc_get_filter_alpha(i));
    }

    net_encode_u32(&buf[84], digio_get());  // simple digital IO values
    net_encode_u32(&buf[88], digio_get_config()); // simple digital IO direction and pullups

    for (int i = 0; i < 3; i++) {
        net_encode_u8(&buf[92 + i*7], motor_get_enable(i));
        net_encode_u16(&buf[93 + i*7], motor_get_pwm_actual(i)>>7);
        net_encode_u16(&buf[95 + i*7], motor_get_pwm_goal(i));
        net_encode_u16(&buf[97 + i*7], motor_get_slew(i));
    }

    net_encode_u32(&buf[113], qei_get_position(0));
    net_encode_u32(&buf[117], qei_get_velocity(0));
    net_encode_u32(&buf[121], qei_get_position(1));
    net_encode_u32(&buf[125], qei_get_velocity(1));

    for (int i = 0; i < 8; i++) {
        uint32_t mode, value;
        fast_digio_get_configuration(i, &mode, &value);
        net_encode_u8(&buf[129 + i*5], mode);
        net_encode_u32(&buf[130 + i*5], value);
    }

    for (int i = 0; i < 3; i++) {
        uint64_t integrator;
        uint32_t count;

        extadc_get_gyro(i, &integrator, &count);
        net_encode_u64(&buf[169] + i*12, integrator);
        net_encode_u32(&buf[177] + i*12, count);
    }

    nkern_mutex_unlock(&status_mutex);
    return 205;
}
