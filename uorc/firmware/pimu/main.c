#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include "lm3s8962.h"
#include "luminary.h"
#include <nkern.h>

#include "serial.h"
#include "ethernet.h"
#include "ssi.h"
#include "i2c.h"

#include "config.h"
#include "param.h"
#include "flash.h"
#include "heartbeat.h"

#include "gyros.h"
#include "accel.h"
#include "pressure.h"
#include "magnetometer.h"

/** Our entry point is the function "init" (as specified in
 * cortex-m3/arch. Init will make just a few essential steps (similar
 * to crt0), switch to our desired stack frame, then call init2().
 **/
void init(void);

void init2();

/** User code begins in main_task. **/
void main_task(void *arg);

void watchdog_init();

uint32_t _nkern_stack[512];
extern uint32_t _nkern_stack[], _nkern_stack_top[];

extern uint32_t _heap_end[];
extern uint32_t _edata[], _etext[], _data[];
extern uint32_t __bss_start__[], __bss_end__[];

static void *user_params = ((void*) 0x3f800);

nkern_mutex_t tx_mutex;

void init()
{
    // copy .data section
    for (uint32_t *src = _etext, *dst = _data ; dst != _edata ; src++, dst++)
        (*dst) = (*src);

    // zero .bss section
    for (uint32_t *p = __bss_start__; p != __bss_end__; p++)
        (*p) = 0;

    // sign the kernel stack; this assumes we're NOT called from the kernel stack.
    for (uint32_t *p =  _nkern_stack; p != _nkern_stack_top; p++) {
        *p = NKERN_STACK_MAGIC_NUMBER;
    }

    // Set the process stack to the top of the stack
    asm volatile("msr PSP, %0                      \r\n\t"
                 "msr MSP, %0                      \r\n\t"
                 :: "r" (_nkern_stack_top));

    // we've tampered with the stack pointers which will make local
    // variables unreliable. We'll finish our initialization from a
    // new stack frame.
    init2();
}

void init2() __attribute__((noinline));
void init2()
{
    irqstate_t state;

    // Disabling interrupts is important, so that we can initialize
    // peripherals that might enable IRQs before the scheduler is
    // running.
    interrupts_disable(&state);

    nvic_init();

    SYSCTL_RCGC2_R |= 0x7f;    // enable all GPIO, PORTS A-G
//    SYSCTL_RCGC0_R |= (1<<16); // enable ADC
//    SYSCTL_RCGC1_R |= (1<<8);  // enable QEI clock
//    SYSCTL_RCGC1_R |= (1<<9);  // enable QEI clock
    SYSCTL_RCGC1_R |= (1<<0);  // UART0
    // SYSCTL_RCGC2_R |= (1<<28) | (1<<30); // emac0 and ephy0
    SYSCTL_RCGC1_R |= (1<<4);  // SSI
    SYSCTL_RCGC0_R |= (1<<20); // enable PWM
    SYSCTL_RCGC1_R |= (0xf<<16); // enable TIMER0-TIMER3
    SYSCTL_RCGC1_R |= (1<<12);    // enable i2c

    // We now set all interrupts to very low priority, except those
    // that generate timing-critical waveforms (e.g., the servo task
    // which uses the TIMER1A interrupt.)
    //
    // Note that interrupts_disable doesn't actually disable interrupts.
    // It sets the minimum interrupt service priority to 0xc0. Thus,
    // some interrupts can continue to occur.
    //
    // Obviously, the servo task cannot use any kernel resources,
    // since it may have preempted the kernel and the kernel is not
    // reentrant.
    NVIC_SYS_PRI3_R |= (0xff << 24); // systick

    for (int i = 0; i < 63; i++) {
        nvic_set_priority(i, 0xff);  // set to lowest priority.
    }

    nvic_set_priority(21, 0x80);     // restore servos to very high priority.

    // set up pll
    if (1) {
        // rev a2 errata, bump LDO up to 2.75
        SYSCTL_LDOPCTL_R = SYSCTL_LDOPCTL_2_75V;

        if (CPU_HZ==50000000) {

            SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                           SYSCTL_XTAL_8MHZ);
        } else {
            // 8 MHz?
            SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN |
                           SYSCTL_XTAL_8MHZ);
        }
    }

    ////////////////////////////////////
    // set up pins

    // PWM
    // set up  DIO0-DIO3 (PWM0-PWM3) as output PWM.

    PWM_ENABLE_R |= (1<<0) | (1<<1) | (1<<2) | (1<<3);

    // MOT0: PWMA = PF0/PWM0
    //       PWMB = PG1/PWM1
    PWM_0_CTL_R = (1<<0); // count down, enable. (1<<1 = count up)
    PWM_0_LOAD_R = MOTOR_PWM_PERIOD;
    PWM_0_CMPA_R = 0x00;
    PWM_0_CMPB_R = 0x00;
    PWM_0_GENA_R = (0x3<<6)  | (0x2<<0); // set to 1 when count=cmpa, set to 0 when count=0
    PWM_0_GENB_R = (0x3<<10) | (0x2<<0); // set to 1 when count=cmpb, set to 0 when count=0

    GPIO_PORTF_DEN_R |= (1<<0);
    GPIO_PORTF_DIR_R |= (1<<0);
    GPIO_PORTF_DR2R_R |= (1<<0);
