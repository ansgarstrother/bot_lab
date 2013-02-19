#ifndef _EXTADC_H
#define _EXTADC_H

void extadc_init();
void extadc_set_filter_alpha(int port, uint32_t v);
uint32_t extadc_get(int port);
uint32_t extadc_get_filtered(int port);
uint32_t extadc_get_filter_alpha(int port);
void extadc_get_gyro(int port, uint64_t *integrator, uint32_t *count);

#endif
