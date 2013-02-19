#ifndef _NKERN_PIPE_H
#define _NKERN_PIPE_H

#include "nkern.h"

// must be power of two
#define NKERN_PIPE_SIZE 256

typedef struct
{
    int readpos, writepos;

    char buffer[NKERN_PIPE_SIZE];

    nkern_task_list_t read_wait;
    nkern_task_list_t write_wait;

} nkern_pipe_t;


#endif