//    GPIO_PORTF_ODR_R |= (1<<0);
//    GPIO_PORTF_AFSEL_R |= (1<<0); // disable PF0 PWM; make it GPIO for camera trigger

    GPIO_PORTG_DEN_R |= (1<<1);
    GPIO_PORTG_DIR_R |= (1<<1);
    GPIO_PORTG_DR2R_R |= (1<<1);
    GPIO_PORTG_ODR_R |= (1<<1);
    GPIO_PORTG_AFSEL_R |= (1<<1);

    // MOT1: PWMA = PB0/PWM2
    //       PWMB = PB1/PWM3
    PWM_1_CTL_R = (1<<0); // count down, enable. (1<<1 = count up)
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

    // CAN:    RX = PD0/CAN0RX
    //         TX = PD1/CAN0TX
    // NOTE: significant errata on LM3S8962 CAN peripheral

    // UART0:  RX = PA0/U0RX
    //         TX = PA1/U0TX
    // handled by serial.c

    // UART1:  RX = PD2/U1RX
    //         TX = PD3/U1TX
    // handled by serial.c

    // I2C:   SCL = PB2/I2C0SCL
    //        SDA = PB3/I2C0SDA
    GPIO_PORTB_AFSEL_R |= (1<<2) | (1<<3);
    GPIO_PORTB_DEN_R   |= (1<<2) | (1<<3);
    GPIO_PORTB_ODR_R   |= (1<<2) | (1<<3); // Errata 6.1: configure ODR last.
    // GPIO_PORTB_PUR_R   |= (1<<2) | (1<<3); // doesn't work. we need external pull-ups.

    // SPI:   SCK = PA2/SSI0CLK
    //       MISO = PA4/SSI0RX
    //       MOSI = PA5/SSI0TX
    //   SS_ACCEL = PD5
    //     SS_ADC = PD6/FAULT
    //    SS_USER = PD7/IDX0
    GPIO_PORTA_AFSEL_R |= (1<<2) | (1<<4) | (1<<5);
    GPIO_PORTA_DATA_R  &= (~((1<<2) | (1<<5)));
    GPIO_PORTA_DIR_R   |= (1<<2) | (1<<5);
    GPIO_PORTA_DEN_R   |= (1<<2) | (1<<4) | (1<<5);
    GPIO_PORTA_DR2R_R  |= (1<<2) | (1<<5);
    GPIO_PORTA_PUR_R   |= (1<<4);

    GPIO_PORTD_DATA_R |= (1<<5) | (1<<6) | (1<<7); // make all SS inactive
    GPIO_PORTD_DIR_R  |= (1<<5) | (1<<6) | (1<<7); // \CS digital outputs
    GPIO_PORTD_DEN_R  |= (1<<5) | (1<<6) | (1<<7);
    GPIO_PORTD_DR2R_R |= (1<<5) | (1<<6) | (1<<7);

    // LED_STATUS: PA3/SSI0FSS
    GPIO_PORTA_DIR_R |= (1<<3);  // high current digital output, slow slew
    GPIO_PORTA_DEN_R |= (1<<3);
    GPIO_PORTA_DR8R_R |= (1<<3);
    GPIO_PORTA_SLR_R |= (1<<3);
    GPIO_PORTA_DATA_R &= ~(1<<3); // off for now.

    // LED_DEBUG: PF1/IDX1
    GPIO_PORTF_DIR_R |= (1<<1);  // high current digital output, slow slew
    GPIO_PORTF_DEN_R |= (1<<1);
    GPIO_PORTF_DR8R_R |= (1<<1);
    GPIO_PORTF_SLR_R |= (1<<1);
    GPIO_PORTF_DATA_R &= ~(1<<1); // off for now.

    // Configure these pins as digital inputs with pull ups.

    GPIO_PORTD_DEN_R  |= (1<<4); // DIO5
    GPIO_PORTD_DR2R_R |= (1<<4);
    GPIO_PORTD_PUR_R  |= (1<<4);

    GPIO_PORTA_DEN_R  |= (1<<6); // DIO4
    GPIO_PORTA_DR2R_R |= (1<<6);
    GPIO_PORTA_PUR_R  |= (1<<6);

    GPIO_PORTB_DEN_R  |= (1<<4) | (1<<5) | (1<<6); // PRESSURE_EOC, PRESSURE_XCLR, MAGNETOMETER_DRDY
    GPIO_PORTB_DR2R_R |= (1<<4) | (1<<5) | (1<<6);
    GPIO_PORTB_PUR_R |= (1<<4) | (1<<5) | (1<<6);

    GPIO_PORTC_DEN_R  |= (1<<5) | (1<<7); // GYROA_ST, GYROB_ST  (XXX should be output)
    GPIO_PORTC_DR2R_R |= (1<<5) | (1<<7);
    GPIO_PORTC_PUR_R  |= (1<<5) | (1<<7);

    GPIO_PORTG_DEN_R  |= (1<<0); // ACCEL_INT
    GPIO_PORTG_DR2R_R |= (1<<0);
    GPIO_PORTG_PUR_R  |= (1<<0);

    /// initialize brown-out reset (BOR)
    SYSCTL_PBORCTL_R |= SYSCTL_PBORCTL_BORIOR;

    // Must do basic kernel initialization before calling device
    // driver inits, since they will allocate/initialize wait lists.
    nkern_init();

    nkern_task_create("main",
                      main_task, NULL,
                      NKERN_PRIORITY_NORMAL, 4096);

    // once we start the kernel, main_task will begin executing.
    nkern_start();
    while(1);
}

