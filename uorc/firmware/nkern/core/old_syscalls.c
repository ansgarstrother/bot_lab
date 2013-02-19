#include <stdlib.h>
#include <reent.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include "nkern.h"

/** Syscalls needed to support newlib. **/

/* _end must be declared as char _end[] not char *end. Why?

char foo[]="abcde" means that there is a 6-byte block of ram, and foo is
a constant which is the address of the first character in it.

char *foo="abcde" means that there is a 6-byte block of memory
(possibly in flash) and that we have allocated a 4-byte block of RAM
(the pointer foo) which is initialized with the address of the 6-byte
block.

In the context of the linker, _end is a location in memory-- a
constant. If we declare char *_end, we're saying that the value of
that pointer should be fetched from the memory AT *_end, not the value
of _end itself.
*/

extern char end[]; 

extern char _heap_end[];

char *sbrk_end;

//void *_sbrk_r(struct _reent *_r, ptrdiff_t nbytes)
void *_sbrk(ptrdiff_t nbytes)
{
	uint32_t flags;

	disable_interrupts(&flags);

	nkern_fatal(0x7171);
	//	(void) _r;

        if (!sbrk_end) {        //  Initialize if first time through. 
		sbrk_end = end;
        }

        char *base = sbrk_end;        //  Point to end of heap. 
        sbrk_end += nbytes;           //  Increase heap.  

	nkern_debugf("sbrk %08x %08X %d\r\n", end, sbrk_end, nbytes);

	if (sbrk_end > _heap_end) {
		nkern_debugf("sbrk: heap exhausted\r\n");
		nkern_blink(2,15);
	}

	enable_interrupts(&flags);

        return base;            //  Return pointer to start of new heap area. 
}

/*
int _fork_r(struct _reent *ptr) 
{
	ptr->_errno = ENOTSUP;
	return -1;
}

nkern_mutex_t malloc_mutex;


void __malloc_lock(struct _reent *_r)
{
//	nkern_mutex_lock(&malloc_mutex);
}

void __malloc_unlock(struct _reent *_r)
{
//	nkern_mutex_unlock(&malloc_mutex);
}
*/

/*

int _stat_r ( struct _reent *_r, const char *file, struct stat *pstat )
{
//	pstat->st_mode = S_IFCHR;
	return 0;
}

int _fstat_r ( struct _reent *_r, int fd, struct stat *pstat )
{
//	pstat->st_mode = S_IFCHR;
	return 0;
}

int _link_r ( struct _reent *_r, const char *oldname, const char *newname )
{
//	r->errno = EMLINK;
	return -1;
}

int _unlink_r ( struct _reent *_r, const char *name )
{
//	r->errno = EMLINK;
	return -1;
}

off_t _lseek_r( struct _reent *_r, int fd, off_t pos, int whence )
{
	return 0;
}

pid_t _getpid_r( struct _reent *_r )
{
	return nkern_running_task->pid;
}

int _times_r ( struct _reent *r, struct tms *tmsbuf )
{
	return -1;
}

int  _open_r( struct _reent *r, const char *path, int flags, int mode )
{
	return -1;
}

int  _close_r ( struct _reent *r, int fd )
{
	return -1;
}

_ssize_t _write_r ( struct _reent *ptr, int fd, const void *buf, size_t cnt )
{
	return -1;
}

_ssize_t _read_r ( struct _reent *r, int fd, void *ptr, size_t len )
{
	return -1;
}

*/
