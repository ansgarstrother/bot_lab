#ifndef _NKERN_INTERNAL_H
#define _NKERN_INTERNAL_H

/////////////////////////////////////////////////////////
// Task states
/////////////////////////////////////////////////////////
#define NKERN_TASK_READY            0
#define NKERN_TASK_WAIT             1
#define NKERN_TASK_BEGIN_WAIT       2
#define NKERN_TASK_EXITING          3

typedef struct sleep_queue sleep_queue_t;
struct sleep_queue
{
    nkern_task_t *head;
};

void sleep_queue_init(sleep_queue_t *sq);
void sleep_queue_insert_ascending_waketime(sleep_queue_t *sq, nkern_task_t *n);
nkern_task_t *sleep_queue_remove_head_if_sleep_done(sleep_queue_t *sq, uint64_t utime);
void sleep_queue_remove(sleep_queue_t *sq, nkern_task_t *n);

struct nkern_task
{
    // state is defined by the architecture and 
    // MUST ALWAYS BE THE FIRST ELEMENT!
    nkern_task_state_t state;

    // task name, just used for debugging
    const char *name;

    // e.g., ready, waiting, sleeping
    uint32_t status;

    // the priority of the task. Higher values are higher priority.
    // Values must be [0, NKERN_NPRIORITIES - 1]. Higher-priority
    // tasks *always* pre-empt lower-priority tasks.
    uint32_t priority;

    // the stack that was allocated for the proc.
    void *stack;
    // size in bytes of the stack (as allocated)
    int   stack_size; 

    // unique process identifier
    uint32_t pid;

    // reent structure used by newlib
//    struct _reent *reent;

    nkern_wait_t  *waits;
    int            num_waits; 
    uint64_t       wait_utime;        // if there's a timeout event, the utime.
    int            wait_index;

    // accounting information
    uint32_t usecs_used_last;         // usecs used in last sampling period
    uint32_t usecs_used_current;      // usecs used so far during this sampling period
    uint32_t scheduled_count_last;    // how many times scheduled in last sampling period
    uint32_t scheduled_count_current; // how many times scheduled so far during this sample period

    // forms a linked list of all tasks
    nkern_task_t *next;

    // per-task sleep queue storage
    nkern_task_t *sleep_next;

    // per-task scheduler storage
    nkern_task_t *scheduler_next;
};


#endif
