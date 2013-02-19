#include <stdio.h>
#include "nkern.h"

void nkern_cond_init(nkern_cond_t *c, const char *name)
{
    nkern_task_list_init(&c->waitlist, name);
}

void nkern_cond_wait(nkern_cond_t *c, nkern_mutex_t *m)
{
    irqstate_t flags;

    disable_interrupts(&flags);

    nkern_mutex_unlock(m);
    nkern_task_list_append(&c->waitlist, nkern_running_task);
    nkern_yield();
    
    enable_interrupts(&flags);
}

uint32_t nkern_cond_signal(nkern_cond_t *c)
{
    return nkern_wake_one(&c->waitlist);
}

uint32_t nkern_cond_broadcast(nkern_cond_t *c)
{
    return nkern_wake_all(&c->waitlist);
}
