#ifndef _INTADC_H
#define _INTADC_H

void intadc_init();
void intadc_set_filter_alpha(int port, uint32_t v);
uint32_t intadc_get(int port);
uint32_t intadc_get_filtered(int port);
uint32_t intadc_get_filter_alpha(int port);

#endif
