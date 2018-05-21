/*
 * FILE: thread.h - thread implementation for sel4osapi
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#ifndef SEL4OSAPI_THREAD_H_
#define SEL4OSAPI_THREAD_H_

/*
 * Data structure containing all contextual
 * information pertaining a thread.
 */
typedef struct sel4osapi_thread_info
{
    /*
     * Thread id (unique within enclosing process)
     */
    int tid;
    /*
     * Name of the thread.
     */
    char name[SEL4OSAPI_THREAD_NAME_MAX_LEN];
    /*
     * Argument passed to the thread
     */
    void *arg;
    /*
     * User data
     */
    void *tls;
    /*
     * Whether the thread is active or not.
     * A thread should terminate once this
     * attribute becomes false.
     */
    unsigned char active;
    /*
     * Async endpoint used by the thread to
     * block its execution when required to
     * wait.
     */
    seL4_CPtr wait_aep;
    /*
     * Priority of the thread.
     */
    int priority;
    /*
     * Address of the thread's IPCBuffer.
     * Since we use the IPCBuffer's UserData
     * as thread-specific storage, we need to
     * save the IPCBuffer's address if we
     * want to be able to access it from
     * user applications.
     */
    seL4_IPCBuffer *ipc;
    seL4_Word ipc_word;
} sel4osapi_thread_info_t;

/*
 * Routine executed by a thread.
 */
typedef void (*sel4osapi_thread_routine_fn)( sel4osapi_thread_info_t *thread);

/*
 * Data type representing a thread.
 * This type should not be used directly.
 */
typedef struct sel4osapi_thread
{
    /*
     * Endpoint used by the thread to coordinate
     * its execution with its starting thread.
     */
    vka_object_t local_endpoint;
    /*
     * Async endpoint used to implement
     * thread waits.
     */
    vka_object_t thread_aep;
    /*
     * "Native" thread.
     */
    sel4utils_thread_t native;
    /*
     * Fault endpoint configured for the thread.
     * Set to the process' fault endpoint.
     */
    seL4_CPtr fault_endpoint;
    /*
     * Thread's routine.
     */
    sel4osapi_thread_routine_fn thread_routine;
    /*
     * Thread's context information.
     */
    sel4osapi_thread_info_t info;
} sel4osapi_thread_t;

/*
 * Create a new thread.
 */
sel4osapi_thread_t*
sel4osapi_thread_create(
        const char *name, sel4osapi_thread_routine_fn thread_routine, void *thread_arg, int priority);

/*
 * Start a thread.
 */
int
sel4osapi_thread_start(sel4osapi_thread_t *thread);

/*
 * Wait for a thread to complete.
 */
int
sel4osapi_thread_join(sel4osapi_thread_t *thread);

/*
 * Release all resource associated with a thread.
 */
void
sel4osapi_thread_delete(sel4osapi_thread_t *thread);

/*
 * Access contextual information about the current thread.
 */
sel4osapi_thread_info_t*
sel4osapi_thread_get_current();

seL4_Uint32
sel4osapi_thread_sleep(uint32_t ms);

/* ---------------------------------------------------------------------------
 * FABRIZIO: Those functions are not needed and should be replaced with seL4_GetMR/seL4_SetMR
 */
#define sel4osapi_getMR(_i)     (sel4osapi_thread_get_current()->ipc->msg[_i])

#define sel4osapi_setMR(_i, _val)\
{\
    sel4osapi_thread_get_current()->ipc->msg[_i] = _val;\
}

/* --------------------------------------------------------------------------- */

#ifdef WRAP_SYSCALLS
static inline seL4_MessageInfo_t
sel4osapi_Call(seL4_CPtr dest, seL4_MessageInfo_t minfo)
{
    /* restore IPCBuffer */
    seL4_Word udata = seL4_GetUserData();
    seL4_Word ipc = sel4osapi_thread_get_current()->ipc_word;
    assert(udata != 0);
    assert(ipc != 0);
    seL4_SetUserData(ipc);
    minfo = seL4_Call(dest, minfo);
    /* restore sel4osapi data */
    seL4_SetUserData(udata);
    return minfo;
}

static inline seL4_MessageInfo_t
sel4osapi_Wait(seL4_CPtr src, seL4_Word *sender_badge)
{
    /* restore IPCBuffer */
    seL4_Word udata = seL4_GetUserData();
    seL4_Word ipc = sel4osapi_thread_get_current()->ipc_word;
    assert(udata != 0);
    assert(ipc != 0);
    seL4_SetUserData(ipc);
    seL4_MessageInfo_t minfo = seL4_Wait(src, sender_badge);
    /* restore sel4osapi data */
    seL4_SetUserData(udata);
    return minfo;
}
#else

/*
#define sel4osapi_Call      seL4_Call
#define sel4osapi_Reply      seL4_Reply
*/

#endif
#endif /* SEL4OSAPI_THREAD_H_ */
