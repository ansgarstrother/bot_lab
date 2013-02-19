#ifndef _NKERN_H
#define _NKERN_H

#include <stdint.h>
#include <stdarg.h>
#include <reent.h>
#include <stdlib.h>
#include <arch.h>

typedef struct nkern_task nkern_task_t;

/* Tasks belong either in a wait queue (waiting for an event), the
 * sleep queue (waiting for a particular time), or in a run queue
 * (ready to run).
 */
typedef struct nkern_wait_list nkern_wait_list_t;
typedef struct nkern_wait nkern_wait_t;

/** Driver design

    Drivers will typically maintain waitlists for each type of event
    they handle. For example, a serial driver will maintain a TX and
    RX waitlist. Tasks that are waiting for TX will be added to the TX
    waitlist, and the TX IRQ handler will wake up all the tasks on the
    wait list.

    In addition, drivers should provide non-blocking read/write
    calls. Suppose an application is attempting to do non-blocking
    read on a serial port (and is using the select-like
    "nkern_wait_many" method). It is possible that a character is
    received and wakes the applications, but that another thread reads
    the serial port first. If only a blocking read is available, the
    thread may end up blocking.

 Non-blocking IO

    Non-blocking I/O is similar to the select/poll calls in POSIX,
    except that each event (read, write) must be setup separately. To
    do this, the application pre-allocates an array of nkern_wait_t
    records and inform drivers of their interest in their events using
    the driver's appropriate *_wait_setup function. The driver's
    wait_setup function will call nkern_init_wait_event, which adds
    the task to the driver's wait list, but without putting the task
    to sleep. The driver can immediately wake up the task with
    nkern_wake_one/all, if there is no reason to sleep (e.g., the
    serial port can be read without blocking). Otherwise, the driver
    can wake up waiting tasks in response to an IRQ.

    Once all of the nkern_wait records have been initialized by the
    application, the application calls nkern_wake_many_until, which
    puts the task into the sleep state and yields. The scheduler will
    check to see if nkern_wake_one/all has already been called for
    that task, in which case the task is immediately marked
    runnable. Otherwise, the task goes to sleep.

**/

/** A structure representing one way to wake up from a wait
 * condition. For example, a select call on two different devices
 * would wait on an array of two nkern_waits. (Note that timeouts are
 * handled as a special case.) **/
struct nkern_wait
{
    nkern_task_t      *task; // which task requested this wait?
    nkern_wait_list_t *list; // what wait list was this task added to?
    uint32_t           occurred; // has the event occurred?

    nkern_wait_t      *next;  // managed by nkern_wait_list to form linked list
};

struct nkern_wait_list
{
    nkern_wait_t *head;
    nkern_wait_t *tail;
    const char *name;
};

typedef struct nkern_periodic_task nkern_periodic_task_t;
struct nkern_periodic_task
{
    const char            *name;
    int32_t               (*proc)(void*);
    void                  *arg;
    int64_t               uperiod;

    int64_t               next_utime;
    nkern_periodic_task_t *next;
};

typedef struct nkern_task_list nkern_task_list_t;

struct nkern_task_list
{
    nkern_task_t *head;
    nkern_task_t *tail;
    const char *name;
    nkern_task_list_t *next;
};


typedef struct iop iop_t;
struct iop
{
    void *user;

    int (*read)(iop_t *iop, void *buf, uint32_t count); // read up to 'count' bytes
    int (*write)(iop_t *iop, const void *buf, uint32_t count); // fully write count bytes.
    int (*flush)(iop_t *iop); // flush write buffer.
};

// Functions beginning with an underscore should only be called from
// an IRQ-disabled state (i.e., IRQ handlers).
void nkern_wait_list_init(nkern_wait_list_t *l, const char *name);
void _nkern_wait_list_append(nkern_wait_list_t *l, nkern_wait_t *n);
void _nkern_wait_list_prepend(nkern_wait_list_t *l, nkern_wait_t *n);
nkern_wait_t *_nkern_wait_list_remove_first(nkern_wait_list_t *l);
void _nkern_wait_list_insert_ascending_waketime(nkern_wait_list_t *l, nkern_wait_t *n);
nkern_wait_t *_nkern_wait_list_remove_first_if_sleep_done(nkern_wait_list_t *l, uint64_t time);
uint32_t _nkern_wait_list_is_empty(nkern_wait_list_t *l);
int _nkern_wait_list_remove(nkern_wait_list_t *l, nkern_wait_t *n);

