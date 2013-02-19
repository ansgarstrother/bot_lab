#include <nkern.h>
#include "config.h"
#include "lm3s8962.h"
#include "tlc3548.h"

static uint32_t adc_external[8];              // most recent raw value
static uint32_t adc_external_filtered[8];     // filtered value, 1st order low pass IIR 
static uint32_t adc_external_filter_alpha[8]; // coefficient of IIR * ADC_FILTER_SCALE

static uint64_t gyro_integrator[3];
static uint32_t gyro_integrator_count[3];

static nkern_mutex_t gyro_mutex;

void extadc_set_filter_alpha(int port, uint32_t alpha)
{
    adc_external_filter_alpha[port] = alpha;
}

uint32_t extadc_get(int port)
{
    return adc_external[port];
}

uint32_t extadc_get_filtered(int port)
{
    return adc_external_filtered[port];
}

uint32_t extadc_get_filter_alpha(int port)
{
    return adc_external_filter_alpha[port];
}

void extadc_get_gyro(int port, uint64_t *integrator, uint32_t *count)
{
    nkern_mutex_lock(&gyro_mutex);
    *integrator = gyro_integrator[port];
    *count = gyro_integrator_count[port];
    nkern_mutex_unlock(&gyro_mutex);
}

void extadc_task(void *arg)
{
    tlc3548_init();

    uint64_t utime = nkern_utime();

    while (1) {

        for (int i = 0; i < 8; i++) {

            // we're always reading the *last* conversion request.
            int port = (i-1)&7;

            // 14bit ADC, but tlc3548 left-justifies it for us.
            adc_external[port] = tlc3548_command(i<<12); 

            adc_external_filtered[port] = (adc_external_filtered[port] * adc_external_filter_alpha[port] +
                                           adc_external[port] * (ADC_FILTER_SCALE - adc_external_filter_alpha[port]) +
                                           ADC_FILTER_SCALE/2) >> ADC_FILTER_SCALE_BITS;

            if (port < 3) {
                nkern_mutex_lock(&gyro_mutex);

                gyro_integrator[port] += adc_external[port];
                gyro_integrator_count[port] ++;

                nkern_mutex_unlock(&gyro_mutex);
            }

            utime += 500;        // 500us --> 250Hz update rate per channel
//            nkern_sleep_until(utime); 
            nkern_usleep(500);
        }
    }
}

void extadc_init()
{
    nkern_mutex_init(&gyro_mutex, "gyro");

    for (int i = 0; i < 8; i++) {
        adc_external_filter_alpha[i] = (int) (0.95*ADC_FILTER_SCALE);
    }

    nkern_task_create("extadc",
                      extadc_task, NULL,
                      NKERN_PRIORITY_NORMAL, 512);
}

