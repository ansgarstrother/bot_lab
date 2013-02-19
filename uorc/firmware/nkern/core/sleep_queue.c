#include <nkern.h>

void sleep_queue_init(sleep_queue_t *sq)
{
    sq->head = NULL;
}

// unconditionally remove the given task from the sleep queue.
// currently O(N), could be made O(1) by making the queue
// doubly-linked.
void sleep_queue_remove(sleep_queue_t *sq, nkern_task_t *n)
{
    nkern_task_t **p = &sq->head;

    while (*p) {
        if ((*p) == n) {
            (*p) = n->sleep_next;

            n->sleep_next = NULL;
            return;
        }

        p = &((*p)->sleep_next);
    }
}

// insert the task into the sleep queue according to its wait_utime
void sleep_queue_insert_ascending_waketime(sleep_queue_t *sq, nkern_task_t *n)
{
    nkern_task_t **p = &sq->head;
    while ((*p)!=NULL && n->wait_utime > (*p)->wait_utime) {
        p = &(*p)->sleep_next;
    }

    n->sleep_next = *p;
    *p = n;
}

nkern_task_t *sleep_queue_remove_head_if_sleep_done(sleep_queue_t *sq, uint64_t utime)
{
    nkern_task_t *n = sq->head;

    if (n == NULL || n->wait_utime > utime)
        return NULL;

    sq->head = n->sleep_next;
    n->sleep_next = NULL;
    return n;
}
