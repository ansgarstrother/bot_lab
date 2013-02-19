#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#include "lm3s8962.h"
#include "luminary.h"
#include <nkern.h>

#include "serial.h"
#include "ethernet.h"
#include "ssi.h"
#include "mcp23s17.h"
#include "tlc3548.h"
#include "i2c.h"

#include "intadc.h"
#include "extadc.h"
#include "config.h"
#include "param.h"
#include "flash.h"
#include "fast_digio.h"
#include "motor.h"
#include "digio.h"
#include "discover.h"
#include "qei.h"
#include "command.h"
#include "heartbeat.h"
#include "dhcpd.h"
#include "shell.h"
#include "watchdog.h"

/** Our entry point is the function "init" (as specified in
 * cortex-m3/arch. Init will make just a few essential steps (similar
 * to crt0), switch to our desired stack frame, then call init2().
 **/
void init(void);

void init2();

/** User code begins in main_task. **/
void main_task(void *arg);

uint32_t _nkern_stack[512];
extern uint32_t _nkern_stack[], _nkern_stack_top[];

extern uint32_t _heap_end[];
extern uint32_t _edata[], _etext[], _data[];
extern uint32_t __bss_start__[], __bss_end__[];

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
    SYSCTL_RCGC0_R |= (1<<16); // enable ADC
    SYSCTL_RCGC1_R |= (1<<8);  // enable QEI clock
    SYSCTL_RCGC1_R |= (1<<9);  // enable QEI clock
    SYSCTL_RCGC1_R |= (1<<0);  // UART0
    SYSCTL_RCGC2_R |= (1<<28) | (1<<30); // emac0 and ephy0
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

    // QEI0:    A = PC4/PHA0
    //          B = PC6/PHB0
    //QEI0_CTL_R = (1<<0) | (1<<3) | (1<<5); // full resolution mode (see errata)
    QEI0_CTL_R = (1<<0) | (0<<3) | (1<<5); // half resolution mode (no errata)

    QEI0_MAXPOS_R = 0xffffffff;      // maximum position before wrap-around
    QEI0_LOAD_R = CPU_HZ / QEI_VELOCITY_SAMPLE_HZ;  // velocity sampling interval

    GPIO_PORTC_AFSEL_R  |= (1<<4) | (1<<6);        // alternate function with pull-ups
    GPIO_PORTC_ODR_R    &= ~((1<<4) | (1<<6));
    GPIO_PORTC_DEN_R    |= (1<<4) | (1<<6);
    GPIO_PORTC_PUR_R    |= (1<<4) | (1<<6);

    // QEI1:    A = PE3/PHA1
    //          B = PE2/PHB1
    //QEI1_CTL_R = (1<<0) | (1<<3) | (1<<5); // full resolution mode (see errata)
    QEI1_CTL_R = (1<<0) | (0<<3) | (1<<5); // half resolution mode (no errata)
    QEI1_MAXPOS_R = 0xffffffff;      // maximum position before wrap-around
    QEI1_LOAD_R = CPU_HZ / QEI_VELOCITY_SAMPLE_HZ;  // velocity sampling interval

    GPIO_PORTE_AFSEL_R  |= (1<<2) | (1<<3);        // alternate function with pull-ups
    GPIO_PORTE_ODR_R    &= ~((1<<2) | (1<<3));
    GPIO_PORTE_DEN_R    |= (1<<2) | (1<<3);
    GPIO_PORTE_PUR_R    |= (1<<2) | (1<<3);

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
    // Try built-in pullups to get rid of our external 3ks?
    GPIO_PORTB_AFSEL_R |= (1<<2) | (1<<3);
    GPIO_PORTB_ODR_R   |= (1<<2) | (1<<3);
    GPIO_PORTB_DEN_R   |= (1<<2) | (1<<3);

    // SPI:   SCK = PA2/SSI0CLK
    //       MISO = PA4/SSI0RX
    //       MOSI = PA5/SSI0TX
    //    GPIO_SS = PD5
    //     ADC_SS = PD6/FAULT
    //    USER_SS = PD7/IDX0
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

    // LED_HEARTBEAT: PA3/SSI0FSS
    GPIO_PORTA_DIR_R |= (1<<3);  // high current digital output, slow slew
    GPIO_PORTA_DEN_R |= (1<<3);
    GPIO_PORTA_DR8R_R |= (1<<3);
    GPIO_PORTA_SLR_R |= (1<<3);
    GPIO_PORTA_DATA_R &= ~(1<<3); // off for now.

    // LED_MOTORFAULT: PF1/IDX1
    GPIO_PORTF_DIR_R |= (1<<1);  // high current digital output, slow slew
    GPIO_PORTF_DEN_R |= (1<<1);
    GPIO_PORTF_DR8R_R |= (1<<1);
    GPIO_PORTF_SLR_R |= (1<<1);
    GPIO_PORTF_DATA_R &= ~(1<<1); // off for now.

    // GPIO_INT  = PG0

    // GPIO:   0 = PD4/CCP0
    //         1 = PA6/CCP1
    //         2 = PA7
    //         3 = PB4/C0-
    //         4 = PB5/COo
    //         5 = PB6/C0+
    //         6 = PC5
    //         7 = PC7
    // Configure these pins as digital inputs with pull ups.

    GPIO_PORTD_DEN_R  |= (1<<4);
