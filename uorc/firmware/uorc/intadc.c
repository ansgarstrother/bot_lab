#include <nkern.h>
#include "config.h"
#include "lm3s8962.h"

static nkern_wait_list_t intadc_wait_list;

static uint32_t adc_internal[5];              // most recent raw value
static uint32_t adc_internal_filtered[5];     // filtered value, 1st order low pass IIR
static uint32_t adc_internal_filter_alpha[5]; // coefficient of IIR * ADC_FILTER_SCALE

void intadc_irq(void) __attribute__((naked));
void intadc_irq()
{
    IRQ_TASK_SAVE;
    NKERN_IRQ_ENTER;

    _nkern_wake_all(&intadc_wait_list);
    ADC_ISC_R = 1; // clear done flag.

    NKERN_IRQ_EXIT;
    IRQ_TASK_RESTORE;
}

uint32_t intadc_get(int port)
{
    return adc_internal[port];
}

uint32_t intadc_get_filtered(int port)
{
    return adc_internal_filtered[port];
}

uint32_t intadc_get_filter_alpha(int port)
{
    return adc_internal_filter_alpha[port];
}

void intadc_set_filter_alpha(int port, uint32_t v)
{
    adc_internal_filter_alpha[port] = v;
}

void intadc_task(void *arg)
{
    nkern_wait_list_init(&intadc_wait_list, "intadc");
    nvic_set_handler(30, intadc_irq);
    NVIC_EN0_R |= (1<<NVIC_ADC_SEQ0);

    // Analog Initialization: 
    ADC_ACTSS_R = 0; // disable all sequencers during configuration

    ADC_EMUX_R = 0x0;          // set all sequencers to manual sampling
    ADC_SSMUX0_R = (0<<0) | (1<<4) | (2<<8) | (3<<12); // do channels in order
    ADC_SSCTL0_R = (1<<19) | (1<<18) | (1<<17); // sample 5 is the last, and temperature, and interrupt
    ADC_SAC_R = 6;             // over sampling of 2^N
    ADC_IM_R = 1;              // unmask interrupts
    ADC_ACTSS_R = 1;           // enable sequencer 0 (last step of configuration)

    // ADC is 500kHz sample rate. We sample 5 values with 64x
    // oversampling = 320 samples per update. This gives us a maximum
    // sample rate of 1.5khz.

    int64_t utime = nkern_utime();

    while(1) {

        // flush FIFO (avoid error condition)
        while (!(ADC_SSFSTAT0_R & (1<<8))) {
            uint32_t t = ADC_SSFIFO0_R;
        }
            
        // request a conversion using sequencer 0
        ADC_PSSI_R = 1;

        nkern_wait(&intadc_wait_list);

//        utime += INTADC_PERIOD_US;
//        nkern_sleep_until(utime);
/*        nkern_usleep(INTADC_PERIOD_US);

        while (!(ADC_RIS_R & 1)) {
            nkern_yield();
        }

        ADC_ISC_R = 1; // clear done flag.
*/

        uint32_t status = ADC_SSFSTAT0_R;
        uint32_t sz = ((status>>4)&0xf) - (status&0x0f);

        for (int i = 0; i < 5; i++) {
            uint32_t v = (ADC_SSFIFO0_R)&0x3ff;
            nkern_random_entropy_add(v, 1);

            adc_internal[i] = v<<6;  // scale to 16 bits. (10 bit ADC).

            // IIR filter
            adc_internal_filtered[i] = (adc_internal_filtered[i] * adc_internal_filter_alpha[i] +
                                        adc_internal[i] * (ADC_FILTER_SCALE - adc_internal_filter_alpha[i]) +
                                        ADC_FILTER_SCALE/2) >> ADC_FILTER_SCALE_BITS;
        }
    }
}

void intadc_init()
{
    for (int i = 0; i < 5; i++)
        adc_internal_filter_alpha[i] =  (int) (0.98*ADC_FILTER_SCALE);


    nkern_task_create("int_adc",
                      intadc_task, NULL,
                      NKERN_PRIORITY_NORMAL, 1024);

}
