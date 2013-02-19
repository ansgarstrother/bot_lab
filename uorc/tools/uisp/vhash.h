#ifndef _VHASH_H
#define _VHASH_H

#include <stdint.h>

struct vhash_element;

struct vhash_element
{
    void *key;
    void *value;

    struct vhash_element *next;
};

typedef struct
{
    uint32_t(*hash)(const void *a);
    int(*equals)(const void *a, const void *b);

    int alloc;
    int size;

    struct vhash_element **elements;

} vhash_t;

vhash_t *vhash_create(uint32_t(*hash)(const void *a), int(*equals)(const void *a, const void *b));
void vhash_destroy(vhash_t *vh);
void *vhash_get(vhash_t *vh, const void *key);
void vhash_put(vhash_t *vh, void *key, void *value);

uint32_t vhash_str_hash(const void *a);

// returns 1 if equal
int vhash_str_equals(const void *a, const void *b);

#endif
