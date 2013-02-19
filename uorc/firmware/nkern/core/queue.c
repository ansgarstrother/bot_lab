#include <stdio.h>
#include <nkern.h>

int nkern_queue_init(nkern_queue_t *q, const char *name)
{
    nkern_wait_list_init(&q->read_waitlist, name);
    nkern_wait_list_init(&q->write_waitlist, name);

    return 0;
}

int nkern_queue_put_if_empty(nkern_queue_t *q, void *d)
{
    irqstate_t flags;
    interrupts_disable(&flags);

    int empty = (q->in == q->out);
    if (empty) {
        nkern_queue_put(q, d, 0);
        interrupts_restore(&flags);
        return 0;
    }

    interrupts_restore(&flags);
    return -1;
}

int nkern_queue_put(nkern_queue_t *q, void *d, int block)
{
    irqstate_t flags;
    interrupts_disable(&flags);

    // in2 is the next value of q->in, if we
    // added an item now.
    int in2;
again:
    in2 = q->in + 1;
    if (in2 == q->alloc)
        in2 = 0;
    
    // if adding this element would overflow the queue
    // then wait for a reader.
    if (in2 == q->out) {
        if (!block) {
            interrupts_restore(&flags);
            return -1;
        }
        nkern_wait(&q->write_waitlist);
        goto again;
    }
    
    // we're good to go!
    q->data[q->in] = d;
    q->in = in2;

    // wake any potential consumers.
    nkern_wake_one(&q->read_waitlist);

    interrupts_restore(&flags);
    return 0;
}

void *nkern_queue_get(nkern_queue_t *q, int block)
{
    irqstate_t flags;
    interrupts_disable(&flags);

again:
    // wait until there's data in the queue.
    if (q->in == q->out) {
        if (!block) {
            interrupts_restore(&flags);
            return NULL;
        }
        nkern_wait(&q->read_waitlist);
        goto again;
    }

    // there's data now.
    void *d = q->data[q->out];
    q->out += 1;
    if (q->out == q->alloc)
        q->out = 0;

    nkern_wake_one(&q->write_waitlist);
    interrupts_restore(&flags);

    return d;
}
