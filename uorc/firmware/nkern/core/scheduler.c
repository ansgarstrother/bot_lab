#include <stdio.h>
#include <nkern.h>
#include "scheduler.h"

// This is the heart of the scheduler. Alternative scheduling
// algorithms can be replaced by substituing just the few functions in
// this file. You can also add state to nkern_task_t.
//
// This implementation is very simple: a singly-linked list with five
// priority levels. The runnable task with greatest priority is
// picked. This scheduler does not handle priority inversion.

void scheduler_init(scheduler_t *s)
{
    for (int pri = 0; pri < NKERN_NPRIORITIES; pri++) {
        s->head[pri] = NULL;
        s->tail[pri] = NULL;
    }

    s->runnable_tasks = 0;
}

// add the process to the tail of the appropriate priority list
// returns 1 if the newly added task has higher priority than the
// currently running task.
int scheduler_add(scheduler_t *s, nkern_task_t *n)
{
    uint32_t pri = n->priority;

    if (s->head[pri] == NULL) {
        // empty list?
        s->head[pri] = n;
        s->tail[pri] = n;
        n->scheduler_next = NULL;
    } else {
        // append
        s->tail[pri]->scheduler_next = n;
        s->tail[pri]= n;
        n->scheduler_next = NULL;
    }

    s->runnable_tasks ++;

    return n->priority > nkern_running_task->priority;
}

nkern_task_t *scheduler_pick(scheduler_t *s)
{
    for (int pri = NKERN_NPRIORITIES - 1; pri >= 0; pri--) {

        nkern_task_t *n = s->head[pri];

        if (!n)
            continue;

        s->head[pri] = n->scheduler_next;

        // list now empty? (fix tail pointer)
        if (s->head[pri] == NULL)
            s->tail[pri] = NULL;

        n->scheduler_next = NULL;
        s->runnable_tasks--;
        return n;
    }

    return NULL;
}

// how many runnable tasks?
int scheduler_num_runnable_tasks(scheduler_t *s)
{
    return s->runnable_tasks;
}