///////////////////////////////////////////////////////////////////////////
iop_t *serial0_iop; //, *serial1_iop;

void encode16(uint8_t *buf, uint32_t v)
{
    buf[0] = (v>>8)&0xff;
    buf[1] = (v>>0)&0xff;
}

void encode32(uint8_t *buf, uint32_t v)
{
    buf[0] = (v>>24)&0xff;
    buf[1] = (v>>16)&0xff;
    buf[2] = (v>>8)&0xff;
    buf[3] = v&0xff;
}

void encode64(uint8_t *buf, uint64_t v)
{
    buf[0] = (v>>56)&0xff;
    buf[1] = (v>>48)&0xff;
    buf[2] = (v>>40)&0xff;
    buf[3] = (v>>32)&0xff;
    buf[4] = (v>>24)&0xff;
    buf[5] = (v>>16)&0xff;
    buf[6] = (v>>8)&0xff;
    buf[7] = (v>>0)&0xff;
}

static inline int32_t checksum_init()
{
    return 0x12345678;
}

static inline int32_t checksum_update(int32_t chk, int8_t c)
{
    chk = chk + (c & 0xff);
    chk = (chk << 1) ^ (chk >> 23);

    return chk;
}

static inline int8_t checksum_finish(int32_t chk)
{
    return (chk & 0xff) ^ ((chk >> 8) & 0xff) ^ ((chk >> 16) & 0xff);
}

