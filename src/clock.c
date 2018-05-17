/*
 * FILE: clock.c - system clock for sel4osapi
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#include <sel4osapi/osapi.h>

#include <vka/capops.h>

struct timeout_entry
{
    seL4_Word caller;
    seL4_CPtr aep;
    seL4_Uint32 periodic;
    seL4_Uint32 period;
    seL4_Uint32 next_event;
};

typedef enum sel4osapi_sysclock_opcode
{
    SYSCLOCK_OP_GET_TIME = 0,
    SYSCLOCK_OP_SET_TIMEOUT = 1,
    SYSCLOCK_OP_CANCEL_TIMEOUT = 2
} sel4osapi_sysclock_opcode_t;

#define ENABLE_TIMEOUT_SERVER   1

void
timer_entry_init(void *el, void *arg)
{
    struct timeout_entry *entry = (struct timeout_entry*) el;
    entry->aep = seL4_CapNull;
    entry->caller = 0;
    entry->period = 0;
    entry->periodic = 0;
}

int
timer_entry_compare(void *el1, void *el2)
{
    struct timeout_entry *e1 = (struct timeout_entry*) el1;
    struct timeout_entry *e2 = (struct timeout_entry*) el2;

    assert(e1 != NULL);
    assert(e2 != NULL);

    if (e1->aep == e2->aep)
    {
        return 0;
    }
    if (e1->aep > e2->aep)
    {
        return -1;
    }

    return 1;
}

seL4_Word
sel4osapi_sysclock_schedule_timeout(
        seL4_Bool periodic, seL4_Uint32 timeout_ms, seL4_CPtr callback_aep)
{
    int error = 0;
    sel4osapi_process_env_t *process = sel4osapi_process_get_current();
    seL4_MessageInfo_t minfo;
    seL4_Word timeout_id;

    minfo = seL4_MessageInfo_new(0,0,1,3);
    seL4_SetCap(0, callback_aep);
    sel4osapi_setMR(0, SYSCLOCK_OP_SET_TIMEOUT);
    sel4osapi_setMR(1, periodic);
    sel4osapi_setMR(2, timeout_ms);

    seL4_Call(process->sysclock_server_ep, minfo);
    error = sel4osapi_getMR(0);
    timeout_id = sel4osapi_getMR(1);

    if (error)
    {
        syslog_info("FAILED set-timeout, aep=%d, t-aep=%d", callback_aep, sel4osapi_thread_get_current()->wait_aep);
        return 0;
    }
    else
    {
        return timeout_id;
    }
}

int
sel4osapi_sysclock_cancel_timeout(seL4_Word timeout_id)
{
    sel4osapi_process_env_t *process = sel4osapi_process_get_current();
    seL4_MessageInfo_t minfo;
    int error = 0;

    minfo = seL4_MessageInfo_new(0,0,0,2);
    sel4osapi_setMR(0, SYSCLOCK_OP_CANCEL_TIMEOUT);
    sel4osapi_setMR(1, timeout_id);
    seL4_Call(process->sysclock_server_ep, minfo);
    error = sel4osapi_getMR(0);

    return error;
}

seL4_Uint32
sel4osapi_sysclock_wait_for_timeout(seL4_Word timeout_id, seL4_CPtr callback_aep, seL4_Bool cancel)
{
    seL4_Word sender_badge;
    seL4_Uint32 wakeup_time;
    seL4_MessageInfo_t reply_minfo = seL4_Recv(callback_aep, &sender_badge);
    assert(seL4_MessageInfo_get_length(reply_minfo) == 1);
    wakeup_time = sel4osapi_getMR(0);

    if (cancel)
    {
        sel4osapi_sysclock_cancel_timeout(timeout_id);
    }

    return wakeup_time;
}

void
sel4osapi_sysclock_server_thread(sel4osapi_thread_info_t *thread)
{
    int error = 0;
    sel4osapi_sysclock_t *sysclock = (sel4osapi_sysclock_t *) thread->arg;
    vka_t *vka = sel4osapi_system_get_vka();

    cspacepath_t caller_ep_path;
    seL4_CPtr caller_ep;

    error = vka_cspace_alloc(vka,&caller_ep);
    assert(error == 0);
    vka_cspace_make_path(vka,caller_ep, &caller_ep_path);
    seL4_SetCapReceivePath(caller_ep_path.root, caller_ep_path.capPtr, caller_ep_path.capDepth);

    while (thread->active)
    {
        seL4_Word sender_badge;
        seL4_MessageInfo_t minfo;
        seL4_Uint32 opcode;

        minfo = seL4_Recv(sysclock->server_ep_obj.cptr, &sender_badge);
        assert(seL4_MessageInfo_get_length(minfo) >= 1);

        opcode = sel4osapi_getMR(0);
        assert(opcode == SYSCLOCK_OP_CANCEL_TIMEOUT || opcode == SYSCLOCK_OP_SET_TIMEOUT || opcode == SYSCLOCK_OP_GET_TIME);

        switch (opcode) {
            case SYSCLOCK_OP_CANCEL_TIMEOUT:
            {
                UNUSED seL4_Uint32 timeout_id;
                int done = 0;

                assert(seL4_MessageInfo_get_length(minfo) == 2);

                timeout_id = sel4osapi_getMR(1);

#if ENABLE_TIMEOUT_SERVER
                error = sel4osapi_mutex_lock(sysclock->timeouts_mutex);
                assert(!error);
                {
                    simple_list_t *entry = sysclock->schedule->entries;

                    while (entry != NULL && !done)
                    {
                        struct timeout_entry *timeout = (struct timeout_entry*) entry->el;

                        if (timeout != NULL && ((seL4_Uint32)timeout->aep == timeout_id))
                        {
                            seL4_CPtr tout_aep = timeout->aep;
                            error = simple_pool_free(sysclock->schedule, timeout);
                            assert(error == 0);
                            cspacepath_t aep_path;
                            vka_cspace_make_path(vka,tout_aep, &aep_path);
                            error = vka_cnode_revoke(&aep_path);
                            assert(error == 0);
                        }
                        else
                        {
                            entry = entry->next;
                        }
                    }
                }
                sel4osapi_mutex_unlock(sysclock->timeouts_mutex);
#endif

                minfo = seL4_MessageInfo_new(0,0,0,1);
                if (done)
                {
                    sel4osapi_setMR(0,0);
                }
                else
                {
                    sel4osapi_setMR(0,1);
                }
                break;
            }
            case SYSCLOCK_OP_SET_TIMEOUT:
            {
                seL4_CPtr reply_aep = seL4_CapNull;
                UNUSED seL4_Uint32 timeout_ms = 0;
                UNUSED seL4_Uint32 periodic = 0;
                seL4_Uint32 insert_time = sysclock->time;

                assert(seL4_MessageInfo_get_extraCaps(minfo) == 1);
                assert(seL4_MessageInfo_get_length(minfo) == 3);

                periodic = sel4osapi_getMR(1);
                timeout_ms = sel4osapi_getMR(2);

#if ENABLE_TIMEOUT_SERVER
                error = sel4osapi_mutex_lock(sysclock->timeouts_mutex);
                assert(!error);
                struct timeout_entry *new_entry = simple_pool_alloc(sysclock->schedule);

                if (new_entry == NULL)
                {
                    syslog_error("Failed to allocate timer entry");
                    error = 1;
                    reply_aep = seL4_CapNull;
                    insert_time = 0;
                    goto prepare_reply;
                }

                assert(new_entry->aep == seL4_CapNull);
                insert_time = sysclock->time;
                new_entry->aep = caller_ep;
                new_entry->caller = sender_badge;
                new_entry->period = timeout_ms;
                new_entry->periodic = periodic;
                new_entry->next_event = insert_time + timeout_ms;

                reply_aep = new_entry->aep;
                error = 0;

        prepare_reply:
                sel4osapi_mutex_unlock(sysclock->timeouts_mutex);
#endif
                minfo = seL4_MessageInfo_new(0,0,0,3);
                sel4osapi_setMR(0, error);
                sel4osapi_setMR(1, reply_aep);
                sel4osapi_setMR(2, insert_time);
                break;
            }
            case SYSCLOCK_OP_GET_TIME :
            {
                minfo = seL4_MessageInfo_new(0,0,0,1);
                sel4osapi_setMR(0, (seL4_Word)sysclock->time);
                break;
            }
        }

        seL4_Reply(minfo);

        if (opcode == SYSCLOCK_OP_SET_TIMEOUT)
        {
            error = vka_cspace_alloc(vka,&caller_ep);
            assert(error == 0);
            vka_cspace_make_path(vka,caller_ep, &caller_ep_path);
            seL4_SetCapReceivePath(caller_ep_path.root, caller_ep_path.capPtr, caller_ep_path.capDepth);
        }
    }
}

uint32_t
sel4osapi_sysclock_get_time()
{
#if 0
    uint32_t current_time = 0;
    seL4_MessageInfo_t msg_info = seL4_MessageInfo_new(0,0,0,1);
    UNUSED seL4_MessageInfo_t reply_tag;
    sel4osapi_process_env_t *process = sel4osapi_process_get_current();

    sel4osapi_setMR(0, SYSCLOCK_OP_GET_TIME);
    
#if defined(CONFIG_ARCH_ARM)
    reply_tag = seL4_CallWithMRs(process->sysclock_server_ep, msg_info, &current_time, seL4_Null, seL4_Null, seL4_Null);
#elif defined(CONFIG_ARCH_X86)
    reply_tag = seL4_CallWithMRs(process->sysclock_server_ep, msg_info, &current_time, NULL);
#endif
    return current_time;
#else
    sel4osapi_process_env_t *process = sel4osapi_process_get_current();
    if (process && process->sysclock_server_ep) {
        printf(">>>> Getting a (good) clock\n");
        seL4_MessageInfo_t msg_info = seL4_MessageInfo_new(0,0,0,1);
        UNUSED seL4_MessageInfo_t reply_tag;

        seL4_SetMR(0, SYSCLOCK_OP_GET_TIME);
        reply_tag = seL4_Call(process->sysclock_server_ep, msg_info);
        printf(">>>> Done, clock=%d", seL4_GetMR(0));
        return seL4_GetMR(0);
    }
    // Clock not initialized
    return 0;

#endif
}

void
sel4osapi_sysclock_timer_thread(sel4osapi_thread_info_t *thread)
{
    int error;
    sel4osapi_sysclock_t *sysclock = (sel4osapi_sysclock_t *)thread->arg;
    vka_t *vka = sel4osapi_system_get_vka();

    //error = timer_periodic(sysclock->native_timer->timer, SEL4OSAPI_SYSCLOCK_PERIOD_MS * 1000 * 1000);
    error = ltimer_set_timeout(&sysclock->native_timer.ltimer, SEL4OSAPI_SYSCLOCK_PERIOD_MS * 1000 * 1000, TIMEOUT_PERIODIC);
    assert(error == 0);

    while (thread->active)
    {
        seL4_Word sender_badge;
        seL4_Wait(sysclock->timer_aep.cptr, &sender_badge);
        sysclock->time += SEL4OSAPI_SYSCLOCK_PERIOD_MS;

#if ENABLE_TIMEOUT_SERVER
        error = sel4osapi_mutex_lock(sysclock->timeouts_mutex);
        assert(!error);
        {
            simple_list_t *entry = sysclock->schedule->entries;

            while (entry != NULL)
            {
                struct timeout_entry *timeout = (struct timeout_entry*) entry->el;
                simple_list_t *next = entry->next;

                if (timeout != NULL && timeout->aep != seL4_CapNull && timeout->next_event <= sysclock->time)
                {
                    seL4_MessageInfo_t msg = seL4_MessageInfo_new(0, 0, 0, 1);
                    seL4_SetMR(0, sysclock->time);
                    seL4_Send(timeout->aep, msg);

                    if (timeout->periodic)
                    {
                        timeout->next_event = sysclock->time + timeout->period;
                    }
                    else
                    {
                        seL4_CPtr tout_aep = timeout->aep;
                        error = simple_pool_free(sysclock->schedule, timeout);
                        assert(error == 0);
                        cspacepath_t aep_path;
                        vka_cspace_make_path(vka,tout_aep, &aep_path);
                        error = vka_cnode_revoke(&aep_path);
                        assert(error == 0);
                    }
                }

                entry = next;
            }

        }
        sel4osapi_mutex_unlock(sysclock->timeouts_mutex);
#endif

        // sel4_timer_handle_single_irq(sysclock->native_timer);
        {
            ps_irq_t irq;
            error = ltimer_get_nth_irq(&sysclock->native_timer.ltimer, 0, &irq);
            assert(!error);
            ltimer_handle_irq(&sysclock->native_timer.ltimer, &irq);
        }
    }
}

extern void doMallocTestNoFree(void);


#if 0
#ifdef CONFIG_ARCH_ARM
seL4_timer_t * sel4osapi_arch_get_default_timer(vka_t *vka, vspace_t *vspace, simple_t *simple, seL4_CPtr notification) {
    seL4_timer_t *retVal;
    retVal = sel4platsupport_get_default_timer(vka, vspace, simple, notification);
    ZF_LOGF_IF(retVal == NULL, "Failed to create default timer");
    return retVal;
}

#elif CONFIG_ARCH_X86
#include <sel4platsupport/device.h>
#include <platsupport/plat/hpet.h>
seL4_timer_t * sel4osapi_arch_get_default_timer(vka_t *vka, vspace_t *vspace, simple_t *simple, seL4_CPtr notification) {
    /* Map the HPET so we can query its proprties */
    seL4_timer_t *retVal;
    vka_object_t frame;
    void *vaddr;
    uintptr_t timer_paddr = 0x11111111;
    vaddr = sel4platsupport_map_frame_at(vka, vspace, timer_paddr, seL4_PageBits, &frame);
    int irq;
    int vector;
    ZF_LOGF_IF(vaddr == NULL, "Failed to map HPET paddr");
    if (!hpet_supports_fsb_delivery(vaddr)) {
        if (!config_set(CONFIG_IRQ_IOAPIC)) {
            ZF_LOGF("HPET does not support FSB delivery and we are not using the IOAPIC");
        }
        uint32_t irq_mask = hpet_ioapic_irq_delivery_mask(vaddr);
        /* grab the first irq */
        irq = FFS(irq_mask) - 1;
    } else {
        irq = -1;
    }
    // vector = DEFAULT_TIMER_INTERRUPT;
    vector = 0; // Default interrupt
    vspace_unmap_pages(vspace, vaddr, 1, seL4_PageBits, VSPACE_PRESERVE);
    vka_free_object(vka, &frame);
    retVal = sel4platsupport_get_hpet_paddr(vspace, simple, vka, timer_paddr, notification, irq, vector);
    ZF_LOGF_IF(retVal == NULL, "Failed to get HPET timer");
    return retVal;
}