//    GPIO_PORTD_DIR_R  |= (1<<4);
    GPIO_PORTD_DR2R_R |= (1<<4);
    GPIO_PORTD_PUR_R  |= (1<<4);

    GPIO_PORTA_DEN_R  |= (1<<6) | (1<<7);
//    GPIO_PORTA_DIR_R  |= (1<<6) | (1<<7);
    GPIO_PORTA_DR2R_R |= (1<<6) | (1<<7);
    GPIO_PORTA_PUR_R  |= (1<<6) | (1<<7);

    GPIO_PORTB_DEN_R  |= (1<<4) | (1<<5) | (1<<6);
//    GPIO_PORTB_DIR_R  |= (1<<4) | (1<<5) | (1<<6);
    GPIO_PORTB_DR2R_R |= (1<<4) | (1<<5) | (1<<6);
    GPIO_PORTB_PUR_R |= (1<<4) | (1<<5) | (1<<6);

    GPIO_PORTC_DEN_R  |= (1<<5) | (1<<7);
//    GPIO_PORTC_DIR_R  |= (1<<5) | (1<<7);
    GPIO_PORTC_DR2R_R |= (1<<5) | (1<<7);
    GPIO_PORTC_PUR_R  |= (1<<5) | (1<<7);

    /// initialize brown-out reset (BOR)
    SYSCTL_PBORCTL_R |= SYSCTL_PBORCTL_BORIOR;

    // Must do basic kernel initialization before calling device
    // driver inits, since they will allocate/initialize wait lists.
    nkern_init();

    nkern_task_create("main",
                      main_task, NULL,
                      NKERN_PRIORITY_NORMAL, 1536);

    // once we start the kernel, main_task will begin executing.
    nkern_start();
    while(1);
}

// toUpper() echo service.
void tcp_task(void *arg)
{
    tcp_listener_t *tcplistener = tcp_listen(24);

    while (1) {

        tcp_connection_t *conn = tcp_accept(tcplistener);
        kprintf("TCP_TEST: Accepted connection\n");

        while (1) {
            int c = tcp_getc(conn);
            if (c < 0)
                break;

            if (c >= 'a' && c <= 'z')
                c += 'A' - 'a';

            int res = tcp_putc(conn, c);
            if (res < 0)
                break;

            if (c=='\n')
                tcp_flush(conn);
        }

        kprintf("TCP_TEST: Closing\n");
        tcp_close(conn);
        tcp_free(conn);
    }
}

