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

#ifdef CONFIG_LIB_OSAPI_CUSTOM_SYNC

#include <sel4osapi/mutex.h>

#define SEL4OSAPI_SEMAPHORE_QUEUE_SIZE  (SEL4OSAPI_MAX_THREADS_PER_PROCESS + 1)

#define SEL4OSAPI_SEMAPHORE_TIMEOUT_INFINITE   -1

/*
 * Data type representing a binary semaphore.
 */
typedef struct sel4osapi_semaphore
{
    unsigned int count;
    sel4osapi_mutex_t *mutex;
    seL4_CPtr wait_queue[SEL4OSAPI_SEMAPHORE_QUEUE_SIZE];
    unsigned int queued;
} sel4osapi_semaphore_t;

/*
 * Create a new semaphore.
 */
sel4osapi_semaphore_t*
sel4osapi_semaphore_create(int init);

/*
 * Delete a semaphore.
 */
void
sel4osapi_semaphore_delete(sel4osapi_semaphore_t* semaphore);

/*
 * Take the semaphore or block up to the specified timeout.
 * Currently only timeout=0 is supported. Other values will
 * be ignored.
 */
int
sel4osapi_semaphore_take(sel4osapi_semaphore_t* semaphore, int32_t timeout);

/*
 * Give the semaphore.
 */
int
sel4osapi_semaphore_give(sel4osapi_semaphore_t* semaphore);

#else
// *****************************************************************************
// ** seL4_libs/libsel4sync implementation
// *****************************************************************************

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
#endif




#endif /* SEL4OSAPI_SEMAPHORE_H_ */
