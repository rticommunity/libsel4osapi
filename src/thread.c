/*
 * FILE: thread.c - thread implementation for sel4osapi
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#include <sel4osapi/osapi.h>

static void
sel4osapi_thread_init_tls(sel4osapi_thread_t *thread)
{

    thread->info.ipc_word = seL4_GetUserData();
    assert(thread->info.ipc_word != 0);
    seL4_SetUserData((seL4_Word)thread);
}

sel4osapi_thread_t*
sel4osapi_thread_create(
        char *name, sel4osapi_thread_routine_fn thread_routine, void *thread_arg, int priority)
{
    UNUSED int error;
    vka_t *vka = sel4osapi_system_get_vka();
    vspace_t *vspace = sel4osapi_system_get_vspace();
    sel4osapi_process_env_t *env = sel4osapi_process_get_current();
    sel4osapi_thread_t *thread = NULL;
    int tid;

    thread = (sel4osapi_thread_t*) simple_pool_alloc(env->threads);
    assert(thread != NULL);
    tid = simple_pool_get_current_size(env->threads);

    error = vka_alloc_endpoint(vka, &thread->local_endpoint);
    assert(error == 0);

    error = vka_alloc_notification(vka, &thread->thread_aep);
    assert(error == 0);


    thread->fault_endpoint = env->fault_endpoint;
    seL4_CapData_t data = seL4_CapData_Guard_new(0, seL4_WordBits - env->cspace_size_bits);
    error = sel4utils_configure_thread(vka, vspace, vspace, env->fault_endpoint,
                                      priority, env->root_cnode, data, &thread->native);
    assert(error == 0);

    thread->thread_routine = thread_routine;

    thread->info.priority = priority;

    thread->info.arg = thread_arg;
    if (name != NULL)
    {
        snprintf(thread->info.name,SEL4OSAPI_THREAD_NAME_MAX_LEN,"%s",name);
    }
    else
    {
        snprintf(thread->info.name,SEL4OSAPI_THREAD_NAME_MAX_LEN,"p-%02d-t-%02d",env->pid, tid);
    }
    thread->info.tls = NULL;
    thread->info.tid = tid;

    thread->info.wait_aep = thread->thread_aep.cptr;
    thread->info.ipc = (seL4_IPCBuffer*) thread->native.ipc_buffer_addr;

    /*env->threads_num++;*/

    return thread;
}

void
sel4osapi_thread_delete(sel4osapi_thread_t *thread)
{
    vka_t *vka = sel4osapi_system_get_vka();
    vspace_t *vspace = sel4osapi_system_get_vspace();
    vka_free_object(vka, &thread->local_endpoint);
    sel4utils_clean_up_thread(vka, vspace, &thread->native);
}


void sel4osapi_thread_routine_wrapper(void *arg1, void *arg2, UNUSED void *ipc_buf) {
    sel4osapi_thread_t *thread = (sel4osapi_thread_t *) arg1;

    sel4osapi_thread_init_tls(thread);

    thread->info.active = 1;

    thread->thread_routine(&thread->info);

    thread->info.active = 0;

    seL4_MessageInfo_t info = seL4_MessageInfo_new(0, 0, 0, 1);
    seL4_SetMR(0, 0);
    seL4_Call(thread->local_endpoint.cptr, info);

    /* SHOULD NEVER GET HERE */
    assert(0);
    while (1) { seL4_Yield(); };
}

int
sel4osapi_thread_start(sel4osapi_thread_t *thread)
{
    int error = 0;

    error = sel4utils_start_thread(&thread->native, sel4osapi_thread_routine_wrapper, thread, NULL, 1);
    assert(error  == 0);

    return seL4_NoError;
}

int
sel4osapi_thread_join(sel4osapi_thread_t *thread)
{
    seL4_Word badge;
    seL4_Wait(thread->local_endpoint.cptr, &badge);
    return seL4_GetMR(0);
}


sel4osapi_thread_info_t*
sel4osapi_thread_get_current()
{
    sel4osapi_thread_t *self = (sel4osapi_thread_t*) seL4_GetUserData();
    return &self->info;
}

seL4_Uint32
sel4osapi_thread_sleep(uint32_t ms)
{
#ifdef CONFIG_LIB_OSAPI_SYSCLOCK
    sel4osapi_thread_info_t *thread = sel4osapi_thread_get_current();
    seL4_Word timeout_id = sel4osapi_sysclock_schedule_timeout(0, ms, thread->wait_aep);
    assert(timeout_id != seL4_CapNull);
    return sel4osapi_sysclock_wait_for_timeout(timeout_id, thread->wait_aep, 0);
#else
    return 0;
#endif
}
