#include "fast_digio.h"
#include "nkern.h"

static uint32_t fast_digio_mode[8];
static uint32_t servo_usecs[8];

static int servo_port;    // which port are we currently servicing?
static int servo_state;   // 0=signal high phase, 1=signal low phase.

static uint32_t pwm_on_usec[8], pwm_period_usec[8];
static uint8_t pwm_task_started[8]; // has the task for this pin been created?

#define SERVO_SLOT_US 4000

extern void serial0_putc_spin(char c);

static void servo_irq_real(void) __attribute__ ((noinline));
static void servo_irq_real()
{
    TIMER1_CTL_R   &= (~(1<<0)); // disable timer
    TIMER1_ICR_R = (1<<0);       // ack interrupt

    // Configure period of timer to construct the next edge of the
    // servo waveforms.  Period is measured in clock ticks, our clock
    // is 50MHz, hence 50 scale factor.
    if (servo_state==0) {
        // period of "high" output
        TIMER1_TAILR_R  =  50*servo_usecs[servo_port];
    } else {
        // period of "low" output, which is just the remainder of the slot.
        TIMER1_TAILR_R  =  50*(SERVO_SLOT_US - servo_usecs[servo_port]);
    }

    if (fast_digio_mode[servo_port] != FAST_DIGIO_MODE_SERVO)
        goto setup_next;

    switch (servo_port)
    {
    case 0:
        gpio_set_bit(GPIO_PORTD_DATA_BITS_R, 4, 1 - servo_state);
        break;

    case 1:
        gpio_set_bit(GPIO_PORTA_DATA_BITS_R, 6, 1 - servo_state);
        break;

    case 2:
        gpio_set_bit(GPIO_PORTA_DATA_BITS_R, 7, 1 - servo_state);
        break;

    case 3:
        gpio_set_bit(GPIO_PORTB_DATA_BITS_R, 4, 1 - servo_state);
        break;

    case 4:
        gpio_set_bit(GPIO_PORTB_DATA_BITS_R, 5, 1 - servo_state);
        break;

    case 5:
        gpio_set_bit(GPIO_PORTB_DATA_BITS_R, 6, 1 - servo_state);
        break;

    case 6:
        gpio_set_bit(GPIO_PORTC_DATA_BITS_R, 5, 1 - servo_state);
        break;

    case 7:
        gpio_set_bit(GPIO_PORTC_DATA_BITS_R, 7, 1 - servo_state);
        break;
    }

setup_next:

    TIMER1_IMR_R   |= (1<<0);    // enable interrupt
    TIMER1_CTL_R   |= (1<<0);    // enable timer.

    // next state...
    servo_state = 1 - servo_state;
    if (servo_state==0)
        servo_port = (servo_port+1)&7;

}

static void pwm_task(void *arg)
{
    int port = (int) arg;

    int64_t utime = nkern_utime();

    while (1) {
        if (pwm_on_usec[port] > 0) {
            fast_digio_set_digital_out(port, 1);

            utime += pwm_on_usec[port];
            nkern_sleep_until(utime);
        }

        if (pwm_period_usec[port] - pwm_on_usec[port] > 0) {
            fast_digio_set_digital_out(port, 0);

            utime += pwm_period_usec[port] - pwm_on_usec[port];
            nkern_sleep_until(utime);
        }
    }
}

static void set_digital_direction(uint32_t port, uint32_t output)
{
    int pin;

    switch (port)
    {
    case 0:
        pin = 4;
        GPIO_PORTD_DIR_R = (GPIO_PORTD_DIR_R & (~(1<<pin))) | (output << pin);
        break;

    case 1:
        pin = 6;
        GPIO_PORTA_DIR_R = (GPIO_PORTA_DIR_R & (~(1<<pin))) | (output << pin);
        break;

    case 2:
        pin = 7;
        GPIO_PORTA_DIR_R = (GPIO_PORTA_DIR_R & (~(1<<pin))) | (output << pin);
        break;

    case 3:
        pin = 4;
        GPIO_PORTB_DIR_R = (GPIO_PORTB_DIR_R & (~(1<<pin))) | (output << pin);
        break;

    case 4:
        pin = 5;
        GPIO_PORTB_DIR_R = (GPIO_PORTB_DIR_R & (~(1<<pin))) | (output << pin);
        break;

    case 5:
        pin = 6;
        GPIO_PORTB_DIR_R = (GPIO_PORTB_DIR_R & (~(1<<pin))) | (output << pin);
        break;

    case 6:
        pin = 5;
        GPIO_PORTC_DIR_R = (GPIO_PORTC_DIR_R & (~(1<<pin))) | (output << pin);
        break;

    case 7:
        pin = 7;
        GPIO_PORTC_DIR_R = (GPIO_PORTC_DIR_R & (~(1<<pin))) | (output << pin);
        break;
    }
}

