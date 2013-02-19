#include "lm3s8962.h"
#include "nkern.h"

#define WATCHDOG0_BASE          0x40000000L

#define WDT0_LOAD          (*((volatile unsigned long *) (WATCHDOG0_BASE + 0x0000)))
#define WDT0_CTL           (*((volatile unsigned long *) (WATCHDOG0_BASE + 0x0008)))
#define WDT0_LOCK          (*((volatile unsigned long *) (WATCHDOG0_BASE + 0x0C00)))

// locking doesn't seem to work.

static void watchdog_tickle()
{
//    WDT0_LOCK = 0x1acce551; // unlock
    WDT0_LOAD = 20000000; // measured in sysclk ticks (50MHz). (400ms)
//    WDT0_LOCK = 0;        // relock.
}

static void watchdog_task(void *arg)
{
    while (1) {
        watchdog_tickle();

        nkern_usleep(100000); // 100ms
    }
}

void watchdog_init()
{
    // power up watch dog
    SYSCTL_RCGC0_R |= SYSCTL_RCGC0_WDT;

    watchdog_tickle();

    WDT0_CTL |= WDT_CTL_RESEN; // enable reset on WDT timeout (inten flag starts counter?)
    WDT0_CTL |= WDT_CTL_INTEN; // enable counter (must be called last)

//    WDT0_LOCK = 0; // lock out stray resets.

    nkern_task_create("watchdog",
                      watchdog_task, NULL,
                      NKERN_PRIORITY_NORMAL, 512);
}

void watchdog_disable()
{
    WDT0_CTL &= (~WDT_CTL_RESEN);
    WDT0_CTL &= (~WDT_CTL_INTEN);
}
