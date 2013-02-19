#include "accel.h"

static volatile uint32_t channels[3];

static nkern_mutex_t mutex;

#define SPI_CLK 2000000

///////////////////////////////////////////////////////////////////////////
// returns 0 on success
static int lis3lv02dl_write(uint32_t addr, uint32_t write_data)
{
    ssi_lock();
    ssi_config(SPI_CLK, 0, 1, 8); // 8 MHz max SPI frequency for LIS3LV02DL
    gpio_set_bit(GPIO_PORTD_DATA_BITS_R, 5, 0); // assert adc SS

    uint32_t tx[2], rx[2];
    tx[0] = addr; // RW (bit 7) = 0. No auto increment (bit 6 = 0)
    tx[1] = write_data;

    ssi_rxtx(tx, rx, 2);

    gpio_set_bit(GPIO_PORTD_DATA_BITS_R, 5, 1); // de-assert adc SS
    ssi_unlock();

    return 0;
}

// returns 0 on success
//
static int lis3lv02dl_read(uint32_t addr0, uint8_t data[], int len)
{
    ssi_lock();
    ssi_config(SPI_CLK, 0, 1, 8); // 8 MHz max SPI frequency for LIS3LV02DL
    gpio_set_bit(GPIO_PORTD_DATA_BITS_R, 5, 0); // assert adc SS

    uint32_t tx[len+1], rx[len+1];
    tx[0] = addr0 + (1<<7) + (1<<6); // read, auto-increment

    ssi_rxtx(tx, rx, len+1);

    gpio_set_bit(GPIO_PORTD_DATA_BITS_R, 5, 1); // de-assert adc SS
    ssi_unlock();

    for (int i = 0; i < len; i++)
        data[i] = rx[i+1];

    return 0;
}

static void lis3lv02dl_task(void *arg)
{
    nkern_usleep(5000);

    uint8_t config[16];
    config[0] = 0;

    int retry = 0;

    while (1) {
        // sometimes it takes a couple tries...
        lis3lv02dl_read(0x0f, config, 1);
        if (config[0] == 0x3a)
            break;

        if (retry > 1)
            kprintf("LIS3LV02DL: no response (trial %d)\n", retry);
        nkern_usleep(500000);
        retry++;
    }

    kprintf("LIS3LV02DL found\n");

    nkern_usleep(5000);

    // read OFFSET_X, OFFSET_Y, OFFSET_Z, GAIN_X, GAIN_Y, GAIN_Z calibration parameters.
    lis3lv02dl_read(0x16, config, 6);

    if (0) {
        kprintf("XOFF %02x, YOFF %02x, ZOFF %02x, XGAIN %02x, YGAIN %02x, ZGAIN %02x\n",
                config[0], config[1], config[2], config[3], config[4], config[5]);
    }

    nkern_usleep(5000);

    // bit 6, 7: power. (11 = on)
    // bit 4, 5: decimation control. 0 = /512, 1 = /128, 2 = /32, 3=/8.
    // bit 2, 1, 0: zenable, yenable, xenable
    lis3lv02dl_write(0x20, (3<<6) + (0<<4) + (1<<2) + (1<<1) + (1<<0));

    nkern_usleep(5000);

    // bit 7: full scale (0=2g, 1=6g)
    // bit 6: block data update (1=atomic updates of LSB/MSB)
    // bit 0: 1=16 bit
    lis3lv02dl_write(0x21, (0<<7) + (1<<6) + (1<<0));

    while (1) {
        uint8_t data[16];

        // read OUTX_L, OUTX_H, OUTY_L, OUTY_H, OUTZ_L, OUTZ_H
        lis3lv02dl_read(0x28, data, 6);

        uint32_t x = data[0] + (data[1]<<8);
        uint32_t y = data[2] + (data[3]<<8);
        uint32_t z = data[4] + (data[5]<<8);

        nkern_mutex_lock(&mutex);
        channels[0] = x;
        channels[1] = y;
        channels[2] = z;
        nkern_mutex_unlock(&mutex);

/*
        kprintf("%08x %08x %08x %08x %08x %08x %08x %08x\n",
                ads8345_read(0), ads8345_read(1), ads8345_read(2), ads8345_read(3),
                ads8345_read(4), ads8345_read(5), ads8345_read(6), ads8345_read(7));
*/

        nkern_usleep(10000); // nominal 100Hz rate.
    }
}

// data is a 3 vector
void accel_get_data(uint32_t *data)
{
    nkern_mutex_lock(&mutex);

    for (int i = 0; i < 3; i++) {
        data[i] = channels[i];
    }

    nkern_mutex_unlock(&mutex);
}

void accel_init()
{
    nkern_mutex_init(&mutex, "accel");

    nkern_task_create("accel",
                      lis3lv02dl_task, NULL,
                      NKERN_PRIORITY_NORMAL, 1536);

}