// packet format:
//
// 4 byte magic sequence. (Note: different for RX and TX cases!)
// 2 byte length
// <payload of length bytes>
//    offset 0: command code
//    offset 1: response/status code
//    ... : command specific
// 1 byte checksum

void write_packet(const void *_buf, int len)
{
    uint8_t *buf = (uint8_t*) _buf;

    nkern_mutex_lock(&tx_mutex);

    // magic header
    serial0_putc(0xed);
    serial0_putc(0x87);
    serial0_putc(0x71);
    serial0_putc(0xe2);

    // all subsequent bytes are checksummed.
    int32_t chk = checksum_init();

    // output payload length
    serial0_putc(len>>8);
    chk = checksum_update(chk, len >> 8);
    serial0_putc(len & 0xff);
    chk = checksum_update(chk, len & 0xff);

    // output payload
    for (int i = 0; i < len; i++) {
        chk = checksum_update(chk, buf[i]);
        serial0_putc(buf[i]);
    }

    // output checksum
    chk = checksum_finish(chk);
    serial0_putc(chk);

    nkern_mutex_unlock(&tx_mutex);
}

// read a packet, returning just the payload. Returns negative on error.
int read_packet(char *buf, int maxlen)
{
    // synchronize to magic string.
    while (1) {
        uint8_t b;

        b = serial0_getc();
        if (b != 0xed)
            continue;

        b = serial0_getc();
        if (b != 0x87)
            continue;

        b = serial0_getc();
        if (b != 0x17)
            continue;

        b = serial0_getc();
        if (b != 0x9a)
            continue;
        break;
    }

    int chk = checksum_init();

    int len = (serial0_getc()<<8) + (serial0_getc());
    chk = checksum_update(chk, len>>8);
    chk = checksum_update(chk, len & 0xff);

    if (len > maxlen)
        return -1;

    for (int i = 0; i < len; i++) {
        buf[i] = serial0_getc();
        chk = checksum_update(chk, buf[i]);
    }

    chk = checksum_finish(chk);

    int8_t readchk = serial0_getc();
    if (readchk != chk)
        return -2;

    return len;
}

void output_task(void *arg)
{
    uint32_t adc[9];
    double   alttemp[2];
    uint32_t accel[3], magnetometer[3];

    while (1) {
        uint64_t time = nkern_utime();

        accel_get_data(accel);
        pressure_get_data(alttemp);
        gyros_get_data(adc);
        magnetometer_get_data(magnetometer);

        if (1) {
            uint8_t buf[1024];
            int pos = 0;

            buf[pos] = 0x01; pos++; // data message
            buf[pos] = 0x00; pos++; // normal status-- no error

            // message
            encode64(&buf[pos], time); pos+=8;

            encode32(&buf[pos], adc[0]); pos+=4; // integrator values
            encode32(&buf[pos], adc[1]); pos+=4;
            encode32(&buf[pos], adc[2]); pos+=4;
            encode32(&buf[pos], adc[3]); pos+=4;
            encode32(&buf[pos], adc[4]); pos+=4;
            encode32(&buf[pos], adc[5]); pos+=4;
            encode32(&buf[pos], adc[6]); pos+=4;
            encode32(&buf[pos], adc[7]); pos+=4;

            encode16(&buf[pos], accel[0]); pos+=2;
            encode16(&buf[pos], accel[1]); pos+=2;
            encode16(&buf[pos], accel[2]); pos+=2;

            encode16(&buf[pos], magnetometer[0]); pos+=2;
            encode16(&buf[pos], magnetometer[1]); pos+=2;
            encode16(&buf[pos], magnetometer[2]); pos+=2;

            encode32(&buf[pos], (uint32_t) (alttemp[0]*100)); pos+=4;  // units: 1cm
            encode32(&buf[pos], (uint32_t) (alttemp[1]*100)); pos+=4;   // units: 0.01 deg C

            write_packet(buf, pos);
        }

        nkern_usleep(20000);
    }
}

