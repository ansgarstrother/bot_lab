#include "gyros.h"
#include "param.h"

// 0: Z x 1
// 1: Z x 4
// 2: X x 4
// 3: X x 1
// 4: Z x 1
// 5: Z x 4
// 6: X x 4
// 7: X x 1
static int64_t integrator[8]; // in units of radians * 10^9
static uint32_t ncount;

static nkern_mutex_t mutex;

// for bizarre reasons, the addresses that you request for particular
// channels is not sane. This array maps a channel number to an address.
const uint8_t channel_to_address[] = { 0, 4, 1, 5, 2, 6, 3, 7 };

///////////////////////////////////////////////////////////////////////////
// channel: [0, 7]
static uint32_t ads8345_read(uint32_t channel)
{
    ssi_lock();
    ssi_config(1000000, 0, 1, 8); // 2 MHz max SPI frequency for ADS8345
    gpio_set_bit(GPIO_PORTD_DATA_BITS_R, 6, 0); // assert adc SS

    // Command byte (first byte of every transfer)
    // bit 7: always 1
    // bit 4-6: channel
    // bit 2: 1 single ended, 0 differential
    // bit 0-1: power down (0 = power down between conversions, with instant wake-up, 3 = always on)

    uint8_t cmd = (1<<7) | (channel_to_address[channel] << 4) | (1<<2) | (3<<0);
    uint32_t tx[4], rx[4];
    tx[0] = cmd;
    tx[1] = 0;
    tx[2] = 0;
    tx[3] = 0;

    ssi_rxtx(tx, rx, 4);

    gpio_set_bit(GPIO_PORTD_DATA_BITS_R, 6, 1); // de-assert adc SS
    ssi_unlock();

    return (((rx[1]<<16) | (rx[2]<<8) | (rx[3])) >> 7 ) & 0xffff;
}

static void gyros_task(void *arg)
{
    uint32_t tmp[8];

    uint64_t lastutime = nkern_utime();

    while (1) {
        uint64_t utime = nkern_utime();

        // read raw data
        for (int i = 0; i < 8; i++) {
            tmp[i] = ads8345_read(i); // at most 2^16
        }

        // update integrator... at most 2^12
        int64_t usecs = (utime - lastutime);

        nkern_mutex_lock(&mutex);

        for (int i = 0; i < 8; i++) {
            int64_t v = tmp[i];
            if (v & (0x8000))
                v |= 0xffffffffffff0000L;

            double delta = v - flash_params->gyro_offset[i];
            delta *= flash_params->gyro_gain[i];
            delta *= usecs;
            integrator[i] += (int64_t) delta;
        }

        lastutime = utime;

        nkern_mutex_unlock(&mutex);

        nkern_yield();
    }
}

void gyros_get_data(uint32_t *data)
{
    nkern_mutex_lock(&mutex);

    for (int i = 0; i < 8; i++) {
        data[i] = (uint32_t) (integrator[i] / 1000);
    }

    nkern_mutex_unlock(&mutex);
}

void gyros_init()
{
    nkern_mutex_init(&mutex, "gyros");

    nkern_task_create("gyros",
                      gyros_task, NULL,
                      NKERN_PRIORITY_NORMAL, 1536);

}
