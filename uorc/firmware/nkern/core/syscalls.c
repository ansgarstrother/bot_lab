#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include <nkern.h>

static volatile char *heap_ptr;

extern const char _end[]; // first available address for the heap
extern const char _heap_end[]; 

#define MAX_FDS 16

struct fdops
{
    char  is_open;
    void *user; 

    long (*write)(void *user, const void *data, int len );
    long (*read)(void *user, void *data, int len );
};

struct fdops fdops_table[MAX_FDS];

long write_null(void *user, const void *data, int len)
{
    (void) user; (void) data; (void) len;
    return -1;
}

long read_null(void *user, void *data, int len)
{
    (void) user; (void) data; (void) len;
    return -1;
}

void syscall_init()
{
    for (int fd = 0; fd < MAX_FDS; fd++) {
        fdops_table[fd].user = NULL;
        fdops_table[fd].read = read_null;
        fdops_table[fd].write = write_null;
        fdops_table[fd].is_open = 0;
    }
}

int isatty(int file)
{
    (void) file;
    return 1;
}

/** We implement malloc_lock by completely disabling interrupts. This
    means that ISRs can call malloc if they absolutely have to, but it
    also means that mallocs are BAD, as they decrease system response
    time! **/
static int malloc_lock_count;
irqstate_t malloc_irqstate;

void __malloc_lock(struct _reent *_r)
{
    (void) _r;
    irqstate_t flags;

    interrupts_disable(&flags);

    if (malloc_lock_count==0) {
        malloc_irqstate = flags;
    }

    malloc_lock_count++;
}

void __malloc_unlock(struct _reent *_r)
{
    (void) _r;

    // interrupts are already disabled
    malloc_lock_count--;
    if (malloc_lock_count==0)
        interrupts_restore(&malloc_irqstate);
}


void *_sbrk(ptrdiff_t incr)
{
    irqstate_t flags;

    if (heap_ptr == NULL)
        heap_ptr = (char*) _end;

    interrupts_disable(&flags);

    char *p = (char*) heap_ptr;

    // if we've crossed the boundary given by _heap_end, 
    // we've run out of memory. 
    if ((heap_ptr+incr < _heap_end) != (p < _heap_end)) {
        p = NULL;
    } else {
        heap_ptr += incr;
    }

    interrupts_restore(&flags);
    return p;
}

void *_sbrk_r(struct _reent *_r, ptrdiff_t incr)
{
  (void) _r;
  return _sbrk(incr);
}


int setfdops(int fd, 
             void *user, 
             long (*read)(void *user, void *data, int len ),
             long (*write)(void *user, const void *data, int len))

{
    fdops_table[fd].user = user;
    fdops_table[fd].read = read;
    fdops_table[fd].write = write;
    fdops_table[fd].is_open = 1;

    return 0;
}

int openfd(void *user, 
             long (*read)(void *user, void *data, int len ),
             long (*write)(void *user, const void *data, int len))
{
    for (int fd = 0; fd < MAX_FDS; fd++) {
        if (!fdops_table[fd].is_open) {
            int res = setfdops(fd, user, read, write);
            if (!res)
                return fd;
            return -1;
        }
    }

    return -1;
}

_ssize_t _write(int fd, const void *buf, size_t cnt)
{
    return _write_r(NULL, fd, buf, cnt);
}

_ssize_t _write_r(struct _reent *reent, int fd, const void *buf, size_t cnt)
{
    (void) reent;
    return fdops_table[fd].write(fdops_table[fd].user, buf, cnt);
}

int _close(int fd)
{
    return _close_r(NULL, fd);
}

int _close_r(struct _reent *reent, int fd)
{
    (void) reent;
    (void) fd;

    fdops_table[fd].is_open = 0;
    fdops_table[fd].read  = read_null;
    fdops_table[fd].write = write_null;
    fdops_table[fd].user  = NULL;
    return 0;
}

int _fstat_r(struct _reent *reent, int fd, struct stat *pstat)
{
    (void) reent;
    (void) fd;
    (void) pstat;
    return 0;
}

off_t _lseek_r(struct _reent *reent, int fd, off_t pos, int whence)
{
    (void) reent;
    (void) fd;
    (void) pos;
    (void) whence;
    return 0;
}

_ssize_t _read(int fd, void *buf, size_t cnt)
{
    return _read_r(NULL, fd, buf, cnt);
}

_ssize_t _read_r(struct _reent *reent, int fd, void *buf, size_t cnt)
{
    (void) reent;
    return fdops_table[fd].read(fdops_table[fd].user, buf, cnt);
}

int _gettimeofday(struct timeval *tv, struct timezone *tz)
{
    (void) tv;
    (void) tz;
    return 0;
}
