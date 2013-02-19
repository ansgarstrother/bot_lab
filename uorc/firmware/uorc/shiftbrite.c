#include <nkern.h>
#include "lm3s8962.h"

#include "mcp23s17.h"
#include "fast_digio.h"

#define SHIFTBRITE_DATA 0
#define SHIFTBRITE_LATCH 1
#define SHIFTBRITE_ENABLE 2
#define SHIFTBRITE_CLOCK 3

#define NUM_RINGS 3
static uint32_t rgb_on[NUM_RINGS][3]; // color for each ring [0, 1023]
static uint32_t rgb_off[NUM_RINGS][3]; // color for each ring [0, 1023]
static uint32_t pwm_period_ms[NUM_RINGS];
static uint32_t pwm_on_ms[NUM_RINGS];
static uint32_t pwm_counter_ms[NUM_RINGS];
static uint32_t num_periods[NUM_RINGS];

static nkern_mutex_t mutex;

// rgb values are [0, 1023] each.
// ring: which ring [0, 2]
// rgb_on: RGB color to use during "on" duty cycle
// rgb_off: RGB color to use during "off" duty cycle (usually [0, 0, 0])
// period_ms: How often does the pattern repeat, in milliseconds?
// on_ms: How long is the light on?
// count: How many times should the pattern repeat? Use 0xffff to repeat forever.
// sync: 0 counter is free-running. 1: counter is set to 0. 2: counter is set to value of ring 0.
void shiftbrite_set(uint32_t ring, uint32_t _rgb_on[3], uint32_t _rgb_off[3], uint32_t period_ms, uint32_t on_ms, uint32_t count, int sync)
{
    nkern_mutex_lock(&mutex);

    if (ring < NUM_RINGS) {
        rgb_on[ring][0] = _rgb_on[0];
        rgb_on[ring][1] = _rgb_on[1];
        rgb_on[ring][2] = _rgb_on[2];
        rgb_off[ring][0] = _rgb_off[0];
        rgb_off[ring][1] = _rgb_off[1];
        rgb_off[ring][2] = _rgb_off[2];
        pwm_period_ms[ring] = period_ms;
        pwm_on_ms[ring] = on_ms;
        num_periods[ring] = count;

        if (sync == 1)
            pwm_counter_ms[ring] = 0; // force reload on next cycle.
        if (sync == 2 && ring != 0)
            pwm_counter_ms[ring] = pwm_counter_ms[0];
    }

    nkern_mutex_unlock(&mutex);
}

void shiftbrite_shift32(uint32_t v)
{
    for (int i = 31; i >= 0; i--) {
        fast_digio_set_digital_out(SHIFTBRITE_DATA, (v>>i)&1);
        fast_digio_set_digital_out(SHIFTBRITE_CLOCK, 1);
        fast_digio_set_digital_out(SHIFTBRITE_CLOCK, 0);
    }
}

void shiftbrite_latch()
{
    fast_digio_set_digital_out(SHIFTBRITE_LATCH, 1);
    fast_digio_set_digital_out(SHIFTBRITE_LATCH, 0);
//    nkern_usleep(LATCH_DELAY);
}

void shiftbrite_task(void *arg)
{
    fast_digio_set_configuration(SHIFTBRITE_DATA, FAST_DIGIO_MODE_OUT, 0);
    fast_digio_set_configuration(SHIFTBRITE_LATCH, FAST_DIGIO_MODE_OUT, 0);
    fast_digio_set_configuration(SHIFTBRITE_ENABLE, FAST_DIGIO_MODE_OUT, 0);
    fast_digio_set_configuration(SHIFTBRITE_CLOCK, FAST_DIGIO_MODE_OUT, 0);

    uint64_t utime = nkern_utime();
    int cnt = 0;

    // it actually takes a surprising amount of time to clock out all
    // these bits.  Thus, we try to only do updates when we need to.
    while (1) {

        int do_update = 0;

        int on[3];

        // does anyone need an update?
        nkern_mutex_lock(&mutex);

        for (int i = 0; i < NUM_RINGS; i++) {

            // roll over?
            if (pwm_counter_ms[i] >= pwm_period_ms[i]) {
                pwm_counter_ms[i] = 0;

                if (num_periods[i] > 0 && num_periods[i] < 0xffff)
                    num_periods[i]--;
            }

            if (pwm_counter_ms[i] == 0)
                do_update = 1;

            if (pwm_counter_ms[i] < pwm_on_ms[i] && num_periods[i] > 0) {
                on[i] = 1;
            } else {
                on[i] = 0;
            }

            // time to turn off?
            if (pwm_counter_ms[i] == pwm_on_ms[i]) {
                do_update = 1;
            }

            pwm_counter_ms[i]++;
        }

        nkern_mutex_unlock(&mutex);

        // periodically update everything just in case.
        if (cnt == 0) {
            // set full power everywhere.
            uint32_t v = (0x7f<<0) + (0x7f<<10) + (0x7f<<20)  + (1<<30);
            for (int i = 0; i < NUM_RINGS*8; i++) {
                shiftbrite_shift32(v);
            }
            shiftbrite_latch();
//            cnt = 0;
            do_update = 1;
        }

        if (do_update) {
            for (int i = 0; i < 8; i++) {
                for (int j = 0; j < NUM_RINGS; j++) {
                    if (on[j])
                        shiftbrite_shift32((rgb_on[j][0]<<0) + (rgb_on[j][1]<<10) + (rgb_on[j][2]<<20));
                    else
                        shiftbrite_shift32((rgb_off[j][0]<<0) + (rgb_off[j][1]<<10) + (rgb_off[j][2]<<20));
                }
            }

            shiftbrite_latch();
        }

        // wait for one millisecond
//        nkern_usleep(1000);
        utime += 1000;
        nkern_sleep_until(utime);
        cnt ++;
    }
}

static int initted = 0;

void shiftbrite_init()
{
    if (initted)
        return;
    initted = 1;

    nkern_mutex_init(&mutex, "led");

    uint32_t rgb_on[3] = { 0, 0, 0 };
    uint32_t rgb_off[3] = { 0, 0, 0 };

    shiftbrite_set(0, rgb_on, rgb_off, 1000, 0, 0, 0);
    shiftbrite_set(1, rgb_on, rgb_off, 1000, 0, 0, 0);
    shiftbrite_set(2, rgb_on, rgb_off, 1000, 0, 0, 0);

    nkern_task_create("shiftbrite",
                      shiftbrite_task, NULL,
                      NKERN_PRIORITY_LOW, 512);
}
