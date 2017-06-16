/*
 * FILE: semaphore.c - semaphore implementation for sel4osapi
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#include <sel4osapi/osapi.h>

#define WAIT_RESULT_OK          0
#define WAIT_RESULT_TIMEOUT     2

sel4osapi_semaphore_t*
sel4osapi_semaphore_create(int init)
{
    sel4osapi_semaphore_t *semaphore = NULL;
    int i = 0;

    semaphore = (sel4osapi_semaphore_t*) sel4osapi_heap_allocate(sizeof(sel4osapi_semaphore_t));
    assert(semaphore != NULL);

    semaphore->mutex = sel4osapi_mutex_create();
    assert(semaphore->mutex);

    for (i = 0; i < SEL4OSAPI_SEMAPHORE_QUEUE_SIZE; ++i) {
        semaphore->wait_queue[i] = seL4_CapNull;
    }
    semaphore->queued = 0;

    semaphore->count = init?1:0;

    return semaphore;
}

void
sel4osapi_semaphore_delete(sel4osapi_semaphore_t* semaphore)
{
    sel4osapi_mutex_delete(semaphore->mutex);
    sel4osapi_heap_free(semaphore);
}

int
sel4osapi_semaphore_take(sel4osapi_semaphore_t* semaphore, int32_t timeout_ms)
{
    sel4osapi_thread_info_t *current_thread = sel4osapi_thread_get_current();
    int error = 0;

    assert(semaphore != NULL);

    error = sel4osapi_mutex_lock(semaphore->mutex);
    assert(!error);

    if (timeout_ms == 0) {
        if (semaphore->count == 0) {
            sel4osapi_mutex_unlock(semaphore->mutex);
            return -1;
        }
    }

    assert(timeout_ms != 0 || semaphore->count != 0);

    while (semaphore->count == 0)
    {
#ifdef CONFIG_LIB_OSAPI_SYSCLOCK
        seL4_Word timeout_id = seL4_CapNull;
#endif
        int queued = 0;
        int wait_result = 0;
        int i = 0;
        for (i = 0; i < SEL4OSAPI_SEMAPHORE_QUEUE_SIZE && !queued; ++i) {
            if (semaphore->wait_queue[i] == seL4_CapNull)
            {
                semaphore->wait_queue[i] = current_thread->wait_aep;
                semaphore->queued++;
                queued = i+1;

                syslog_trace_a("queued thread %s on semaphore %x", current_thread->name, (unsigned int)semaphore);

#ifdef CONFIG_LIB_OSAPI_SYSCLOCK
                if (timeout_ms > 0)
                {
                    timeout_id = sel4osapi_sysclock_schedule_timeout(0, timeout_ms, current_thread->wait_aep);
                    assert(timeout_id != seL4_CapNull);
                    syslog_trace_a("scheduled timeout (%dms) for thread %s on semaphore %x", timeout_ms, current_thread->name, (unsigned int)semaphore);
                }
#endif
            }
        }

        assert(queued > 0);
        sel4osapi_mutex_unlock(semaphore->mutex);

        seL4_Wait(current_thread->wait_aep, NULL);
        wait_result = sel4osapi_getMR(0);

        error = sel4osapi_mutex_lock(semaphore->mutex);
        assert(!error);

        semaphore->wait_queue[queued - 1] = seL4_CapNull;

        syslog_trace_a("thread %s woke up on semaphore %x", current_thread->name, (unsigned int)semaphore);

#ifdef CONFIG_LIB_OSAPI_SYSCLOCK
        if (timeout_id != seL4_CapNull)
        {
            sel4osapi_sysclock_cancel_timeout(timeout_id);
        }
#endif

        if (wait_result != WAIT_RESULT_OK)
        {
            sel4osapi_mutex_unlock(semaphore->mutex);
            syslog_trace_a("thread %s TIMED OUT on semaphore %x (result=%d)", current_thread->name, (unsigned int)semaphore, wait_result);
            return -1;
        }
    }

    semaphore->count = 0;

    sel4osapi_mutex_unlock(semaphore->mutex);

    syslog_trace_a("thread %s took semaphore %x", current_thread->name, (unsigned int)semaphore);

    return 0;
}

int
sel4osapi_semaphore_give(sel4osapi_semaphore_t* semaphore)
{
    int error = sel4osapi_mutex_lock(semaphore->mutex);
    assert(!error);

    semaphore->count = 1;

    int i = 0;
    for (i = 0; i < SEL4OSAPI_SEMAPHORE_QUEUE_SIZE; ++i) {
        if (semaphore->wait_queue[i] != seL4_CapNull)
        {
            seL4_SendWithMRs(semaphore->wait_queue[i], 
                    seL4_MessageInfo_new(0, 0, 0, 1), 
                    WAIT_RESULT_OK,
                    seL4_Null,
                    seL4_Null,
                    seL4_Null);
        }
    }

    sel4osapi_mutex_unlock(semaphore->mutex);
    return 0;
}
