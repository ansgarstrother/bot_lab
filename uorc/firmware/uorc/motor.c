#include "motor.h"
#include "config.h"
#include "lm3s8962.h"
#include "digio.h"

// Externally, PWM values are 8 bits. Internally, we use 16 bits so that
// we can implement slow slew rates accurately.
static int32_t motor_pwm_goal[3];     // [-255, 255]. Last commanded position.
static int32_t motor_pwm_actual[3];   // 128*current PWM value
static int32_t motor_slew[3];         // 128*(how many pwm values per millisecond)
static uint8_t motor_enable[3];       // [0, 1]

static uint64_t last_command_utime;           // utime of last command
static uint64_t watchdog_timeout_us = (1 * 1000000);

static uint8_t watchdog_state;
static uint8_t watchdog_count;

// make motor channels 1 & 2 identical. user should command the pair via channel 1.
static uint8_t parallel12 = 1;

void motorfault_led(int v)
{
    gpio_set_bit(GPIO_PORTF_DATA_BITS_R, 2, v);
}

uint8_t motor_get_watchdog_state()
{
    return watchdog_state;
}

uint8_t motor_get_watchdog_count()
{
    return watchdog_count;
}

int32_t motor_get_pwm_goal(int port)
{
    return motor_pwm_goal[port];
}

int32_t motor_get_pwm_actual(int port)
{
    return motor_pwm_actual[port];
}

int32_t motor_get_slew(int port)
{
    return motor_slew[port];
}

uint8_t motor_get_enable(int port)
{
    return motor_enable[port];
}

void motor_fault_led_task(void *arg)
{
    while (1) {
        uint32_t gpio = digio_get();

        for (int motor = 0; motor < 3; motor++) {
            int fault = ((gpio>>(8+2*motor))&1)==0 ? 1 : 0;
            if (fault) {
                // if motor is faulting,
                for (int i = 0; i < motor + 1; i++) {
                    motorfault_led(1);
                    nkern_usleep(5000);
                    motorfault_led(0);
                    nkern_usleep(200000);
                }
                nkern_usleep(500000);
            }
        }

        nkern_usleep(500000);
    }
}

// setting watchdog to zero disables the watchdog timer.
void motor_set_watchdog_timeout(int32_t us)
{
    watchdog_timeout_us = us;
}

void motor_set_slew(int port, uint32_t slew)
{
    motor_slew[port] = slew;
}

void motor_set_enable(int port, int enable)
{
    if (enable != motor_enable[port]) {
        // need to do SPI transaction to modify enable state.
        // Motor enables are the only outputs on GPIOA
        motor_enable[port] = enable;

        // copy settings if paralleling
        if (parallel12 && port==1)
            motor_enable[2] = enable;

        mcp23s17_write(MCP23S17_GPIOA,
                       ((1-motor_enable[0])<<1) |
                       ((1-motor_enable[1])<<3) |
                       ((1-motor_enable[2])<<5));

    }
    last_command_utime = nkern_utime();
}

// pwm = [-255, 255]
void motor_set_goal_pwm(int port, int32_t pwm)
{
    motor_pwm_goal[port] = pwm;
    last_command_utime = nkern_utime();
}

// enable = 0 : high-state the outputs
// pwm = [-255, 255]
void motor_set_pwm(int port, int32_t pwm)
{
    // XXX: can't do 100% duty cycle yet...
    switch(port)
    {
    case 0:
        if (pwm >=0) {
            PWM_0_CMPA_R = pwm * (MOTOR_PWM_PERIOD/256);
            PWM_0_CMPB_R = 0;
        } else {
            PWM_0_CMPA_R = 0;
            PWM_0_CMPB_R = (-pwm) * (MOTOR_PWM_PERIOD/256);
        }

        break;

    case 1:
        if (pwm >=0) {
            PWM_1_CMPA_R = pwm * (MOTOR_PWM_PERIOD/256);
            PWM_1_CMPB_R = 0;
        } else {
            PWM_1_CMPA_R = 0;
            PWM_1_CMPB_R = (-pwm) * (MOTOR_PWM_PERIOD/256);
        }

        if (parallel12) {
            if (pwm >=0) {
                PWM_2_CMPA_R = pwm * (MOTOR_PWM_PERIOD/256);
                PWM_2_CMPB_R = 0;
            } else {
                PWM_2_CMPA_R = 0;
                PWM_2_CMPB_R = (-pwm) * (MOTOR_PWM_PERIOD/256);
            }
        }
        break;

    case 2:
        if (!parallel12) {
            if (pwm >=0) {
                PWM_2_CMPA_R = pwm * (MOTOR_PWM_PERIOD/256);
                PWM_2_CMPB_R = 0;
            } else {
                PWM_2_CMPA_R = 0;
                PWM_2_CMPB_R = (-pwm) * (MOTOR_PWM_PERIOD/256);
            }
        }

        break;
    }

}