void nkern_task_list_init(nkern_task_list_t *l, const char *name);
void _nkern_task_list_append(nkern_task_list_t *l, nkern_task_t *n);
void _nkern_task_list_prepend(nkern_task_list_t *l, nkern_task_t *n);
nkern_task_t *_nkern_task_list_remove_first(nkern_task_list_t *l);
void _nkern_task_list_insert_ascending_waketime(nkern_task_list_t *l, nkern_task_t *n);
nkern_task_t *_nkern_task_list_remove_first_if_sleep_done(nkern_task_list_t *l, uint64_t time);
uint32_t _nkern_task_list_is_empty(nkern_task_list_t *l);
int _nkern_task_list_remove(nkern_task_list_t *l, nkern_task_t *n);

#include <nkern_sync.h>
#include <nkern_internal.h>

/////////////////////////////////////////////////////////
// Priorities
/////////////////////////////////////////////////////////
#define NKERN_NPRIORITIES        6

#define NKERN_PRIORITY_CRITICAL  5
#define NKERN_PRIORITY_HIGH      4
#define NKERN_PRIORITY_NORMAL    3
#define NKERN_PRIORITY_LOW       2
#define NKERN_PRIORITY_IDLE      1
#define NKERN_PRIORITY_SYS_IDLE  0  /* the kernel idle thread runs at this priority */


/** Call before any other calls to kernel functions. **/
void nkern_init();

/** Begin the scheduler **/
void nkern_start();

/** Exit the current thread. **/
void nkern_exit();

/** Yield to another thread. **/
void nkern_yield();

nkern_task_t *nkern_task_create(const char *name,
                                void (*proc)(void*), void *_arg,
                                int priority, int stacksize);


/** Periodic tasks all run at regular priority, running on the same
 * thread. The function proc should not block for extended periods of
 * time, as it will then block all other periodic tasks. Instead, it
 * should run and return quickly. The result code determines when/if it will be executed again:
 *
 * ret < 0  : Task will not be run again.
 *
 * ret == 0 : Task will be run at its default interval
 *
 * ret > 0 : The task will be rescheduled for (ret) usec from when it
 *           was invoked this time.  rescheduled. Otherwise, it will
 *           run every usecperiod.
 **/
nkern_periodic_task_t *nkern_task_create_periodic(const char *name,
                                                  int32_t (*proc)(void*),
                                                  void *arg,
                                                  int64_t usecperiod);

/** Return the current system time in microseconds. **/
uint64_t nkern_utime();

void nkern_gettimeofday(uint32_t *_days, uint32_t *_hours, uint32_t *_minutes,
                        uint32_t *_seconds, uint32_t *_usecs);

void nkern_usleep(uint64_t usecs);
void nkern_sleep_until(uint64_t utime);

uint32_t _nkern_wake_all(nkern_wait_list_t *wl);
uint32_t _nkern_wake_one(nkern_wait_list_t *wl);

static inline uint32_t nkern_wake_all(nkern_wait_list_t *l)
{
    irqstate_t flags;
    interrupts_disable(&flags);
    uint32_t res = _nkern_wake_all(l);
    interrupts_restore(&flags);

    if (res)
        nkern_yield();

    return res;
}

static inline uint32_t nkern_wake_one(nkern_wait_list_t *l)
{
    irqstate_t flags;
    interrupts_disable(&flags);
    uint32_t res = _nkern_wake_one(l);
    interrupts_restore(&flags);
    return res;
}

/////////////////////////////////////////////////////////////////////////////
// Optional IRQ safety prologue/epilogue
//
// Some nkern operations are illegal from an interrupt context. By using these
// prologue and epilogues, incorrect usage will be detected.
extern volatile uint32_t nkern_in_interrupt_flag;

#define NKERN_IRQ_ENTER    nkern_in_interrupt_flag++;
#define NKERN_IRQ_EXIT     nkern_in_interrupt_flag--;

static inline uint32_t nkern_in_irq()
{
    return nkern_in_interrupt_flag;
}

/***************************************************
 * Debug tasks
 ***************************************************/

void nkern_kprintf_init(iop_t *iop);

// emit debugging information
void nkern_print_stats(iop_t *iop);

