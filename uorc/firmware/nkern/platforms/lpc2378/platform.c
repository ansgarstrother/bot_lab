#include <lpc23xx.h>
#include <stdint.h>
#include <nkern.h>

// nkern support functions

static volatile uint64_t utime_base;

static uint32_t last_ctime0;

static uint32_t rtc_ticks;

/*
void rtc_irq_real()
{
    rtc_ticks++;
    FIO2PIN = rtc_ticks&0xff;
    utime_base += 1950;

    _nkern_schedule();

    ILR = ILR; // clear all RTC-related interrupts
    VICVectAddr = 0;
}

uint64_t nkern_utime()
{
    return utime_base;
}
*/
static void rtc_irq_real(void) __attribute__ ((noinline));
static void rtc_irq_real()
{
    rtc_ticks++;
    FIO2PIN = rtc_ticks&0xff;

    // entering a new second?
    uint32_t ctime0 = CTIME0 & 0x3f;
    if (ctime0 != last_ctime0) {
        last_ctime0 = ctime0;
        utime_base += 1000000;
    }

    _nkern_periodic();
    _nkern_schedule();

    ILR = ILR; // clear all RTC-related interrupts
    VICVectAddr = 0;
}

uint64_t nkern_utime()
{
    irqstate_t flags;
    disable_interrupts(&flags);


    // LPC2378 DOCUMENTATION BUG:
    // CTC counts twice as fast as documented, 65 kHz.
    // Fortunately all low 16 bits seem to be valid.
    uint32_t ctc = CTC & 0xffff;
    uint32_t ctime0 = CTIME0 & 0x3f;

    if (ctime0 != last_ctime0) {
        last_ctime0 = ctime0;
        utime_base += 1000000;

        ctc = CTC & 0xffff;
    }

    uint64_t utime = utime_base;

    enable_interrupts(&flags);

    // approximate 1000000/65536 = 15.2587890625 as 15.25
// utime += ctc*15 + ctc/4;
    utime += ctc*15 + (ctc>>2) + (ctc>>7);
    return utime;
}

static void rtc_irq(void) __attribute__ ((naked));
static void rtc_irq(void)
{
    IRQ_TASK_SAVE;
    NKERN_IRQ_ENTER;
    rtc_irq_real();
    NKERN_IRQ_EXIT;
    IRQ_TASK_RESTORE;
}

void nkern_start_periodic_interrupt()
{
    /*
        CISS = 0x80 | 0x00; // every  0.488 ms
        CISS = 0x80 | 0x01; // every  0.977 ms
        CISS = 0x80 | 0x02; // every  1.950 ms
        CISS = 0x80 | 0x03; // every  3.900 ms
        CISS = 0x80 | 0x04; // every  7.800 ms
        CISS = 0x80 | 0x05; // every 15.600 ms
        CISS = 0x80 | 0x06; // every 31.250 ms
        CISS = 0x80 | 0x07; // every 62.500 ms
    */
    PCONP |= (1<<9); // power up the RTC (on by default)
    CCR = 0x11;      // enable RTC, use 32KHz crystal

    AMR = 0xff;      // no alarms, damnit! (this register is undefined at power on!)

    //    CISS = 0x80 | 0x07; // every  62.5 ms
    CISS = 0x80 | 0x02; // every  1.95 ms
    // if you change this, change the utime_count increment
    // in rtc_irq_real

    VICVectAddr13 = (uint32_t) rtc_irq;
    VICIntEnable |= (1<<13);
    VICVectCntl13 = 4;
}

void usleep_spin(int v)
{
    v<<=6;

    for ( ; v ; v--)
        nop();
}
