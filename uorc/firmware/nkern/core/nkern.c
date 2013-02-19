#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <reent.h>

#include <nkern.h>
#include <nkern_internal.h>
#include <arch.h>

#include "scheduler.h"
#include "periodic.h"

// How often do we update process statistics?
#define ACCOUNTING_UPDATE_USECS 500000

// a list of sleeping tasks, sorted by wake-up time.
sleep_queue_t sleep_queue;

// the scheduler is the data-structure + algorithm that determines
// what process will run next.
scheduler_t scheduler;

// the currently running task
nkern_task_t *nkern_running_task;

volatile uint32_t nkern_in_interrupt_flag;
static uint32_t nkern_next_pid = 1;

// head pointer of linked list connecting all tasks
static nkern_task_t *tasks_head;
static int num_tasks;          // total number of tasks

extern uint32_t _nkern_stack[], _nkern_stack_top[], _heap_end[];

static int load_accumulator;   // used in the computation of load_average[0]
static int load_counter;       // used in the computation of load_average[0]
static int load_average[3];    // load_average[0] and low-passed versions of it.

extern void nkern_initialize_state(nkern_task_t *state, void (*proc)(void*), void *_arg, int stacksize);
void *_sbrk(ptrdiff_t incr);

#define STACK_GUARD_WORD   4

/** the idle thread, which runs at SYSTEM_IDLE priority  */
void nkern_idle_task(void *a)
{
    (void) a;

    while(1)
        nkern_yield();
}

const char* const prioritynames[] = {"pri0", "pri1", "pri2", "pri3", "pri4", "pri5", "pri6"};

void nkern_init()
{
    sleep_queue_init(&sleep_queue);
    scheduler_init(&scheduler);

    // stack must be large enough for any IRQs that run on it...
    nkern_task_t *idletask = nkern_task_create("idle", nkern_idle_task, NULL,
                                               NKERN_PRIORITY_SYS_IDLE, 1024);
    nkern_periodic_init();

    nkern_running_task = idletask;
}

void nkern_start()
{
//    irqstate_t flags;
//    interrupts_disable(&flags);

//    nkern_running_task = NULL; //scheduler_pick(&scheduler);
//    assert(nkern_running_task != NULL);

    _nkern_bootstrap();   // never returns....
}

void nkern_exit()
{
    irqstate_t flags;
    interrupts_disable(&flags);

    nkern_running_task->status = NKERN_TASK_EXITING;
    nkern_yield();
    interrupts_restore(&flags);

    while(1);
}

nkern_task_t *nkern_task_create(const char *name,
                                void (*proc)(void*), void *_arg,
                                int priority, int stack_size)
{
    nkern_task_t *task = (nkern_task_t*) calloc(1, sizeof(nkern_task_t));

    if (task == NULL)
        nkern_fatal(0xF0F1);

    task->name = name;
    task->stack = calloc((stack_size+3)/4, 4);
    task->stack_size = stack_size;

    if (task->stack == NULL) {
        nkern_fatal(0xF0F2);
    }

    // sign the stack so we can measure stack usage later
    for (int i = 0; i < stack_size/4; i++)
        ((uint32_t*) task->stack)[i] = NKERN_STACK_MAGIC_NUMBER;

    if (priority >= NKERN_NPRIORITIES)
        priority = NKERN_NPRIORITIES-1;

    task->priority = priority;
    task->status = NKERN_TASK_READY;

    nkhook_initialize_task_state(task, proc, _arg, stack_size);

    ///////// critical section /////////////
    irqstate_t flags;
    interrupts_disable(&flags);

    task->pid = nkern_next_pid++;

    scheduler_add(&scheduler, task);

    // add task to our linked list of all tasks
    task->next = tasks_head;
    tasks_head = task;
    num_tasks++;

    interrupts_restore(&flags);

    return task;
}

void _destroy_task(nkern_task_t *n)
{
    // search the linked list of tasks and remove this one.
    nkern_task_t **p = &tasks_head;
    nkern_task_t *previous = NULL;

    while (*p) {
        if ((*p) == n) {
            (*p) = n->next;
            n->next = NULL;
            break;
        }
        previous = *p;
        p = &((*p)->next);
    }

    num_tasks--;

    // deallocate storage
    free(nkern_running_task->stack); // safe, since malloc/free cli()
    free(nkern_running_task);
}

void nkern_dump_state(nkern_task_t *task);

void nkern_yield()
{
    irqstate_t flags;
    interrupts_disable(&flags);

    if (nkern_in_irq())
        nkern_fatal(0xf101);

    nkhook_userspace_reschedule();

    interrupts_restore(&flags);
}

int nkern_init_wait_event(nkern_wait_t *wr, nkern_wait_list_t *wl)
{
    wr->task = nkern_running_task;
    wr->list = wl;
    wr->occurred = 0;

    irqstate_t flags;
    interrupts_disable(&flags);
    _nkern_wait_list_append(wl, wr);
    interrupts_restore(&flags);

    return 0;
}

