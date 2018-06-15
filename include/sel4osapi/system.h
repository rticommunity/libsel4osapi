/*
 * FILE: system.h - system functions for sel4osapi
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#ifndef SEL4OSAPI_SYSTEM_H_
#define SEL4OSAPI_SYSTEM_H_

/**
 * \brief Type representing the system's basic resources.
 *
 * sel4osapi creates a single instance of this type in each
 * process. The instance contained by the root task is special
 * in that it is actually required to bootstrap the system's
 * basic resources. Instances created within user processes
 * will create interfaces to these resources that rely on
 * the services initialized by the root task's system instance.
 *
 */
typedef struct sel4osapi_system {
    /*
     * Whether the process is the root task or a user process.
     */
    unsigned char is_root;

    /*
     * Environment information associated with the process.
     */
    sel4osapi_process_env_t *env;

    /*
     * Thread object representing the initial thread of the
     * enclosing process.
     */
    sel4osapi_thread_t main_thread;

#ifdef CONFIG_LIB_OSAPI_SYSCLOCK
    /*
     * System clock instance. Initialize only by the root task.
     */
    sel4osapi_sysclock_t sysclock;
#endif

    /*
     * Mutex that can be used by the process' threads
     * to provide mutual exclusion.
     */
    sel4osapi_mutex_t *global_mutex;

    /*
     * Instance of simple_t to access the kernel's functionalities.
     */
    simple_t simple;
    /*
     * Implementation of the Virtual Kernel Allocator interface
     * backed by the system allocator.
     */
    vka_t vka;
    /*
     * System allocator backed by the simple_t instance.
     */
    allocman_t *allocator;
    /*
     * Process virtual memory address space.
     */
    vspace_t vspace;
    /*
     * Vspace management data.
     */
    sel4utils_alloc_data_t vspace_alloc_data;

    /*
     * Virtual memory address of the buffer assigned
     * to the system allocator.
     */
    void *vmem_addr;

    /*
     * Virtual memory reservation (produced by vspace)
     * describing the virtual memory assigned to the system
     * allocator.
     */
    reservation_t vmem_reservation;

    /*
     * Untyped memory objects allocated for all user processes.
     * Sized to possibly stored all untypeds allocated by
     * bootinfo.
     *
     */
    vka_object_t user_untypeds[CONFIG_MAX_NUM_BOOTINFO_UNTYPED_CAPS];
    /*
     * Table storing the pid of the user process to
     * which an untyped is currently allocated to.
     */
    uint8_t user_untypeds_allocation[CONFIG_MAX_NUM_BOOTINFO_UNTYPED_CAPS];
    /*
     * Sizes of each untyped allocated to user processes,
     * stored in number of bits.
     *
     */
    uint8_t user_untypeds_size_bits[CONFIG_MAX_NUM_BOOTINFO_UNTYPED_CAPS];
    /*
     * Number of untyped memory objects actually
     * allocated to user processes.
     */
    uint8_t user_untypeds_num;

    /*
     * CSpace path to Asyncronous Endpoint used to
     * set threads to idle within the process.
     */
    cspacepath_t idling_aep;

#if defined(CONFIG_LIB_OSAPI_NET) || defined(CONFIG_LIB_OSAPI_SERIAL) || defined(CONFIG_LIB_OSAPI_SYSCLOCK)
    ps_io_ops_t io_ops;
#endif

#ifdef CONFIG_LIB_OSAPI_SERIAL
    sel4osapi_serialserver_t serial;
#endif

    sel4osapi_ipcserver_t ipc;

#ifdef CONFIG_LIB_OSAPI_NET
    sel4osapi_netstack_t net;

    sel4osapi_udpstack_t udp;
#endif

    simple_pool_t *processes;



} sel4osapi_system_t;


#define sel4osapi_system_get_instanceI()     (&sel4osapi_gv_system)

extern sel4osapi_system_t sel4osapi_gv_system;

/*
 * @brief Initializes sel4osapi to be used by the system's root_task.
 *
 * This operation initializes the basic system resources that are required
 * by a minimal seL4 system.
 *
 * A new simple_t instance is created using the system's BootInfo
 * (as returned by seL4_GetBootInfo).
 *
 * The system's Virtual Kernel Allocator is configured to use the root
 * cspace, vspace and all the available untypeds.
 *
 * A static buffer must be provided through in order to bootstrap the allocator.
 *
 * A virtual memory buffer will be then allocated in the root vspace to support
 * the VKA.
 *
 * @param [in] bootstrap_mem_pool memory buffer of size SEL4OSAPI_BOOTSTRAP_MEM_POOL_SIZE
 *                                used to bootstrap the system.
 *
 * @return 0 on success
 */
int
sel4osapi_system_initialize(void *bootstrap_mem_pool);

/*
 * @brief Initializes sel4osapi to be used by a user process.
 *
 * This operation should be the first operation invoked by a user process
 * created using sel4osapi.
 *
 * It will use the arguments passed to the process to retrieve the
 * process environment information and it will initialize the basic system
 * resources accordingly.
 *
 */
int
sel4osapi_system_initialize_process(void *bootstrap_mem_pool, int argc, char **argv);

#if 0
/*
 * @brief Access the system singleton instance.
 *
 * This operation does not call sel4osapi_system_initialize.
 */
sel4osapi_system_t*
sel4osapi_system_get_instance();
#endif

/*
 * Block the current thread indefinitely.
 */
void
sel4osapi_idle();

vka_t*
sel4osapi_system_get_vka();

vspace_t*
sel4osapi_system_get_vspace();

simple_t*
sel4osapi_system_get_simple();

#if defined(CONFIG_LIB_OSAPI_SERIAL) || defined(CONFIG_LIB_OSAPI_NET)
ps_io_ops_t*
sel4osapi_system_get_io_ops();
#endif

#ifdef CONFIG_LIB_OSAPI_NET

sel4osapi_netstack_t*
sel4osapi_system_get_netstack();

int
sel4osapi_system_initialize_network(char *iface_name, sel4osapi_netiface_driver_t *iface_driver);
#endif

#endif /* SEL4OSAPI_SYSTEM_H_ */
