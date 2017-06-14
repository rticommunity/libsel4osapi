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

/*
 * Data type representing a recursive mutex.
 */
typedef struct sel4osapi_mutex
{
    unsigned int held;
    sel4osapi_thread_info_t *owner;
    vka_object_t aep;
} sel4osapi_mutex_t;

/*
 * Create a new mutex.
 */
sel4osapi_mutex_t*
sel4osapi_mutex_create();

/*
 * Delete an existing mutex.
 */
void
sel4osapi_mutex_delete(sel4osapi_mutex_t *mutex);

/*
 * Take the lock on a mutex.
 * This implementation of mutex is recursive.
 * As such, an application may call lock()
 * multiple times (up to UINT_MAX), but it
 * will be responsible for releasing it (by
 * means of unlock()) a corresponding number
 * of times.
 */
int
sel4osapi_mutex_lock(sel4osapi_mutex_t *mutex);

/*
 * Release the lock on a mutex.
 */
int
sel4osapi_mutex_unlock(sel4osapi_mutex_t *mutex);


#endif /* SEL4OSAPI_MUTEX_H_ */