int nkern_wait_until(nkern_wait_list_t *wl, uint64_t utime)
{
    if (!wl)
        return nkern_wait_many_until(NULL, 0, utime);

    nkern_wait_t waits[1];

    nkern_init_wait_event(&waits[0], wl);
    return nkern_wait_many_until(waits, 1, utime);
}

// atomically unlock the mutex and wait on the waitlist. Returns the
// value of nkern_wait. We have to be careful to call
// mutex_unlock_preempt, because unlocking a mutex can force a context
// switch, which would make our unlock and wait non-atomic.
int nkern_unlock_and_wait(nkern_mutex_t *mutex, nkern_wait_list_t *wl)
{
    irqstate_t flags;
    interrupts_disable(&flags);

    nkern_mutex_unlock_nopreempt(mutex);

    nkern_wait_t waits[1];
    nkern_init_wait_event(&waits[0], wl);
    int ret = nkern_wait_many_until(waits, 1, 0);

    interrupts_restore(&flags);
    return ret;
}

// atomically unlock the mutex and wait on the waitlist. Returns the
// value of nkern_wait.
int nkern_unlock_and_wait_until(nkern_mutex_t *mutex, nkern_wait_list_t *wl, int64_t utime)
{
    irqstate_t flags;
    interrupts_disable(&flags);

    nkern_mutex_unlock_nopreempt(mutex);

    nkern_wait_t waits[1];
    nkern_init_wait_event(&waits[0], wl);
    int ret = nkern_wait_many_until(waits, 1, utime);

    interrupts_restore(&flags);
    return ret;
}

// This is the mother-of-all wait methods; all other sleep and wait
// methods are implemented in terms of it. The index of the event that
// woke the thread is returned (or -1 if it was a timeout).
int nkern_wait_many_until(nkern_wait_t *waits, int num_waits, uint64_t utime)
{
    irqstate_t flags;
    interrupts_disable(&flags);

    nkern_running_task->waits = waits;
    nkern_running_task->num_waits = num_waits;
    nkern_running_task->status = NKERN_TASK_BEGIN_WAIT;
    nkern_running_task->wait_utime = utime;

    if (utime)
        sleep_queue_insert_ascending_waketime(&sleep_queue, nkern_running_task);

    nkern_yield();

    interrupts_restore(&flags);

    return nkern_running_task->wait_index;
}

static uint64_t last_schedule_utime;
static uint64_t last_accounting_utime; // time of last accounting
static uint64_t last_accounting_utime_interval; // how long the last accounting period was
static uint64_t next_accounting_utime; // when to do the next accounting

static inline void update_accounting(uint64_t now_utime)
{
    if (now_utime < next_accounting_utime)
        return;

    // cpu utilization per process
    uint64_t elapsed_utime = now_utime - last_accounting_utime;
    last_accounting_utime_interval = elapsed_utime;

    last_accounting_utime = now_utime;
    next_accounting_utime = now_utime + ACCOUNTING_UPDATE_USECS;

    for (nkern_task_t *n = tasks_head; n != NULL; n = n->next) {

        n->usecs_used_last = n->usecs_used_current;
        n->usecs_used_current = 0;
        // fixed point math for better precision (* 128)
        n->scheduled_count_last = n->scheduled_count_current *
            (128 * 1000000 / ACCOUNTING_UPDATE_USECS) / 128;
        n->scheduled_count_current = 0;
    }

    // sample load average over last interval
    if (load_counter) {
        load_average[0] = load_accumulator*1000 / load_counter;
        // reset counters...
        load_accumulator = 0;
        load_counter = 0;
    }

    // the "8" is to cause the correct rounding.
    load_average[1] = (load_average[1]*15 + load_average[0] + 8)/16;
    load_average[2] = (load_average[2]*15 + load_average[1] + 8)/16;
}

// This function should be called at a fixed periodic rate (although
// ideally with some randomness). It should not be called at every
// context switch, since these are not periodic due to yield()s.
static uint64_t last_periodic_utime;
void _nkern_periodic()
{
    uint64_t utime = nkern_utime();
    if (utime - last_periodic_utime > 1000) {
        // subtract 1 for the idle task
        load_accumulator += scheduler_num_runnable_tasks(&scheduler) - 1;
        load_counter++;

        last_periodic_utime = utime;
    }
}