char debug_buffer[128];
int debug_buffer_pos = 2;
iop_t debug_iop;

int debug_write(iop_t *iop, const void *_buf, uint32_t count)
{
    char *buf = (char*) _buf;

    for (uint32_t i = 0; i < count; i++) {
        char c = buf[i];
        debug_buffer[debug_buffer_pos++] = c;

        if (c == '\n' || c == '\r' || debug_buffer_pos == 128) {
            debug_buffer[0] = 0xff; // command code
            debug_buffer[1] = 0x00; // status code
            write_packet(debug_buffer, debug_buffer_pos);
            debug_buffer_pos = 2;
        }
    }

    return count;
}

void trigger_set(int v)
{
    gpio_set_bit(GPIO_PORTF_DATA_BITS_R, 0, v);
}

void trigger_task(void *arg)
{
    uint64_t time = nkern_utime();

    uint64_t delay = 1000000 / 30;

    while (1) {
        time += delay;

        nkern_sleep_until(time);

        // pulse
        gpio_set_bit(GPIO_PORTF_DATA_BITS_R, 0, 0);
        nkern_usleep(1000);
        gpio_set_bit(GPIO_PORTF_DATA_BITS_R, 0, 1);

    }
}

void main_task(void *arg)
{
    serial0_iop = serial0_configure(CPU_HZ, 230400);

    nkern_mutex_init(&tx_mutex, "tx_mutex");
    memset(&debug_iop, 0, sizeof(debug_iop));
    debug_iop.write = debug_write;

    nkern_kprintf_init(&debug_iop);

    // send a bunch of bytes so that the receiver's last packet will
    // be finished.  this helps synchronize us with the client.
    for (int i = 0; i < 256; i++)
        serial0_putc(' ');

    kprintf("reset\n");

    // retrieve flash parameters.
    param_init();

    heartbeat_init();
    watchdog_init();

    ssi_init();
    i2c_init();

    pressure_init(); // slowest to initialize
    gyros_init();
    accel_init();
    magnetometer_init();

    nkern_usleep(100000); // wait for initialization

    nkern_task_create("output",
                      output_task, NULL,
                      NKERN_PRIORITY_NORMAL, 4096); // big stack for math functions

    nkern_task_create("trigger",
                      trigger_task, NULL,
                      NKERN_PRIORITY_HIGH, 1024);

    while (1) {
        char buf[512];

        int len = read_packet(buf, 512);
        if (len < 0) {
            kprintf("bad packet %d\n", len);
            continue;
        }

        // response type code = request type code + 1 (success)
        // response type code = request type code + 2 (failure)

        // read flash parameters
        if (buf[0] == 10) {
            buf[1] = 0; // no error
            memcpy(&buf[2], flash_params, FLASH_PARAM_LENGTH);

            write_packet(buf, 2 + FLASH_PARAM_LENGTH);
        }

        // write flash parameters and reboot
        if (buf[0] == 20) {
            if (len == (2 + FLASH_PARAM_LENGTH)) {
                flash_erase_and_write((uint32_t) flash_params, &buf[2], FLASH_PARAM_LENGTH);

                // send ack.
                buf[1] = 0; // no error
                write_packet(buf, 2);

                // force reboot by locking up. (watchdog timer will kick in).
                irqstate_t state;
                interrupts_disable(&state);
                while (1);

            } else {
                kprintf("invalid flash write length\n");
                buf[1] = 1; // error!
                write_packet(buf, 2);
            }
        }

        // spew status information
        if (buf[0] == 30) {
            buf[1] = 0; // no error
            write_packet(buf, 2);
            nkern_print_stats(&debug_iop);
        }
    }
}
