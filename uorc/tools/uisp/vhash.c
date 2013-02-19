#include <stdlib.h>
#include <string.h>

#include "vhash.h"

#define INITIAL_SIZE 16

// when the ratio of allocations to actual size drops below this
// ratio, we rehash. (Reciprocal of more typical load factor.)
#define REHASH_RATIO 2

vhash_t *vhash_create(uint32_t(*hash)(const void *a), int(*equals)(const void *a, const void*b))
{
    vhash_t *vh = (vhash_t*) calloc(1, sizeof(vhash_t));
    vh->hash = hash;
    vh->equals = equals;

    vh->alloc = INITIAL_SIZE;
    vh->elements = (struct vhash_element**) calloc(vh->alloc, sizeof(struct vhash_element*));
    return vh;
}

// free all vhash_element structs. (does not free keys or values).
static void free_elements(struct vhash_element **elements, int alloc)
{
    for (int i = 0; i < alloc; i++) {
        struct vhash_element *el = elements[i];
        while (el != NULL) {
            struct vhash_element *nextel = el->next;
            free(el);
            el = nextel;
        }
    }
}

void vhash_destroy(vhash_t *vh)
{
    free_elements(vh->elements, vh->alloc);
    free(vh->elements);
    free(vh);
}

void *vhash_get(vhash_t *vh, const void *key)
{
    uint32_t hash = vh->hash(key);
    int idx = hash % vh->alloc;

    struct vhash_element *el = vh->elements[idx];
    while (el != NULL) {
        if (vh->equals(el->key, key)) {
            return el->value;
        }
        el = el->next;
    }

    return NULL;
}

// returns one if a new element was added, 0 else. This is abstracted
// so that we can use it when put-ing and resizing.
static inline int vhash_put_real(vhash_t *vh, struct vhash_element **elements, int alloc, void *key, void *value)
{
    uint32_t hash = vh->hash(key);

    int idx = hash % alloc;

    // replace an existing key if it exists.
    struct vhash_element *el = elements[idx];
    while (el != NULL) {
        if (vh->equals(el->key, key)) {
            el->value = value;
            return 0;
        }
        el = el->next;
    }

    // create a new key and prepend it to our linked list.
    el = (struct vhash_element*) calloc(1, sizeof(struct vhash_element));
    el->key = key;
    el->value = value;
    el->next = elements[idx];

    elements[idx] = el;
    return 1;
}

void vhash_put(vhash_t *vh, void *key, void *value)
{
    int added = vhash_put_real(vh, vh->elements, vh->alloc, key, value);
    vh->size += added;

    int ratio = vh->alloc / vh->size;

    if (ratio < REHASH_RATIO) {
        // resize
        int newalloc = vh->alloc*2;
        struct vhash_element **newelements = (struct vhash_element**) calloc(newalloc, sizeof(struct vhash_element*));

        // put all our existing elements into the new hash table
        for (int i = 0; i < vh->alloc; i++) {
            struct vhash_element *el = vh->elements[i];
            while (el != NULL) {
                vhash_put_real(vh, newelements, newalloc, el->key, el->value);
                el = el->next;
            }
        }

        // free the old elements
        free_elements(vh->elements, vh->alloc);

        // switch to the new elements
        vh->alloc = newalloc;
        vh->elements = newelements;
    }
}

uint32_t vhash_str_hash(const void *_a)
{
    char *a = (char*) _a;

    int64_t hash = 0;
    while (*a != 0) {
        hash += *a;
        hash = (hash << 7) + (hash >> 23);
        a++;
    }

    return hash;
}

int vhash_str_equals(const void *_a, const void *_b)
{
    char *a = (char*) _a;
    char *b = (char*) _b;

    return !strcmp(a, b);
}
