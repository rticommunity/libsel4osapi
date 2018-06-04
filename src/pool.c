/*
 * FILE: pool.c - simple object pool implementation
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#include <sel4osapi/osapi.h>

simple_pool_t*
simple_pool_new(int size, size_t el_size, simple_pool_init_el_fn init_fn, void *init_arg, simple_pool_compare_el_fn compare_fn)
{
    simple_pool_t *pool = NULL;

    assert(size > 0);
    assert(el_size > 0);

    pool = (simple_pool_t *) malloc(sizeof(simple_pool_t) + 64);
    assert(pool != NULL);

    pool->el_size = el_size;
    pool->size = size;
    pool->free_entries = NULL;
    pool->entries = NULL;
    pool->init_fn = init_fn;
    pool->init_arg = init_arg;
    pool->compare_fn = compare_fn;
    pool->current_size = 0;

    int i = 0;

    for (i = 0; i < pool->size; ++i) {
        void *el = malloc(pool->el_size + 64);
        assert(el != NULL);
        if (pool->init_fn)
        {
            pool->init_fn(el, pool->init_arg);
        }
        pool->free_entries = sel4osapi_list_insert(pool->free_entries, el);
    }

    return pool;
}


void*
simple_pool_alloc(simple_pool_t *pool)
{
    sel4osapi_list_t *entry = simple_pool_alloc_entry(pool);
    if (entry == NULL)
    {
        return NULL;
    }
    return entry->el;
}

sel4osapi_list_t*
simple_pool_alloc_entry(simple_pool_t *pool)
{
    sel4osapi_list_t *entry = NULL;

    assert(pool != NULL);

    entry = pool->free_entries;

    if (entry == NULL)
    {
        return NULL;
    }

    /*printf("### [POOL=%x][ALLOC][ENTRY=%x][SIZE=%d] &&&\n", (unsigned int) pool, (unsigned int) entry, pool->current_size);*/

    assert(entry->el != NULL);

    pool->free_entries = sel4osapi_list_unlink(pool->free_entries, entry);

    if (pool->init_fn)
    {
        pool->init_fn(entry->el, pool->init_arg);
    }

    pool->entries = sel4osapi_list_insert_node(pool->entries, entry);
    pool->current_size++;

    assert(pool->current_size <= pool->size);

    return entry;
}

int
simple_pool_free(simple_pool_t *pool, void *el)
{
    sel4osapi_list_t *entry = NULL;
    assert(pool != NULL);
    assert(el != NULL);

    entry = simple_pool_find_node(pool, el);
    if (entry == NULL)
    {
        return -1;
    }
    return simple_pool_free_entry(pool, entry);
}

int
simple_pool_free_entry(simple_pool_t *pool, sel4osapi_list_t *entry)
{
    assert(pool != NULL);
    assert(entry != NULL);

    /*printf("### [POOL=%x][FREE][ENTRY=%x][SIZE=%d] &&&\n", (unsigned int) pool, (unsigned int) entry, pool->current_size);*/

    pool->entries = sel4osapi_list_unlink(pool->entries, entry);
    pool->current_size--;

    assert(pool->current_size >= 0);

    if (pool->init_fn)
    {
        pool->init_fn(entry->el, pool->init_arg);
    }

    pool->free_entries = sel4osapi_list_insert_node(pool->free_entries, entry);

    return 0;
}


sel4osapi_list_t*
simple_pool_find_node(simple_pool_t *pool, void *el)
{
    sel4osapi_list_t *cursor = pool->entries;
    sel4osapi_list_t *entry = NULL;

    while (cursor != NULL && entry == NULL)
    {
        if (pool->compare_fn)
        {
            if (pool->compare_fn(cursor->el, el) == 0)
            {
                entry = cursor;
            }
        }
        else
        {
        /*        printf("comparing %x and el=%x\n", (unsigned int)cursor->el, (unsigned int)el);*/
            if (cursor->el == el)
            {
                entry = cursor;
            }
        }
        cursor = cursor->next;
    }

    return entry;
}

int
simple_pool_get_current_size(simple_pool_t *pool)
{
    return pool->current_size;
}

