#ifndef _SHIFTBRITE_H
#define _SHIFTBRITE_H

void shiftbrite_set(uint32_t ring, uint32_t _rgb_on[3], uint32_t _rgb_off[3], uint32_t period_ms, uint32_t on_ms, uint32_t count, int sync);

void shiftbrite_init();

#endif
