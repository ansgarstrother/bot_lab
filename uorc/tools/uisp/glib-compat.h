#ifndef _GLIB_COMPAT_H
#define _GLIB_COMPAT_H

#include "varray.h"
#include "vhash.h"

#define GPtrArray varray_t
#define g_ptr_array_new() varray_create()
#define g_ptr_array_free(x, y) varray_destroy(x)
#define g_ptr_array_add(x,y) varray_add(x,y)
#define g_ptr_array_index(x,y) varray_get(x,y)
#define g_ptr_array_size(x) varray_size(x)

#define g_str_hash vhash_str_hash
#define g_str_equal vhash_str_equals

#define GHashTable vhash_t
#define g_hash_table_new(x, y) vhash_create(x, y)
#define g_hash_table_destroy(x) vhash_destroy(x)
#define g_hash_table_lookup(x, y) vhash_get(x, y)
#define g_hash_table_insert(x, y, z) vhash_put(x, y, z)
#endif
