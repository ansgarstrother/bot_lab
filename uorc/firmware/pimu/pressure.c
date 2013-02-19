#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>

#include "i2c.h"

static nkern_mutex_t mutex;
static double channels[2];

// can evidently go up to 3.4MHz (untried)
#define I2C_HZ 100000

///////////////////////////////////////////////////////////////////////////
uint32_t bmp085_read_u16(uint32_t reg)
{
    i2c_lock();
    i2c_config(I2C_HZ); // can evidently go up to 3.4MHz (untried)

    uint8_t readbuf[2];
    uint8_t writebuf[] = { reg };
    i2c_transaction(0x77, writebuf, 1, readbuf, 2);

    i2c_unlock();

    return (readbuf[0]<<8) | (readbuf[1]);
}

uint32_t bmp085_read_u24(uint32_t reg)
{
    i2c_lock();
    i2c_config(I2C_HZ); // can evidently go up to 3.4MHz (untried)

    uint8_t readbuf[3];
    uint8_t writebuf[] = { reg };
    i2c_transaction(0x77, writebuf, 1, readbuf, 3);

    i2c_unlock();

    return (readbuf[0]<<16) | (readbuf[1]<<8) | (readbuf[0]);
}

int32_t bmp085_read_s16(uint32_t reg)
{
    int32_t v = bmp085_read_u16(reg);
    if (v & 0x8000)
        v |= 0xffff0000;
    return v;
}

void bmp085_write(uint32_t reg, uint32_t value)
{
    i2c_lock();
    i2c_config(I2C_HZ);

    uint8_t writebuf[] = { reg, value };
    i2c_transaction(0x77, writebuf, 2, NULL, 0);

    i2c_unlock();
}

void bmp085_task(void *arg)
{
    int32_t ac1 = bmp085_read_s16(0xaa);
    int32_t ac2 = bmp085_read_s16(0xac);
    int32_t ac3 = bmp085_read_s16(0xae);
    int32_t ac4 = bmp085_read_u16(0xb0);
    int32_t ac5 = bmp085_read_u16(0xb2);
    int32_t ac6 = bmp085_read_u16(0xb4);
    int32_t b1 = bmp085_read_s16(0xb6);
    int32_t b2 = bmp085_read_s16(0xb8);
    int32_t mb = bmp085_read_s16(0xba);
    int32_t mc = bmp085_read_s16(0xbc);
    int32_t md = bmp085_read_s16(0xbe);

    if (0) {
        kprintf("ac1 %8d\n", ac1);
        kprintf("ac2 %8d\n", ac2);
        kprintf("ac3 %8d\n", ac3);
        kprintf("ac4 %8d\n", ac4);
        kprintf("ac5 %8d\n", ac5);
        kprintf("ac6 %8d\n", ac6);
        kprintf("b1  %8d\n", b1);
        kprintf("b2  %8d\n", b2);
        kprintf("mb  %8d\n", mb);
        kprintf("mc  %8d\n", mc);
        kprintf("md  %8d\n", md);
    }

    uint64_t last_temp_utime = 0;

    // temperature-derived values
    int32_t b5 = 0, t = 0;

    double pfilt = 0;
    double palpha = 0.90;

    while (1) {

        // update temperature periodically
        uint64_t utime = nkern_utime();
        if (utime - last_temp_utime > 400 || last_temp_utime == 0) {

            // read uncompensated temperature
            bmp085_write(0xf4, 0x2e);
            nkern_usleep(5000); // minimum 4.5ms
            int32_t ut = bmp085_read_u16(0xf6);

            // convert.
            int32_t x1 = ((ut - ac6) * ac5 ) >> 15;
            int32_t x2 = (mc<<11) / (x1 + md);
            b5 = x1+x2;
            t = (b5 + 8) >> 4;

            last_temp_utime = utime;
        }

        // get pressure.
        // using osrs = 3.

        // read uncompensated pressure
        bmp085_write(0xf4, 0xf4);
        nkern_usleep(26000); // minimum wait time 25.5 ms
        int32_t up = bmp085_read_u24(0xf6) >> 5;
        int32_t p = 0;

        int32_t b6 = b5 - 4000;
        int32_t x1 = (b2 * ( (b6 * b6) >> 12) ) >> 11;
        int32_t x2 = (ac2 * b6) >> 11;
        int32_t x3 = x1 + x2;
        int32_t b3 = (((ac1*4+x3) << 3) + 2 ) >> 2;
        x1 = (ac3 * b6) >> 13;
        x2 = (b1 * ((b6 * b6) >> 12)) >> 16;
        x3 = ((x1 + x2) + 2) >> 2;
        int64_t b4 = (int32_t) ((ac4 * ((int64_t) x3 + 32768)) >> 15);
        int64_t b7 = (up - b3) * (50000 >> 3);
        p = (b7 * 2) / b4;

        x1 = (p >> 8) * (p >> 8); // better as p*p >> 16, but need 64b
        x1 = (x1 * 3038) >> 16;
        x2 = (-7357 * p) >> 16;
        p = p + ((x1 + x2 + 3791) >> 4);

        if (pfilt == 0)
            pfilt = p;
        else {
            pfilt = pfilt * palpha + (1 - palpha) * p;
        }

        // altitude = 44330 * ( 1 - pow(p/p0, 1/5.255) )
        double p0 = 101325; // sea level in Pa
        double altitude = 44330 * (1 - pow(pfilt/p0, 1/5.255));

        nkern_mutex_lock(&mutex);
        channels[0] = altitude; // meters
        channels[1] = t/10.0;   // deg C  (note scaling in output_task)
        nkern_mutex_unlock(&mutex);

//        kprintf("%10.1f C   %10.1f F %8d Pa, %10.1f, altitude = %10.3f\n", t/10.0, (t/10.0)*9/5+32, p, pfilt, altitude);
    }
}

void pressure_get_data(double *data)
{
    nkern_mutex_lock(&mutex);

    for (int i = 0; i < 2; i++) {
        data[i] = channels[i];
    }

    nkern_mutex_unlock(&mutex);
}

void pressure_init()
{
    nkern_mutex_init(&mutex, "pressure");

    nkern_task_create("pressure",
                      bmp085_task, NULL,
                      NKERN_PRIORITY_NORMAL, 4096);
}

