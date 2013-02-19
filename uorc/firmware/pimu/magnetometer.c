#include "magnetometer.h"

static uint32_t channels[3];

static nkern_mutex_t mutex;

static int32_t magnetometer_read(uint8_t reg, uint8_t *readbuf, uint32_t readlen)
{
    i2c_lock();
    i2c_config(400000);
    uint8_t writebuf[] = { reg };

    i2c_transaction(0x1e, writebuf, 1, readbuf, readlen);

    i2c_unlock();

    return 0;
}

static int32_t magnetometer_write(uint8_t reg, uint8_t value)
{
    i2c_lock();
    i2c_config(400000);
    uint8_t writebuf[] = { reg, value };

    i2c_transaction(0x1e, writebuf, 2, NULL, 0);

    i2c_unlock();
    return 0;
}

/*
static uint32_t magnetometer_drdy()
{
    return (GPIO_PORTB_DATA_R >> 6) & 1;
}
*/

// hmc 5843
static void magnetometer_task(void *arg)
{
    uint8_t buf[8];

    // read identification bytes
    magnetometer_read(0x0a, buf, 3);
    if (buf[0]!=0x48 || buf[1] != 0x34 || buf[2] != 0x33) {
        kprintf("ERROR: Magnetometer not found\n");
        return;
    }

    kprintf("magnetometer found\n");

    magnetometer_write(0x02, 0); // continuous conversion mode

    magnetometer_read(0x00, buf, 3);
//    kprintf("config: %02x %02x %02x\n", buf[0], buf[1], buf[2]);

    if (buf[2]!=0x00) {
        kprintf("ERROR: Magnetometer write failed\n");
        return;
    }

    while (1) {
        // Note: doesn't seem to work if we read the status byte too. Weird.
        magnetometer_read(0x03, buf, 6); // XMSB XLSB YMSB YLSB ZMSB ZLSB

        uint32_t x = (buf[0]<<8) + (buf[1]);
        uint32_t y = (buf[2]<<8) + (buf[3]);
        uint32_t z = (buf[4]<<8) + (buf[5]);
        uint32_t status = buf[6];

        nkern_mutex_lock(&mutex);
        channels[0] = x;
        channels[1] = y;
        channels[2] = z;
        nkern_mutex_unlock(&mutex);

//        kprintf("%04x %04x %04x %02x\n", x, y, z, status);

        nkern_usleep(100000);
    }
}

void magnetometer_get_data(uint32_t *data)
{
    nkern_mutex_lock(&mutex);

    for (int i = 0; i < 3; i++) {
        data[i] = channels[i];
    }

    nkern_mutex_unlock(&mutex);
}

void magnetometer_init()
{
    nkern_mutex_init(&mutex, "magnetometer");

    nkern_task_create("magnetometer",
                      magnetometer_task, NULL,
                      NKERN_PRIORITY_NORMAL, 4096); // big stack for math functions
}
