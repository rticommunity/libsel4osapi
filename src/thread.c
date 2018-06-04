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

static void
setup_fault_handler(sel4osapi_thread_t *thread)    // Can be NULL -> Use current thread
{
    int error;
    sel4utils_thread_t fault_handler;
    vka_object_t fault_ep = {0};
    sel4osapi_system_t *env = sel4osapi_system_get_instanceI();
    seL4_CPtr tcbCptr;      // The TCB of the thread we are setting the fault handler

    if (!thread) {
        thread = (sel4osapi_thread_t*)seL4_GetUserData();
    }

    // TCB not specified, use the one from the calling thread
    tcbCptr = thread->native.tcb.cptr; //simple_get_tcb(&env->simple);

    /* create an endpoint */
    if (vka_alloc_endpoint(&env->vka, &fault_ep) != 0) {
        ZF_LOGF("Failed to create fault ep\n");
    }

    /* set the fault endpoint for the current thread */
    error = api_tcb_set_space(tcbCptr, fault_ep.cptr,
                              simple_get_cnode(&env->simple), seL4_NilData, simple_get_pd(&env->simple),
                              seL4_NilData);
    ZF_LOGF_IFERR(error, "Failed to set fault ep for benchmark driver\n");

    error = sel4utils_start_fault_handler(fault_ep.cptr,
                                          &env->vka, &env->vspace, simple_get_cnode(&env->simple),
                                          seL4_NilData,
                                          "sel4osapi-fault-handler", &fault_handler);
    ZF_LOGF_IFERR(error, "Failed to start fault handler");

    error = seL4_TCB_SetPriority(fault_handler.tcb.cptr, 
            simple_get_tcb(&env->simple), // tcbCptr
            thread->info.priority); // seL4_MaxPrio);
    ZF_LOGF_IFERR(error, "Failed to set prio for fault handler");

    if (config_set(CONFIG_KERNEL_RT)) {
        /* give it a sc */
        error = vka_alloc_sched_context(&env->vka, &fault_handler.sched_context);
        ZF_LOGF_IF(error, "Failed to allocate sc");

        error = api_sched_ctrl_configure(simple_get_sched_ctrl(&env->simple, 0),
                                         fault_handler.sched_context.cptr,
                                         100 * US_IN_S, 100 * US_IN_S, 0, 0);
        ZF_LOGF_IF(error, "Failed to configure sc");

        error = api_sc_bind(fault_handler.sched_context.cptr, fault_handler.tcb.cptr);
        ZF_LOGF_IF(error, "Failed to bind sc to fault handler");
    }
}


sel4osapi_thread_t*
sel4osapi_thread_create(
        const char *name, sel4osapi_thread_routine_fn thread_routine, void *thread_arg, int priority)
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
    // seL4_CapData_t data = seL4_CapData_Guard_new(0, seL4_WordBits - env->cspace_size_bits);
    seL4_Word data = seL4_CNode_CapData_new(0, seL4_WordBits - env->cspace_size_bits).words[0];

    // Since verion 9.0 priority cannot be set directly using sel4utils_configure_thread
    // error = sel4utils_configure_thread(vka, vspace, vspace, env->fault_endpoint, env->root_cnode, data, &thread->native);
    //
    sel4utils_thread_config_t config = {0};
    config = thread_config_fault_endpoint(config, env->fault_endpoint);
    config = thread_config_cspace(config, env->root_cnode, data);
    config = thread_config_auth(config, simple_get_tcb(sel4osapi_system_get_simple()));
    config = thread_config_priority(config, (uint8_t)priority);
    config = thread_config_create_reply(config);
    error = sel4utils_configure_thread_config(vka, vspace, vspace, config, &thread->native);

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

    syslog_info("Thread '%s' (ID=%d) created: TCB=%8p (cptr=%08x), pri=%d", name, thread->info.tid, &thread->native.tcb, thread->native.tcb.cptr, priority);

    setup_fault_handler(thread);

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
    syslog_warn("SYSCLOCK not enabled, sel4osapi_thread_sleep is not going to do anything!");
    return 0;
#endif
}
