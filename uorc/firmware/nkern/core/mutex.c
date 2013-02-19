#include <stdio.h>
#include "nkern.h"

// recursive mutex.
void nkern_mutex_init(nkern_mutex_t *m, const char *name)
{
    m->owner = NULL;
    m->lockcount = 0;
    nkern_wait_list_init(&m->waitlist, name);
}

void nkern_mutex_lock(nkern_mutex_t *m)
{
    irqstate_t flags;

    interrupts_disable(&flags);

    while (1) {
        // lock unclaimed?
        if (m->owner == NULL) {
            m->owner = nkern_running_task;
            m->lockcount = 1;
            break;
        }
        
        // lock already owned?
        if (m->owner == nkern_running_task) {
            m->lockcount++;
            break;
        }
        
        // lock is owned by someone else.
        nkern_wait(&m->waitlist);
    }
    
    interrupts_restore(&flags);
}

// Does not automatically call nkern_yield() if another thread has been woken up.
uint32_t nkern_mutex_unlock_nopreempt(nkern_mutex_t *m)
{
    irqstate_t flags;
    int reschedule = 0;

    interrupts_disable(&flags);

    m->lockcount--;
    if (m->lockcount != 0) {
        goto done;
    }

    // mutex is now really unlocked.
    m->owner = NULL;

    // someone wants this mutex
    // wake up the waiting tasks
    reschedule = _nkern_wake_one(&m->waitlist);

done:
    interrupts_restore(&flags);
    return reschedule;
}

uint32_t nkern_mutex_unlock(nkern_mutex_t *m)
{
    irqstate_t flags;
    int reschedule = 0;

    interrupts_disable(&flags);

    m->lockcount--;
    if (m->lockcount != 0) {
        goto done;
    }

    // mutex is now really unlocked.
    m->owner = NULL;

    // someone wants this mutex
    // wake up the waiting tasks
    reschedule = _nkern_wake_one(&m->waitlist);
    if (reschedule)
        nkern_yield();

done:
    interrupts_restore(&flags);
    return reschedule;
}
