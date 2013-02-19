#ifndef _ADC_H
#define _ADC_H

void adc_init(uint32_t portmask);
uint32_t adc_read(int port);

#endif
