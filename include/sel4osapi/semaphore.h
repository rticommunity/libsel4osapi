/*
 * FILE: semaphore.h - semaphore implementation for sel4osapi
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#ifndef SEL4OSAPI_SEMAPHORE_H_
#define SEL4OSAPI_SEMAPHORE_H_

#include "sync/sem.h"

extern vka_t* sel4osapi_system_get_vka();

typedef sync_sem_t sel4osapi_semaphore_t;

static inline sel4osapi_semaphore_t* sel4osapi_semaphore_create(int init) {
    sync_sem_t * sem = (sync_sem_t *) sel4osapi_heap_allocate(sizeof(sync_sem_t));
    assert(sync_sem_new(sel4osapi_system_get_vka(), sem, init) == 0);
    return sem;
}

static inline void sel4osapi_semaphore_delete(sel4osapi_semaphore_t* sem) {
    sync_sem_destroy(sel4osapi_system_get_vka(), (sync_sem_t *)sem);
}

static inline int sel4osapi_semaphore_take(sel4osapi_semaphore_t* sem, UNUSED int32_t timeout) {
    return sync_sem_wait(sem);
}

static inline int sel4osapi_semaphore_give(sel4osapi_semaphore_t* sem) {
    return sync_sem_post(sem);
}




#endif /* SEL4OSAPI_SEMAPHORE_H_ */
