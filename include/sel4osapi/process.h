/*
 * FILE: process.h - process abstraction for sel4osapi
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#ifndef SEL4OSAPI_PROCESS_H_
#define SEL4OSAPI_PROCESS_H_

#define SEL4OSAPI_USER_PROCESS_NAME_MAX_LEN     15


/*
 * Data structure representing a process' environment
 * information. This information includes mostly useful
 * capabilities to resources that are used by
 * sel4osapi/applications.
 *
 * The capabilities are always relative to the root
 * cnode of the process.
 */
typedef struct sel4osapi_process_env {
    /*
     * Name of the process.
     */
    char name[SEL4OSAPI_USER_PROCESS_NAME_MAX_LEN];
    /*
     * Process id.
     */
    unsigned int pid;
    /*
     * Page directory used by the process' VSpace
     */
    seL4_CPtr page_directory;
    /*
     * Root cnode of the process' CSpace.
     */
    seL4_CPtr root_cnode;
    /*
     * Size, in bits, of the process' CSpace
     */
    seL4_Word cspace_size_bits;
    /*
     * TCB of the process' initial thread.
     */
    seL4_CPtr tcb;
    /*
     * Range of free capabilities in the
     * process' CSpace.
     */
    seL4_SlotRegion free_slots;
    /*
     * Range of capabilities to untyped
     * objects assigned to the process.
     */
    seL4_SlotRegion untypeds;
    /*
     * Array of indices of the untyped objects
     * that the root task assigned to the process.
     */
    uint8_t untyped_indices[CONFIG_MAX_NUM_BOOTINFO_UNTYPED_CAPS];
    /*
     * Size, in bits, of each untyped object
     * assigned to the process.
     */
    uint8_t untyped_size_bits_list[CONFIG_MAX_NUM_BOOTINFO_UNTYPED_CAPS];
    /*
     * Endpoint used as fault endpoint by the
     * process' initial thread.
     */
    seL4_CPtr fault_endpoint;
    /*
     * Endpoint to the sysclock instance.
     */
    seL4_CPtr sysclock_server_ep;
    /*
     * Async Endpoint used to block the process'
     * threads in idling mode.
     */
    seL4_CPtr idling_aep;

    /*
     * Threads created by this process
     */
    simple_pool_t *threads;

    /*
     * Priority of the process.
     */
    uint8_t priority;

    sel4osapi_ipcclient_t ipcclient;

#ifdef CONFIG_LIB_OSAPI_NET
    sel4osapi_udp_interface_t udp_iface;
#endif

#ifdef CONFIG_LIB_OSAPI_SERIAL
    sel4osapi_serialclient_t serial;
#endif

} sel4osapi_process_env_t;

/*
 * Type representing a process.
 * By process, we mean a thread which has its own
 * VSpace and CSpace (which can be shared with other
 * threads spawned within the context of the process).
 *
 * The data structure is not meant to be used directly.
 */
typedef struct sel4osapi_process {
    sel4utils_process_t native;
    sel4osapi_process_env_t *env;
    void *env_remote_vaddr;
    seL4_CPtr parent_idling_aep;
    sel4osapi_ipcclient_t *ipcclient;

#ifdef CONFIG_LIB_OSAPI_SERIAL
    sel4osapi_serialclient_t *serialclient;
#endif

} sel4osapi_process_t;

/*
 * Create a new process, loading the specified ELF file
 * from a CPIO archive and configuring it with the specified
 * priority.
 * This operation should only be called within the context of
 * the root task.
 *
 */
sel4osapi_process_t*
sel4osapi_process_create(char *process_name, uint8_t priority);

#if 0
sel4osapi_process_t*
sel4osapi_process_fork(char *process_name, uint8_t priority, void *entry_point);
#endif

/*
 * Start a process.
 * This operation will block to send the process' environment
 * via endpoint, so it is important that the new process calls
 * sel4osapi_system_initialize_process() in order to properly
 * receive this data and unblock the root task.
 */
void
sel4osapi_process_spawn(sel4osapi_process_t *process);

/*
 * Wait for a process to terminate.
 */
int
sel4osapi_process_join(sel4osapi_process_t *process);

/*
 * Release all resources associated with a process.
 */
void
sel4osapi_process_delete(sel4osapi_process_t *process);

/*
 * Print a process' environment information to stdout.
 */
void
sel4osapi_process_print_env(sel4osapi_process_env_t *env);

/*
 * Access the enclosing process' environment information.
 */
sel4osapi_process_env_t*
sel4osapi_process_get_current();

/*
 * Signal root task that a process has terminated.
 * This operation should be the last operation called by a process,
 * since control will not be returned to the caller
 * by invoking sel4osapi_idle().
 */
void
sel4osapi_process_signal_root(sel4osapi_process_env_t *process, int exit_code);

/*
 * Copy a capability into a process' CSpace.
 * This operation will mint the specified cap with a
 * badge corresponding to the process' pid.
 */
seL4_CPtr
sel4osapi_process_copy_cap_into(sel4osapi_process_t *process, vka_t *parent_vka, seL4_CPtr cap, seL4_CapRights_t rights);

#endif /* SEL4OSAPI_PROCESS_H_ */