void motor_task(void *arg)
{
    while (1) {

        int estop_state = digio_get_estop();

        // watchdog timer fired?
        uint8_t this_watchdog_state = (nkern_utime() - last_command_utime > watchdog_timeout_us) &&
            (watchdog_timeout_us > 0);

        // if watchdog timer has elapsed, act as though the estop switch is depressed.
        if (this_watchdog_state)
            estop_state = 1;

        // count positive transitions of watchdog state.
        if (this_watchdog_state && !watchdog_state)
            watchdog_count++;
        watchdog_state = this_watchdog_state;

        for (int i = 0; i < 3; i++) {
            int goal = motor_pwm_goal[i]<<7;

            // If estop is depressed, we ignore the goal PWM, but we
            // still respect slew rate limits.
            if (estop_state)
                goal = 0;

            int actual = motor_pwm_actual[i];

            // Compute a new actual PWM, respecting the slew rate.
            int delta = goal - actual;
            int32_t slew = motor_slew[i];

            // if it's an estop, ensure a reasonable response time
            // even if the user specified a silly slew rate.
            if (estop_state && slew < 0x0080) {
                slew = 0x0080;
            }

            if (delta < -slew)
                delta = -slew;
            if (delta > slew)
                delta = slew;

            motor_pwm_actual[i] += delta;

            motor_set_pwm(i, motor_pwm_actual[i]>>7);
        }

        nkern_usleep(1000);
    }
}

void motor_init()
{
    // PWM clock divisor is disabled, so PWMCLK=SYSCLK (50MHz) (see RCC_R)

    PWM_ENABLE_R |= (1<<0) | (1<<1) | (1<<2) | (1<<3) | (1<<4) | (1<<5);

    // MOT0: PWMA = PF0/PWM0
    //       PWMB = PG1/PWM1

    PWM_0_LOAD_R = MOTOR_PWM_PERIOD;
    PWM_0_CMPA_R = 0x00;
    PWM_0_CMPB_R = 0x00;
    PWM_0_GENA_R = (0x3<<6)  | (0x2<<0); // set to 1 when count=cmpa, set to 0 when count=0
    PWM_0_GENB_R = (0x3<<10) | (0x2<<0); // set to 1 when count=cmpb, set to 0 when count=0

    GPIO_PORTF_DEN_R |= (1<<0);
    GPIO_PORTG_DEN_R |= (1<<1);
    GPIO_PORTF_DIR_R |= (1<<0);
    GPIO_PORTG_DIR_R |= (1<<1);
    GPIO_PORTF_DR2R_R |= (1<<0);
    GPIO_PORTG_DR2R_R |= (1<<1);
    GPIO_PORTF_ODR_R |= (1<<0);
    GPIO_PORTG_ODR_R |= (1<<1);
    GPIO_PORTF_AFSEL_R |= (1<<0);
    GPIO_PORTG_AFSEL_R |= (1<<1);

    // MOT1: PWMA = PB0/PWM2
    //       PWMB = PB1/PWM3
    PWM_1_LOAD_R = MOTOR_PWM_PERIOD;
    PWM_1_CMPA_R = 0x00;
    PWM_1_CMPB_R = 0x00;
    PWM_1_GENA_R = (0x3<<6)  | (0x2<<0); // set to 1 when count=cmpa, set to 0 when count=0
    PWM_1_GENB_R = (0x3<<10) | (0x2<<0); // set to 1 when count=cmpb, set to 0 when count=0

    GPIO_PORTB_DEN_R |= (1<<0) | (1<<1);
    GPIO_PORTB_DIR_R |= (1<<0) | (1<<1);
    GPIO_PORTB_DR2R_R |= (1<<0) | (1<<1);
    GPIO_PORTB_ODR_R |= (1<<0) | (1<<1);
    GPIO_PORTB_AFSEL_R |= (1<<0) | (1<<1);

    // MOT2: PWMA = PE0/PWM4
    //       PWMB = PE1/PWM5

    PWM_2_LOAD_R = MOTOR_PWM_PERIOD;
    PWM_2_CMPA_R = 0x00;
    PWM_2_CMPB_R = 0x00;
    PWM_2_GENA_R = (0x3<<6)  | (0x2<<0); // set to 1 when count=cmpa, set to 0 when count=0
    PWM_2_GENB_R = (0x3<<10) | (0x2<<0); // set to 1 when count=cmpb, set to 0 when count=0

    GPIO_PORTE_DEN_R |= (1<<0) | (1<<1);
    GPIO_PORTE_DIR_R |= (1<<0) | (1<<1);
    GPIO_PORTE_DR2R_R |= (1<<0) | (1<<1);
    GPIO_PORTE_ODR_R |= (1<<0) | (1<<1);
    GPIO_PORTE_AFSEL_R |= (1<<0) | (1<<1);

    if (1) {
        irqstate_t state;
        interrupts_disable(&state);
        PWM_0_CTL_R = (1<<0); // count down, enable. (1<<1 = count up)
        PWM_1_CTL_R = (1<<0); // count down, enable. (1<<1 = count up)
        PWM_2_CTL_R = (1<<0); // count down, enable. (1<<1 = count up)
        interrupts_restore(&state);
    }

    // disable all motors.
    for (int i = 0; i < 3; i++) {
        motor_set_enable(i, 0);
        motor_set_pwm(i, 0);

        // 0x0100 = about 0.5 second from -255 to +255 .
        // larger value = faster transitions, more dangerous.
        motor_slew[i] = 0x0180;
    }

    // reset all motors.
    for (int i = 0; i < 3; i++)
        motor_set_enable(i, 1);
    for (int i = 0; i < 3; i++)
        motor_set_enable(i, 0);

    last_command_utime = nkern_utime();

    nkern_task_create("motor_task",
                      motor_task, NULL,
                      NKERN_PRIORITY_NORMAL, 512);


    nkern_task_create("motor_led_task",
                      motor_fault_led_task, NULL,
                      NKERN_PRIORITY_IDLE, 512);

}