// Wake up a task that was waiting. The event that triggered the
// wakeup should be passed in, or NULL if it was a timeout.
// This function is only called after all wait lists have been set up
// and the user task has called wait().
static int _wake_up_task(nkern_task_t *n, nkern_wait_t *w)
{
    if (n->status == NKERN_TASK_WAIT) {
        if (w)
            n->wait_index = w - n->waits;
        else
            n->wait_index = -1;

        for (int i = 0; i < n->num_waits; i++) {
            _nkern_wait_list_remove(n->waits[i].list, &n->waits[i]);
        }

        if (n->wait_utime)
            sleep_queue_remove(&sleep_queue, n);

        // mark the task as ready to go and schedule it.
        n->status = NKERN_TASK_READY;
        return scheduler_add(&scheduler, n);

    } else {
        // in all other cases, the task is still runnable and hasn't
        // yet been through NKERN_TASK_BEGIN_WAIT. The task's
        // readiness to run will be discovered there, so do nothing.

        // no change in priority.
        return 0;
    }
}

/** Pick a new _nknern_running_task */
void _nkern_schedule()
{
    //////////////////////////////////////////////////
    // safety checks
    //////////////////////////////////////////////////
    if (nkern_running_task == NULL)
        nkern_fatal(0x1111);

    // test for stack overflow
    if (((uint32_t*) nkern_running_task->stack)[STACK_GUARD_WORD]!=NKERN_STACK_MAGIC_NUMBER) {
        nkern_fatal(0xF0F3);
    }

    uint64_t now_utime = nkern_utime();
    uint64_t delta_utime = now_utime - last_schedule_utime;
    last_schedule_utime = now_utime;
    nkern_running_task->usecs_used_current += delta_utime;

    //////////////////////////////////////////////////
    // put the current task away.
    //////////////////////////////////////////////////
    switch (nkern_running_task->status)
    {
    case NKERN_TASK_BEGIN_WAIT:
    {
        // did the event already occur?
        int occurred = -1;
        for (int i = 0; i < nkern_running_task->num_waits; i++) {
            if (nkern_running_task->waits[i].occurred) {
                occurred = i;
                break;
            }
        }

        // if so, clean up and wake the task.
        if (occurred >= 0) {

            for (int i = 0; i < nkern_running_task->num_waits; i++)
                _nkern_wait_list_remove(nkern_running_task->waits[i].list, &nkern_running_task->waits[i]);

            if (nkern_running_task->wait_utime)
                sleep_queue_remove(&sleep_queue, nkern_running_task);

            nkern_running_task->status = NKERN_TASK_READY;

            scheduler_add(&scheduler, nkern_running_task);

        } else {
            // otherwise, put the task to sleep.
            nkern_running_task->status = NKERN_TASK_WAIT;
        }
        break;
    }
    case NKERN_TASK_READY:
        scheduler_add(&scheduler, nkern_running_task);
        break;

    case NKERN_TASK_WAIT:
        nkern_fatal(0xf0f5);
        break;

    case NKERN_TASK_EXITING:
        _destroy_task(nkern_running_task);
        break;

    default:
        nkern_fatal(0xF0F4);
    }

    nkern_running_task = NULL;

    // now that all processes are refiled into a waitlist, update
    // their accounting information.
    update_accounting(now_utime);

    //////////////////////////////////////////////////
    // wake up sleeping processes if their timer has expired
    //////////////////////////////////////////////////

    nkern_task_t *n;
    while ((n = sleep_queue_remove_head_if_sleep_done(&sleep_queue, now_utime))) {
        _wake_up_task(n, NULL);
    }

    //////////////////////////////////////////////////
    // pick a new task (the one with maximum priority)
    //////////////////////////////////////////////////
    nkern_running_task = scheduler_pick(&scheduler);

    if (!nkern_running_task)
        nkern_fatal(0xF0F5);     // no task? where did the idle task go?

    // we have a running task
    nkern_running_task->scheduled_count_current++;
}

/** returns 1 if a task with higher priority than the currently running task was woken up. **/
uint32_t _nkern_wake_one(nkern_wait_list_t *wl)
{
    nkern_wait_t *w = _nkern_wait_list_remove_first(wl);

    if (w) {
        w->occurred = 1;
        return _wake_up_task(w->task, w);
    }

    return 0;
}

/** returns 1 if a task with higher priority than the currently running task was woken up. **/
uint32_t _nkern_wake_all(nkern_wait_list_t *wl)
{
    uint32_t reschedule = 0;

    while (!_nkern_wait_list_is_empty(wl)) {
        reschedule |= _nkern_wake_one(wl);
    }

    return reschedule;
}

/** starting at the bottom of the stack, count how many words match
 * our stack signature. **/
static int stack_available(uint32_t *p)
{
    int      cnt = 0;

    while (*p == NKERN_STACK_MAGIC_NUMBER) {
        cnt++;
        p++;
    }
    return cnt*4;
}