extern nkern_task_t *nkern_running_task;

/************************
 * API for use by device drivers
 ************************/
void _nkern_schedule();
void _nkern_periodic(); // to be called only by the periodic timer

/** a light-weight printf, always safe to call (re-entrant, from ISR,
    never blocks unless underlying putc blocks). **/
void pprintf_putc(void (*putc)(char c), const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
void pprintf(iop_t *iop, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
void vpprintf(iop_t *iop, const char *fmt, va_list ap);

// this putc function writes to an intermediate FIFO and will never
// block.  Thus it is safe to call kprintf from any context.
/*
void kprintf_fifo_out(char c);
void kprintf_wakeup();
#define kprintf(...) { pprintf(kprintf_fifo_out, __VA_ARGS__); kprintf_wakeup(); }
*/

void kprintf(const char *fmt, ...);
void kprintf_flush();

/*************************
 * hooks
 ************************/
/** ensure that interrupts are disabled, saving the previous interrupt state **/
void interrupts_disable(uint32_t *stateptr);

/** restore the previous interrupt state **/
void interrupts_restore(uint32_t *stateptr);

/** Enable interrupts... you generally don't want this! **/
void interrupts_force_enable();

/** called when nkern detects a fatal error, providing a diagnostic code. **/
void nkern_fatal(uint32_t v);

/** used to transition into the multithreaded environment for the first time. **/
void nkhook_initialize_task_state(nkern_task_t *task, void (*proc)(void*), void *_arg, int stacksize);
void nkhook_userspace_reschedule();

void _nkern_bootstrap();

static inline int putc_iop(iop_t *iop, char c)
{
    return iop->write(iop, &c, 1);
}

static inline int getc_iop(iop_t *iop)
{
    char c;
    int res = iop->read(iop, &c, 1);
    if (res < 0)
        return res;
    return c;
}

static inline int flush_iop(iop_t *iop)
{
    if (iop->flush != NULL)
        return iop->flush(iop);
    return 0;
}

/*
int openfd(void *user,
           long (*read)(void *user, void *data, int len ),
           long (*write)(void *user, const void *data, int len));

// set a particular fd.
int setfdops(int fd,
             void *user,
             long (*read)(void *user, void *data, int len ),
             long (*write)(void *user, const void *data, int len));
*/
#ifndef STDIN
#define STDIN  0
#define STDOUT 1
#define STDERR 2
#endif

static inline void assert_fail(char *exp, char *file, char *basefile, int line)
{
    kprintf("assert fail: %s at %s %s : %i\n", exp, file, basefile, line);
    while(1);
}

#define assert(expr) if (!(expr)) assert_fail(#expr, __FILE__, __BASE_FILE__, __LINE__);

int nkern_wait_until(nkern_wait_list_t *wl, uint64_t utime);
int nkern_wait_many_until(nkern_wait_t *waits, int num_waits, uint64_t utime);

// atomically unlock the mutex and wait on the waitlist. (similar to
// pthread_cond_wait).
int nkern_unlock_and_wait(nkern_mutex_t *mutex, nkern_wait_list_t *wl);
int nkern_unlock_and_wait_until(nkern_mutex_t *mutex, nkern_wait_list_t *wl, int64_t utime);

#define nkern_wait(wl) nkern_wait_until(wl, 0)
#define nkern_wait_timeout(wl, usecs) nkern_wait_until(wl, nkern_utime() + usecs)
#define nkern_wait_many_timeout(ws, nw, usecs) nkern_wait_many_until(ws, nw, nkern_utime() + usecs)

#define nkern_usleep(usecs) nkern_wait_until(NULL, nkern_utime() + usecs)
#define nkern_sleep_until(utime) nkern_wait_until(NULL, utime)

int nkern_init_wait_event(nkern_wait_t *wr, nkern_wait_list_t *wl);

/** Before calling nkern_start, the entire nkern stack should be
 * initialized to this value. **/
#define NKERN_STACK_MAGIC_NUMBER 0x5718a9bf

/** Add entropy to the pool. **/
void nkern_random_entropy_add(uint32_t entropy, int32_t nbits);

/** How many bits of entropy are available? **/
uint32_t nkern_random_bits_available();

/** Take nbits from the entropy pool. **/
uint32_t nkern_random_take(uint32_t nbits);


#endif
