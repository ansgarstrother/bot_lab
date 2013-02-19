#include <stdio.h>
#include "nkern.h"

nkern_wait_list_t *nkern_wait_lists;

void nkern_wait_list_init(nkern_wait_list_t *l, const char *name)
{
    irqstate_t state;
    interrupts_disable(&state);

    l->head = NULL;
    l->tail = NULL;
    l->name = name;

    interrupts_restore(&state);
}

void _nkern_wait_list_append(nkern_wait_list_t *l, nkern_wait_t *n)
{
    if (l->head == NULL) {
        // empty list?
        l->head = n;
        l->tail = n;
        n->next = NULL;
    } else {
        // append
        n->next = NULL;

        l->tail->next = n;
        l->tail = n;
    }
}

nkern_wait_t *_nkern_wait_list_remove_first(nkern_wait_list_t *l)
{
    nkern_wait_t *n = l->head;

    if (n == NULL)
        return NULL;

    l->head = n->next;

    // list now empty?
    if (l->head == NULL)
        l->tail = NULL;

    n->next = NULL;
    return n;
}

uint32_t _nkern_wait_list_is_empty(nkern_wait_list_t *l)
{
    return l->head == NULL;
}

// search for and remove the task from the list. n->next is set to
// NULL if the item is found.  returns -1 if the item was not found
// (in which case n->next is not modified).
int _nkern_wait_list_remove(nkern_wait_list_t *l, nkern_wait_t *n)
{
    nkern_wait_t **p = &l->head;
    nkern_wait_t *previous = NULL;

    while (*p) {
        if ((*p) == n) {
            (*p) = n->next;
            
            if (n == l->tail)
                l->tail = previous;

            n->next = NULL;
            return 0;
        }
        previous = *p;
        p = &((*p)->next);
    }

    return -1;
}
