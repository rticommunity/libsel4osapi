/*
 * FILE: mutex.c - mutex implementation for sel4osapi
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#include <sel4osapi/osapi.h>

#include <limits.h>

#define WAIT_RESULT_OK          1

sel4osapi_mutex_t*
sel4osapi_mutex_create()
{
    sel4osapi_mutex_t *mutex = NULL;
    vka_t *vka = sel4osapi_system_get_vka();
    int error = 0;

    mutex = (sel4osapi_mutex_t*) sel4osapi_heap_allocate(sizeof(sel4osapi_mutex_t));

    error = vka_alloc_async_endpoint(vka, &mutex->aep);
    assert(error == 0);

    mutex->held = 0;
    mutex->owner = NULL;

    /* Prime the endpoint. */
    seL4_Notify(mutex->aep.cptr, WAIT_RESULT_OK);

    return mutex;
}

void
sel4osapi_mutex_delete(sel4osapi_mutex_t *mutex)
{
    vka_t *vka = sel4osapi_system_get_vka();

    assert(mutex != NULL);

    vka_free_object(vka, &mutex->aep);

    sel4osapi_heap_free(mutex);
}

int
sel4osapi_mutex_lock(sel4osapi_mutex_t *mutex) {
    sel4osapi_thread_info_t *current_thread = sel4osapi_thread_get_current();

    assert(mutex != NULL);

    if (current_thread != mutex->owner) {
        /* We don't already have the mutex. */
        seL4_Wait(mutex->aep.cptr, NULL);
        mutex->owner = current_thread;
    }

    if (mutex->held == UINT_MAX) {
        /* We would overflow if we re-acquired the mutex. Note that we can only
         * be in this branch if we already held the mutex before entering this
         * function, so we don't need to release the mutex here.
         */
        syslog_error_a("maximum number of recursion reached on mutex %x",(unsigned int)mutex);
        return -1;
    }
    mutex->held++;
    return 0;
}

int
sel4osapi_mutex_unlock(sel4osapi_mutex_t *mutex) {
    sel4osapi_thread_info_t *current_thread = sel4osapi_thread_get_current();

    assert(mutex != NULL);
    assert(mutex->owner == current_thread);

    mutex->held--;
    if (mutex->held == 0) {
        /* This was the outermost lock we held. Wake the next person up. */
        mutex->owner = NULL;
        seL4_Notify(mutex->aep.cptr, WAIT_RESULT_OK);
    }

    return 0;
}