void nkern_gettimeofday(uint32_t *_days,
			uint32_t *_hours, uint32_t *_minutes, uint32_t *_seconds, uint32_t *_usecs)
{
    uint64_t now = nkern_utime();
    uint64_t sec = now/1000000L;

    uint32_t usecs = now - sec*1000000L;

    uint32_t days = (int) sec/86400L;
    sec -= 86400L*days;
    uint32_t hours = (int) sec/3600L;
    sec -= 3600L*hours;
    uint32_t minutes = (int) sec/60L;
    sec -= 60L*minutes;
    uint32_t seconds = sec;

    *_days = days;
    *_hours = hours;
    *_minutes = minutes;
    *_seconds = seconds;
    *_usecs = usecs;
}

extern uint64_t ip_rx_count, ip_tx_count;

struct task_stats
{
    nkern_task_t *task;
    uint32_t usecs_used;
    uint32_t scheduled_count;
};

static int task_stats_cputime_compare(const void *_a, const void *_b)
{
    struct task_stats *a = (struct task_stats*) _a;
    struct task_stats *b = (struct task_stats*) _b;

    return b->usecs_used - a->usecs_used; // XXX or usecs_used_last?
}

void nkern_print_stats(iop_t *iop)
{
    irqstate_t flags;
    interrupts_disable(&flags);

    int our_num_tasks = num_tasks;
    struct task_stats stats[our_num_tasks];

    int idx = 0;
    uint32_t total_usecs_used = 0;

    // atomically sample interesting task state
    for (nkern_task_t *n = tasks_head; n != NULL; n = n->next) {
        stats[idx].task = n;
        stats[idx].usecs_used = n->usecs_used_last;
        stats[idx].scheduled_count = n->scheduled_count_last;
        total_usecs_used += stats[idx].usecs_used;
        idx++;
    }

    interrupts_restore(&flags);

    qsort(stats, our_num_tasks, sizeof(struct task_stats), task_stats_cputime_compare);

    // now, produce some output.
    uint32_t days, hours, minutes, seconds, usecs;
    nkern_gettimeofday(&days, &hours, &minutes, &seconds, &usecs);

    int kernel_stack_sz = ((int) _nkern_stack_top) - ((int) _nkern_stack);

    pprintf(iop, "=================================================================\n");
    pprintf(iop, "up %d days,                                     %5d:%02d:%02d.%06d\n",
            (int) days, (int) hours, (int) minutes, (int) seconds, (int) usecs);

    pprintf(iop, "load average:                %6d.%03d   %6d.%03d   %6d.%03d\n\n",
            load_average[0]/1000, load_average[0]%1000,
            load_average[1]/1000, load_average[1]%1000,
            load_average[2]/1000, load_average[2]%1000);

    pprintf(iop, "task           stack sz / alloc     schdl    time (%%)    waitlist\n");
    pprintf(iop, "-----------------------------------------------------------------\n");

    int total_percent100  = 0;
    int total_schedules   = 0;
    int total_stack_used  = 0;
    int total_stack_alloc = 0;

    for (int i = 0; i < our_num_tasks; i++) {
        struct task_stats *stat = &stats[i];
        nkern_task_t *n = stat->task;

        int stack_used = n->stack_size - stack_available(n->stack);
        int percent100 = 10000LL * stat->usecs_used / total_usecs_used;

        pprintf(iop, "%-16s %6d / %-6d %8d %6d.%02d      ", n->name,
                stack_used, n->stack_size, (int) stat->scheduled_count,
                percent100/100, percent100%100);

        if (n == nkern_running_task) {
            pprintf(iop, "*RUN* (%d)", (int) n->priority);
        } else {
            if (n->status == NKERN_TASK_READY) {
                pprintf(iop, "ready (%d)", (int) n->priority);
            } else {
                for (int i = 0; i < n->num_waits; i++) {
                    pprintf(iop, "%s ", n->waits[i].list->name);
                }
                if (n->wait_utime)
                    pprintf(iop, "sleep");
            }
        }
        pprintf(iop, "\n");

        // don't include CPU time of idle process
        if (strcmp(n->name, "idle"))
            total_percent100 += percent100;

        total_stack_used  += stack_used;
        total_stack_alloc += n->stack_size;
        total_schedules   += stat->scheduled_count;
    }

    pprintf(iop, "kernel           %6d / %-6d      ---       ---      ---\n",
            kernel_stack_sz - stack_available(_nkern_stack), kernel_stack_sz);

    pprintf(iop, "-----------------------------------------------------------------\n");
    pprintf(iop, "                 %6d / %-6d    %5d %6d.%02d           n/a\n",
            total_stack_used, total_stack_alloc, total_schedules, total_percent100/100, total_percent100%100);
    pprintf(iop, "heap remaining: %lu\n", ((uint32_t) _heap_end) - ((uint32_t) _sbrk(0)));
    pprintf(iop, "ip_tx packets: %15d    ip_rx_packets: %15d\n", (int) ip_tx_count, (int) ip_rx_count);

    interrupts_restore(&flags);
}