void tcp_torture_task(void *arg)
{
    tcp_listener_t *tcplistener = tcp_listen(53771);

    while (1) {

        tcp_connection_t *conn = tcp_accept(tcplistener);
        kprintf("TCP_TORTURE: Accepted connection\n");

        while (1) {
            int len0 = tcp_getc(conn);
            if (len0 < 0)
                goto disconnect;
            int len1 = tcp_getc(conn);
            if (len1 < 0)
                goto disconnect;

            int len = len0 + (len1<<8);

            char firstchar = -1;

            for (int i = 0; i < len; i++) {
                char c = tcp_getc(conn);
                int res = tcp_putc(conn, c);
                if (res < 0)
                    goto disconnect;

                if (i == 0)
                    firstchar = c;
                else {
                    if (c != firstchar) {
//                        kprintf("torture rx error\n");
                    }
                }
            }

            int res = tcp_flush(conn);
            if (res < 0)
                goto disconnect;
        }

    disconnect:
        kprintf("TCP_TEST: Closing\n");
        tcp_close(conn);
        tcp_free(conn);
    }
}

iop_t *serial0_iop, *serial1_iop;

void sound_task(void *arg)
{
  fast_digio_set_configuration(0, FAST_DIGIO_MODE_OUT, 1);
  GPIO_PORTD_DR8R_R |= (1<<4);

  if (1) {
    GPIO_PORTD_AFSEL_R |= (1<<4);

    TIMER0_CTL_R &= (~1); // disable timer

    /*    int prescale = 20;

    TIMER0_TAPR_R = prescale;
    TIMER0_TAPMR_R = prescale;
    */

    TIMER0_CFG_R = 0x04;
    TIMER0_TAMR_R = (1<<3) | (0x02<<0); // PWM16 mode

    uint32_t period = 50000000 / 4000;
    TIMER0_TAILR_R = period;
    TIMER0_TAMATCHR_R = period/2;

    TIMER0_CTL_R |= 1; // enable timer

  } else {

    fast_digio_set_configuration(0, FAST_DIGIO_MODE_OUT, 1);

    uint32_t period = 1000000 / 440;

    while(1) {
      nkern_usleep(period/2);
      gpio_set_bit(GPIO_PORTD_DATA_BITS_R, 4, 0);
      nkern_usleep(period/2);
      gpio_set_bit(GPIO_PORTD_DATA_BITS_R, 4, 1);
    }
  }
}

void main_task(void *arg)
{
    // initialize serial0 first: it's our first line of debugging
    serial0_iop = serial0_configure(CPU_HZ, 115200);
    serial1_iop = serial1_configure_halfduplex(CPU_HZ, 1000000);

    nkern_kprintf_init(serial0_iop);
    kprintf("\n\nuOrc RESET\n");

    // retrieve flash parameters. We'll need these for ethernet initialization.
    param_init();

    net_init();
    net_dev_t *netdev = ethernet_init(CPU_HZ, flash_params->macaddr);

    // subnet (must be listed first, so it will have higher routing
    // precedence than the default route.)
    ip_config_add(netdev,
                  flash_params->ipaddr,  // our IP address
                  flash_params->ipmask,  // match all hosts on the same subnet
                  inet_addr("0.0.0.0")); // we're on the same subnet, address directly to recipient

    // default route
    ip_config_add(netdev,
                  flash_params->ipaddr,  // our IP address
                  inet_addr("0.0.0.0"),  // match all hosts everywhere
                  flash_params->ipgw);   // route via gateway.

    heartbeat_init();

    watchdog_init();

    ssi_init();
    qei_init();
    motor_init();

    fast_digio_init();
    i2c_init();

    digio_init();
    intadc_init();
    extadc_init();
    command_init();

    shell_init();

    discover_init();

    // Profiler doesn't work right yet...
    //    profile_init();

    if (flash_params->dhcpd_enable)
        dhcpd_init();

    while (1) {
        char c = getc_iop(serial0_iop);
        nkern_print_stats(serial0_iop);
        pprintf(serial0_iop, "\n");
        udp_print_stats(serial0_iop);
        pprintf(serial0_iop, "\n");
        tcp_print_stats(serial0_iop);
    }
}
