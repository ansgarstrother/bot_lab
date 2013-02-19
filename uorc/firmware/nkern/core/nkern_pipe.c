#include "nkern_pipe.h"

void nkern_pipe_init(nkern_pipe_t *p, const char *name)
{
    p->readpos = 0;
    p->writepos = 0;

    nkern_task_list_init(&p->read_wait, name);
    nkern_task_list_init(&p->write_wait, name);
}

void nkern_pipe_putc(nkern_pipe_t *p, char c)
{
    
}

char nkern_pipe_getc(nkern_pipe_t *p)
{
    while (p->readpos == p->writepos)
        nkern_wait(&p->read_wait);

    
}
