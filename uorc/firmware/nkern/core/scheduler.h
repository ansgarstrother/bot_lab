#ifndef _NKERN_SCHEDULER_H
#define _NKERN_SCHEDULER_H

typedef struct scheduler scheduler_t;
struct scheduler
{
    nkern_task_t *head[NKERN_NPRIORITIES];
    nkern_task_t *tail[NKERN_NPRIORITIES];

    int runnable_tasks;
};

// one-time initialization of state
void scheduler_init(scheduler_t *s);

// signal to the scheduler that task n is now runnable (allowing n to
// be returned later by _pick()).  Returns 1 if a context switch
// should be performed ASAP (e.g., if the newly enabled thread has
// higher priority than the currently running task.
int scheduler_add(scheduler_t *s, nkern_task_t *n);

// Pick the next task to run.
nkern_task_t *scheduler_pick(scheduler_t *s);

// How many tasks are runnable at this moment?
int scheduler_num_runnable_tasks(scheduler_t *s);

#endif