static uint32_t get_digital_out(uint32_t port)
{
    switch (port)
    {
    case 0:
        return (GPIO_PORTD_DATA_R >> 4) & 1;

    case 1:
        return (GPIO_PORTA_DATA_R >> 6) & 1;

    case 2:
        return (GPIO_PORTA_DATA_R >> 7) & 1;

    case 3:
        return (GPIO_PORTB_DATA_R >> 4) & 1;

    case 4:
        return (GPIO_PORTB_DATA_R >> 5) & 1;

    case 5:
        return (GPIO_PORTB_DATA_R >> 6) & 1;

    case 6:
        return (GPIO_PORTC_DATA_R >> 5) & 1;

    case 7:
        return (GPIO_PORTC_DATA_R >> 7) & 1;
    }

    return 0;
}

void fast_digio_set_configuration(uint32_t port, uint32_t mode, uint32_t value)
{
    int newmode = (fast_digio_mode[port] != mode);
    fast_digio_mode[port] = mode;

    switch (mode) {
    case FAST_DIGIO_MODE_IN:
        if (newmode)
            set_digital_direction(port, 0);
        break;
    case FAST_DIGIO_MODE_OUT:
        if (newmode)
            set_digital_direction(port, 1);
        fast_digio_set_digital_out(port, value);
        break;
    case FAST_DIGIO_MODE_SERVO:
        if (newmode)
            set_digital_direction(port, 1);
        // prevent illegal value
        if (value >= (SERVO_SLOT_US - 1))
            value = (SERVO_SLOT_US - 1);
        servo_usecs[port] = value;
        break;
    case FAST_DIGIO_MODE_SLOW_PWM:
        if (newmode)
            set_digital_direction(port, 1);

        // Low 20 bits are the period, in us. (maximum period just over 1 second)
        // Upper 12 bits are duty cycle, with 0x000 = 0% and 0xfff = 100%
        pwm_period_usec[port] = (value & 0xfffff);
        uint32_t dutycycle = ((value >> 20) & 0xfff);

        // product should never exceed 1L<<32, so this should be an
        // okay fixed-point division.
        if (dutycycle == 0xfff)
            pwm_on_usec[port] = pwm_period_usec[port]; // make duty cycle exactly 100%
        else
            pwm_on_usec[port] = (pwm_period_usec[port] * dutycycle) >> 12;

        // prevent super-fast (>=1khz) loops that would kill our CPU.
        if (pwm_period_usec[port] < 1000)
            pwm_period_usec[port] = 1000;

        if (!pwm_task_started[port]) {
            pwm_task_started[port] = 1;
            nkern_task_create("pwm_task",
                              pwm_task, (void*) port,
                              NKERN_PRIORITY_NORMAL, 512);
        }

        break;

    default:
        break;
    }

}

void fast_digio_get_configuration(uint32_t port, uint32_t *mode, uint32_t *value)
{
    *mode = fast_digio_mode[port];

    switch (*mode) {
    case FAST_DIGIO_MODE_IN:
        *value = get_digital_out(port);
        break;
    case FAST_DIGIO_MODE_OUT:
        *value = get_digital_out(port);
        break;
    case FAST_DIGIO_MODE_SERVO:
        *value = servo_usecs[port];
        break;
    default:
        *value = 0xffffffff;
        break;
    }

}

static void servo_irq(void) __attribute__ ((interrupt));
static void servo_irq()
{
    servo_irq_real();
}

void servo_set(int servo, uint32_t usec)
{
    // assume aligned word write is atomic, thus interrupt safe
    servo_usecs[servo] = usec;
}

uint32_t servo_get_us(int servo)
{
    return servo_usecs[servo];
}

// we support up to 8 servos on our digital ports. Each has a maximum
// "on" interval of ~4ms, and each is processed in turn. Thus, our
// period is about 32ms.
void fast_digio_init()
{
    for (int i = 0; i < 8; i++) {
        fast_digio_mode[i] = FAST_DIGIO_MODE_IN;
        set_digital_direction(i, 0);
        servo_set(i, 1500);
    }

    nvic_set_handler(37, servo_irq); // timerA irq

    TIMER1_CTL_R &= (~(1<<0)); // disable.
    TIMER1_CFG_R = 0;          // 32 bit timer configuration
    TIMER1_TAMR_R = 0x01;      // one shot.

    TIMER1_TAILR_R = 0x1000;   // period
    TIMER1_IMR_R   |= (1<<0);  // enable interrupt
    TIMER1_CTL_R   |= (1<<0);    // enable timer.
    NVIC_EN0_R     |= (1<<(NVIC_TIMER1A&31));
}

// probably only useful when called by gdb to stop annoying interrupts...
void servo_stop()
{
    TIMER1_IMR_R &= ~(1<<0);
    TIMER1_CTL_R &= ~(1<<0);
    NVIC_EN0_R   &= ~(1<<(NVIC_TIMER1A&31));
}
