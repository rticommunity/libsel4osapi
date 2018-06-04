/*
 * FILE: pool.h - simple object pool
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */


#ifndef SEL4OSAPI_POOL_C_
#define SEL4OSAPI_POOL_C_

typedef void (*simple_pool_init_el_fn)(void *el, void *arg);
typedef int (*simple_pool_compare_el_fn)(void *el1, void *el2);

typedef struct simple_pool
{
    sel4osapi_list_t *free_entries;
    sel4osapi_list_t *entries;
    size_t el_size;
    int size;
    int current_size;
    simple_pool_init_el_fn init_fn;
    void *init_arg;
    simple_pool_compare_el_fn compare_fn;
} simple_pool_t;


simple_pool_t*
simple_pool_new(int size, size_t el_size, simple_pool_init_el_fn init_fn, void *init_arg, simple_pool_compare_el_fn compare_fn);

void*
simple_pool_alloc(simple_pool_t *pool);

int
simple_pool_free(simple_pool_t *pool, void *el);

sel4osapi_list_t*
simple_pool_alloc_entry(simple_pool_t *pool);

int
simple_pool_free_entry(simple_pool_t *pool, sel4osapi_list_t *entry);

sel4osapi_list_t*
simple_pool_find_node(simple_pool_t *pool, void *el);

int
simple_pool_get_current_size(simple_pool_t *pool);

#endif /* SEL4OSAPI_POOL_C_ */