#else
#error Unsupported architecture
#endif
#endif


void
sel4osapi_sysclock_initialize(sel4osapi_sysclock_t *sysclock)
{
    vka_t *vka = sel4osapi_system_get_vka();
    simple_t *simple = sel4osapi_system_get_simple();
    vspace_t *vspace = sel4osapi_system_get_vspace();
    sel4osapi_process_env_t *process = sel4osapi_process_get_current();
    int error = 0;
    /*int i = 0;*/

    sysclock->time = 0;

    syslog_trace("Initializing system clock...");
    error = vka_alloc_endpoint(vka,&sysclock->server_ep_obj);
    assert(error == 0);

    syslog_trace("Allocating notification endpoint");
    error = vka_alloc_notification(vka,&sysclock->timer_aep);
    assert(error == 0);

    /*
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    doMallocTestNoFree();
    abort();
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

    syslog_trace("Allocating simple pool for timeouts: max_entries=%d, sizeof(each)=%d", SEL4OSAPI_SYSCLOCK_MAX_ENTRIES, sizeof(struct timeout_entry));
    sysclock->schedule = simple_pool_new(SEL4OSAPI_SYSCLOCK_MAX_ENTRIES,sizeof(struct timeout_entry), timer_entry_init, NULL, timer_entry_compare);
    assert(sysclock->schedule != NULL);

    syslog_trace("Calling get_default_timer...");
    // sysclock->native_timer = sel4osapi_arch_get_default_timer(vka, vspace, simple, sysclock->timer_aep.cptr);
    error = sel4platsupport_init_default_timer(vka, vspace, simple, sysclock->timer_aep.cptr, &sysclock->native_timer);
    assert(error == 0);

    syslog_trace("Creating mutex");
    sysclock->timeouts_mutex = sel4osapi_mutex_create();
    assert(sysclock->timeouts_mutex != NULL);

    syslog_trace("Creating timer thread...");
    sysclock->timer_thread = sel4osapi_thread_create(
                                "sysclock::timer", sel4osapi_sysclock_timer_thread, sysclock, seL4_MaxPrio - 1);
    assert(sysclock->timer_thread);

    syslog_trace("Creating server thread...");
    sysclock->server_thread  = sel4osapi_thread_create(
                                "sysclock::server", sel4osapi_sysclock_server_thread, sysclock, seL4_MaxPrio - 1);
    assert(sysclock->server_thread);

    /* store cap to sysclock in root task's env */
    syslog_trace("Storing sysclock cap in root task's env...");
    {
        cspacepath_t scheduler_ep_path, minted_ep_path;
        error = vka_cspace_alloc(vka, &process->sysclock_server_ep);
        assert(error == 0);
        vka_cspace_make_path(vka,sysclock->server_ep_obj.cptr, &scheduler_ep_path);
        vka_cspace_make_path(vka,process->sysclock_server_ep, &minted_ep_path);
        error = vka_cnode_mint(&minted_ep_path,&scheduler_ep_path,seL4_AllRights, 666);
        assert(error == 0);
    }

    syslog_trace("Sysclock initialized successfully");
}

void
sel4osapi_sysclock_start(sel4osapi_sysclock_t *sysclock)
{
    int error = 0;

    error = sel4osapi_thread_start(sysclock->timer_thread);
    assert(error == 0);

    error = sel4osapi_thread_start(sysclock->server_thread);
    assert(error == 0);
}
