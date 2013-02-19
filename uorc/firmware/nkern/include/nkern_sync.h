#ifndef _NKERN_SYNC_H
#define _NKERN_SYNC_H

typedef struct nkern_mutex nkern_mutex_t;
typedef struct nkern_semaphore nkern_semaphore_t;
typedef struct nkern_queue nkern_queue_t;
typedef struct nkern_cond nkern_cond_t;

struct nkern_queue
{
    nkern_wait_list_t read_waitlist;
    nkern_wait_list_t write_waitlist;

    int alloc; // size of data[] (capacity of queue + 1)
    volatile int in, out; // data[] is managed as a ring buffer; these are in/out idx
    // if in==out, the array is empty.
    void *data[];
};

struct nkern_mutex
{
    nkern_wait_list_t waitlist;
    nkern_task_t * volatile owner;
    uint32_t lockcount;
};

struct nkern_cond
{
    nkern_wait_list_t waitlist;
};

struct nkern_semaphore
{
    nkern_wait_list_t   waitlist;
    volatile uint32_t   count;
};


/////////////////////////////////////////////////////////////////////////////
// Mutexes
//
// Mutex support re-entrant locking (i.e., a single thread can acquire
// a mutex multiple times without dead-locking.) Of course, the number
// of unlocks() should match the number of locks().
/////////////////////////////////////////////////////////////////////////////

void nkern_mutex_init(nkern_mutex_t *m, const char *name);
void nkern_mutex_lock(nkern_mutex_t *m);
uint32_t nkern_mutex_unlock(nkern_mutex_t *m);
uint32_t nkern_mutex_unlock_nopreempt(nkern_mutex_t *m); // won't yield(). Used for more complex sync operations

/////////////////////////////////////////////////////////////////////////////
// Semaphores
/////////////////////////////////////////////////////////////////////////////

void nkern_semaphore_init(nkern_semaphore_t *s, const char *name, int count);
void nkern_semaphore_down(nkern_semaphore_t *s);
uint32_t nkern_semaphore_up(nkern_semaphore_t *s);
uint32_t nkern_semaphore_up_many(nkern_semaphore_t *s, uint32_t count);
// returns zero on success, -1 if busy
int nkern_semaphore_down_try(nkern_semaphore_t *s);

/////////////////////////////////////////////////////////////////////////////
// Condition Variables
//
// These are NOT well tested.
//
// Condition variables and mutexes are used in conjunction to perform
// signalling from one thread to another.
/////////////////////////////////////////////////////////////////////////////
void nkern_cond_init(nkern_cond_t *c, const char *name);

/* atomically unlock mutex m, and begin waiting on condition variable c */
void nkern_cond_wait(nkern_cond_t *c, nkern_mutex_t *m);

/* wake a single task waiting on the condition variable. You should
   own the mutex. */
uint32_t nkern_cond_signal(nkern_cond_t *c);

/* wake all tasks waiting on the condition variable. You should own
 * the mutex. */
uint32_t nkern_cond_broadcast(nkern_cond_t *c);

/////////////////////////////////////////////////////////////////////////////
// Queues
/////////////////////////////////////////////////////////////////////////////

// an empty queue is signified by in==out, which means that the
// capacity of an a queue is one less than the number of entries
// allocated.
#define NKERN_QUEUE_ALLOC(capacity) { .alloc = (capacity+1), .in = 0, .out = 0, .data = {[0 ... (capacity)] = 0}}

/** Initialize a queue. **/
int nkern_queue_init(nkern_queue_t *q, const char *name);

/** Add data to the queue, blocking if requested. If the operation
 * cannot proceed without blocking, and blocking is disabled, returns
 * < 0. **/
int nkern_queue_put(nkern_queue_t *q, void *d, int block);

/** Retrieve data from the queue, blocking if requested. If the operation
 * cannot proceed without blocking, and blocking is disabled, returns
 * NULL.
 **/
void *nkern_queue_get(nkern_queue_t *q, int block);

int nkern_queue_put_if_empty(nkern_queue_t *q, void *d);

#endif
