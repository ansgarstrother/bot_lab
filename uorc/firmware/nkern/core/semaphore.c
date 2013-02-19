#include <stdio.h>
#include "nkern.h"

void nkern_semaphore_init(nkern_semaphore_t *s, const char *name, int count)
{
    nkern_wait_list_init(&s->waitlist, name);
    s->count = count;
}

uint32_t nkern_semaphore_up_many(nkern_semaphore_t *s, uint32_t count)
{
    irqstate_t flags;
    interrupts_disable(&flags);

    s->count+=count;

    int reschedule = _nkern_wake_all(&s->waitlist);

    interrupts_restore(&flags);    

    return reschedule;
}

uint32_t nkern_semaphore_up(nkern_semaphore_t *s)
{
    return nkern_semaphore_up_many(s, 1);
}

void nkern_semaphore_down(nkern_semaphore_t *s)
{
    irqstate_t flags;
    interrupts_disable(&flags);

    while (1) {
        if (s->count) {
            s->count--;
            interrupts_restore(&flags);
            return;
        }
        nkern_wait(&s->waitlist);
    }
    
    interrupts_restore(&flags);    
}

// return zero on success
int nkern_semaphore_down_try(nkern_semaphore_t *s)
{
    irqstate_t flags;
    interrupts_disable(&flags);

    if (s->count) {
      s->count--;
      interrupts_restore(&flags);
      return 0;
    } 

    // couldn't down the semaphore
    interrupts_restore(&flags);
    return -1;
}
