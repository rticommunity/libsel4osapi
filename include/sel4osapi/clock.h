/*
 * FILE: clock.h - system clock for sel4osapi
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#ifndef SEL4OSAPI_CLOCK_H_
#define SEL4OSAPI_CLOCK_H_

#define SEL4OSAPI_SYSCLOCK_MAX_ENTRIES  100

/*
 * Type representing the system clock. Not meant to
 * be used directly by applications.
 *
 * The clock is a singleton created by the root task.
 *
 * It spawns two threads:
 *  - One thread registers on the hardware platform and
 *    updates the system's time at fixed time intervals.
 *  - Another thread serves requests for current time to
 *    user processes by listening on an EP that is minted
 *    into each process CSpace.
 *
 */
typedef struct sel4osapi_sysclock
{
    seL4_timer_t native_timer;
    uint32_t time;
    sel4osapi_thread_t *timer_thread;
    vka_object_t timer_aep;
    sel4osapi_thread_t *server_thread;
    vka_object_t server_ep_obj;
    sel4osapi_mutex_t *timeouts_mutex;

    /*simple_list_t *timeouts;
    simple_list_t *free_entries;*/

    simple_pool_t *schedule;

} sel4osapi_sysclock_t;

/*
 * Initialize the sysclock singleton instance.
 *
 * Only meant to be called by sel4osapi_system_initialize().
 */
void
sel4osapi_sysclock_initialize(sel4osapi_sysclock_t *sysclock);

/*
 * Start the sysclock. This will start the associated threads.
 *
 * Only meant to be called by sel4osapi_system_initialize().
 */
void
sel4osapi_sysclock_start(sel4osapi_sysclock_t *sysclock);

/*
 * Return the current system time in milliseconds.
 *
 * The time is relative to time when the sysclock was started.
 */
uint32_t
sel4osapi_sysclock_get_time();

seL4_Word
sel4osapi_sysclock_schedule_timeout(
        seL4_Bool periodic, seL4_Uint32 timeout_ms, seL4_CPtr callback_aep);

int
sel4osapi_sysclock_cancel_timeout(seL4_Word timeout_id);

seL4_Uint32
sel4osapi_sysclock_wait_for_timeout(seL4_Word timeout_id, seL4_CPtr callback_aep, seL4_Bool cancel);

#endif /* CLOCK_H_ */
