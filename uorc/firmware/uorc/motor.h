#ifndef _MOTOR_H
#define _MOTOR_H

#include <stdint.h>
#include "mcp23s17.h"

void motor_init();

// pwm = [-255, 255]
// resets watchdog timeout.
void motor_set_goal_pwm(int port, int32_t pwm);

// enable = 0 : high-state the outputs
void motor_set_pwm(int port, int32_t pwm);
void motor_set_slew(int port, uint32_t slew);
void motor_set_enable(int port, int enable);
int32_t motor_get_pwm_goal(int port);
int32_t motor_get_pwm_actual(int port);
int32_t motor_get_slew(int port);
uint8_t motor_get_enable(int port);
void motor_set_watchdog_timeout(int32_t us);

uint8_t motor_get_watchdog_state();
uint8_t motor_get_watchdog_count();

#endif
