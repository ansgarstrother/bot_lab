#include <stdint.h>
#include "nkern.h"

static nkern_wait_list_t periodic_wait_list;
static nkern_mutex_t periodic_mutex;

static nkern_periodic_task_t * volatile head;

// mutex must be acquired.
static void insert_ascending(nkern_periodic_task_t *n)
{
    nkern_periodic_task_t **p = (nkern_periodic_task_t**) &head;
    while ((*p)!=NULL && n->next_utime > (*p)->next_utime) 
        p = &(*p)->next;
    
    n->next = *p;
    *p = n;
}

void periodic_task(void *arg)
{
    while (1) {
        nkern_mutex_lock(&periodic_mutex);

        int64_t until_utime;

        if (head == NULL)
            until_utime = 0; // wait forever.
        else
            until_utime = head->next_utime;

        nkern_unlock_and_wait_until(&periodic_mutex, &periodic_wait_list, until_utime);

        // we may have awoken because there was a change to the list,
        // or because our original timer expired. Check to see if we should run the task on head.
        nkern_mutex_lock(&periodic_mutex);
        
        int64_t nowtime = nkern_utime();

        if (head == NULL || nowtime < head->next_utime) {
            nkern_mutex_unlock(&periodic_mutex);
            continue;
        }

        nkern_periodic_task_t *thistask = head;
        head = head->next;

        nkern_mutex_unlock(&periodic_mutex);

        int32_t ret = thistask->proc(thistask->arg);

        if (ret < 0) {
            free(thistask);
            continue;
        }

        // reschedule the task.
        if (ret == 0)
            thistask->next_utime += thistask->uperiod;
        else
            thistask->next_utime += ret;

        nkern_mutex_lock(&periodic_mutex);
        insert_ascending(thistask);
        nkern_mutex_unlock(&periodic_mutex);
    }
}

void nkern_periodic_init()
{
    nkern_wait_list_init(&periodic_wait_list, "periodic");
    nkern_mutex_init(&periodic_mutex, "periodic");

    nkern_task_create("periodic", periodic_task, NULL, NKERN_PRIORITY_NORMAL, 1024);
}

// periodic task: if returns != 0, then the task is removed. Else,
// task is rescheduled.

nkern_periodic_task_t *nkern_task_create_periodic(const char *name, int32_t (*proc)(void*), void *arg, int64_t usecperiod)
{
    nkern_periodic_task_t *task = (nkern_periodic_task_t*) calloc(1, sizeof(nkern_periodic_task_t));
    task->name = name;
    task->proc = proc;
    task->arg = arg;
    task->uperiod = usecperiod;
    task->next_utime = nkern_utime() + usecperiod;

    nkern_mutex_lock(&periodic_mutex);
    insert_ascending(task);
    nkern_wake_all(&periodic_wait_list);
    nkern_mutex_unlock(&periodic_mutex);
    
    return task;
}


