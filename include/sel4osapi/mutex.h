/*
 * FILE: mutex.h - mutex implementation for sel4osapi
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#ifndef SEL4OSAPI_MUTEX_H_
#define SEL4OSAPI_MUTEX_H_


#include <sel4osapi/thread.h>
#include "sync/recursive_mutex.h"

extern vka_t* sel4osapi_system_get_vka();

typedef sync_recursive_mutex_t sel4osapi_mutex_t;

static inline sel4osapi_mutex_t *sel4osapi_mutex_create() {
    
    sync_recursive_mutex_t *mutex = (sync_recursive_mutex_t *) sel4osapi_heap_allocate(sizeof(sync_recursive_mutex_t));
    assert(mutex);
    assert(sync_recursive_mutex_new(sel4osapi_system_get_vka(), mutex) == 0);
    return (sel4osapi_mutex_t *)mutex;
}

static inline void sel4osapi_mutex_delete(sel4osapi_mutex_t *mutex) {
    sync_recursive_mutex_destroy(sel4osapi_system_get_vka(), (sync_recursive_mutex_t *)mutex);
}

static inline int sel4osapi_mutex_lock(sel4osapi_mutex_t *mutex) {
    return sync_recursive_mutex_lock((sync_recursive_mutex_t *)mutex);
}

static inline int sel4osapi_mutex_unlock(sel4osapi_mutex_t *mutex) {
    return sync_recursive_mutex_unlock((sync_recursive_mutex_t *)mutex);
}

#endif /* SEL4OSAPI_MUTEX_H_ */
