/*
 * FILE: system.c - system functions for sel4osapi
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#include <sel4osapi/osapi.h>
// #include <sel4platsupport/bootinfo.h>

#include <limits.h>
#include <string.h>
#include <simple-default/simple-default.h>
#include <allocman/bootstrap.h>
#include <allocman/vka.h>
#include <sel4platsupport/bootinfo.h>

#if 0
#if defined(CONFIG_PLAT_IMX6)
#include <sel4platsupport/mach/epit.h>
#else
#include <sel4platsupport/timer.h>
#endif
#endif
/*
 * Maximum number of untypeds allocated to the root task.
 */
#define ROOT_TASK_NUM_UNTYPEDS 16

sel4osapi_system_t sel4osapi_gv_system;

UNUSED static char malloc_static_mem_pool[SEL4OSAPI_BOOTSTRAP_MEM_POOL_SIZE];
sel4utils_res_t muslc_brk_reservation_memory;

extern vspace_t *muslc_this_vspace;
extern reservation_t muslc_brk_reservation;
extern void *muslc_brk_reservation_start;

extern char *morecore_area;
extern size_t morecore_size;

#if 0
sel4osapi_system_t*
sel4osapi_system_get_instanceI()
{
    return &sel4osapi_gv_system;
}


#define MEMTEST_BLOCK_MAX   2000
#define MEMTEST_BLOCK_SIZE  16
void doMallocTestNoFree() {
    int i, j;
    char *memBlock;
    
    printf("=[MallocTest]====================================================================================\n");
    // Allocate MEMTEST_BLOCK_MAX blocks
    for (i = 0; i < MEMTEST_BLOCK_MAX; ++i) {
        // printf("\t#%02d...", i);
        memBlock = malloc(MEMTEST_BLOCK_SIZE);
        //printf("%p\n", memBlock);
        for (j = 0; j < MEMTEST_BLOCK_SIZE; ++j) {
            memBlock[j] = 0xab;
        }
    }
    printf("===================================================================================[Malloc Test]=\n");
}

#endif

/*
 * Bootstrap basic system objects inside the root_task.
 */
static void
sel4osapi_system_bootstrap_root(sel4osapi_system_t *system, void *bootstrap_mem_pool)
{
    int error = 0;
    seL4_BootInfo *info = platsupport_get_bootinfo(); // seL4_GetBootInfo();

    /* mark the system singleton as the root_task */
    system->is_root = 1;

    /* Assign a static buffer to malloc(), since
     * we don't have virtual memory yet. The size of
     * this buffer must be SEL4OSAPI_BOOTSTRAP_MEM_POOL_SIZE */
    syslog_trace("Configuring morecore static area: %d Kb", SEL4OSAPI_BOOTSTRAP_MEM_POOL_SIZE/1024);
    morecore_area = malloc_static_mem_pool;
    morecore_size = SEL4OSAPI_BOOTSTRAP_MEM_POOL_SIZE;

    /* Create an instance of simple_t interface
     * using the bootinfo data provided by kernel */
    syslog_trace("Initializing simple_default");
    simple_default_init_bootinfo(&system->simple, info);

    /* create a system allocator which will rely on
     * the simple_t instance to request kernel services.
     * The allocator uses the CSpace and VSpace of the
     * root_task, and all the available Untyped objects */
    syslog_trace("Retrieving allocator...");
    system->allocator = bootstrap_use_current_simple(
            &system->simple, SEL4OSAPI_BOOTSTRAP_MEM_POOL_SIZE,
            bootstrap_mem_pool);
    assert(system->allocator);

    /* create a vka (interface for interacting with the underlying allocator) */
    syslog_trace("Building VKA...");
    allocman_make_vka(&system->vka, system->allocator);

    /* create a vspace_t object to manage our VSpace.
     * A vspace_t is an interface to manage a Virtual Page Table which keeps track of which
     * frame if any a virtual memory address is currently mapped to.
     * The table is stored at the top of the virtual memory, right before
     * the kernel's reserved address range.
     * The address space available to the system is 0x00001000 to 0xdf7fefff, but
     * the vspace_t will start mapping pages at 0x10000000 if not specified with
     * a specific vmem address when requested to map one or more pages.
     * Pages are 4K each.
     */
    syslog_trace("Getting vspace...");
    error = sel4utils_bootstrap_vspace_with_bootinfo_leaky(&system->vspace,
        &system->vspace_alloc_data, simple_get_pd(&system->simple), &system->vka, info);
    assert(error == 0);

    /* Fill the system allocator with virtual memory.
     * We request the vspace to serve a contiguous range of addresses
     * of size SEL4OSAPI_SYSTEM_VMEM_SIZE. The vspace will not allocate
     * pages but simply mark the address range as RESERVED.
     */
    syslog_trace("Reserving space for HEAP = %d Kb", SEL4OSAPI_VKA_VMEM_SIZE/1024);
    system->vmem_reservation = vspace_reserve_range(&system->vspace,
        SEL4OSAPI_VKA_VMEM_SIZE, seL4_AllRights, 1, &system->vmem_addr);
    assert(system->vmem_reservation.res);

    /* Assign the reserved chunk of virtual memory to the system allocator.
     * The system allocator doesn't rely on the vspace_t to manage virtual memory.
     * Instead, the allocator will create a new page table kernel object an
     * manually map pages into it at the addresses of the specified range.
     */
    syslog_trace("Configuring virtual pool...");
    bootstrap_configure_virtual_pool(system->allocator, system->vmem_addr,
        SEL4OSAPI_VKA_VMEM_SIZE, simple_get_pd(&system->simple));

    /* Reserve and assign a chunk of virtual memory to the root_task's
     * malloc(). This virtual memory will also be manually managed by
     * the muslc implementation, we only set some global variables.
     */
    syslog_trace("Allocating %d Kb for system vmem", SEL4OSAPI_SYSTEM_VMEM_SIZE / 1024);
    error =sel4utils_reserve_range_no_alloc(&system->vspace,
                &muslc_brk_reservation_memory, SEL4OSAPI_SYSTEM_VMEM_SIZE, seL4_AllRights, 1, &muslc_brk_reservation_start);
    assert(error == 0);
    muslc_this_vspace = &system->vspace;
    muslc_brk_reservation.res = &muslc_brk_reservation_memory;
    morecore_area = NULL;
    morecore_size = 0;
    syslog_trace("Morecore configured as dynamic, init done!");
}

/*
 * Bootstrap basic system objects inside a user process.
 */
static void
sel4osapi_system_bootstrap_user(sel4osapi_system_t *system, void *bootstrap_mem_pool, sel4osapi_process_env_t *env)
{
    int error = 0;

    morecore_area = malloc_static_mem_pool;
    morecore_size = SEL4OSAPI_BOOTSTRAP_MEM_POOL_SIZE;

    system->is_root = 0;
    system->env = env;

    system->allocator = bootstrap_use_current_1level(system->env->root_cnode,
                                system->env->cspace_size_bits, system->env->free_slots.start,
                                system->env->free_slots.end, SEL4OSAPI_ROOT_TASK_UNTYPED_MEM_SIZE,
                                bootstrap_mem_pool);

    allocman_make_vka(&system->vka, system->allocator);

    int slot, size_bits_index;
    for (slot = system->env->untypeds.start, size_bits_index = 0;
            slot <= system->env->untypeds.end;
            slot++, size_bits_index++) {

        cspacepath_t path;
        vka_cspace_make_path(&system->vka, slot, &path);
        /* allocman doesn't require the paddr unless we need to ask for phys addresses,
         * which we don't. */
        uint32_t fake_paddr = 0;
        uint32_t size_bits = system->env->untyped_size_bits_list[size_bits_index];
        error = allocman_utspace_add_uts(system->allocator, 1, &path, &size_bits, &fake_paddr, ALLOCMAN_UT_KERNEL);
        assert(!error);
    }

    void *existing_frames[] = { system->env, seL4_GetIPCBuffer()};
    error = sel4utils_bootstrap_vspace(&system->vspace, &system->vspace_alloc_data, system->env->page_directory, &system->vka,
                                                NULL, NULL, existing_frames);
    assert(error == 0);

    system->vmem_reservation = vspace_reserve_range(&system->vspace, SEL4OSAPI_USER_PROCESS_UNTYPED_MEM_SIZE / 4,
                                               seL4_AllRights, 1, &system->vmem_addr);
    assert(system->vmem_reservation.res);
    bootstrap_configure_virtual_pool(system->allocator, system->vmem_addr, SEL4OSAPI_USER_PROCESS_UNTYPED_MEM_SIZE / 4,
                                     system->env->page_directory);

    error =sel4utils_reserve_range_no_alloc(&system->vspace,
            &muslc_brk_reservation_memory,
            SEL4OSAPI_USER_PROCESS_UNTYPED_MEM_SIZE - (SEL4OSAPI_USER_PROCESS_UNTYPED_MEM_SIZE / 4),
            seL4_AllRights, 1, &muslc_brk_reservation_start);
    assert(error == 0);
    muslc_this_vspace = &system->vspace;
    muslc_brk_reservation.res = &muslc_brk_reservation_memory;
    morecore_area = NULL;
    morecore_size = 0;
}

/*
 * Initialize process environment information for the root_task
 * (which is not a proper sel4osapi_process_t).
 */
static void
sel4osapi_system_initialize_root_task_env(sel4osapi_system_t *system)
{
    /* allocate a page to store the sel4osapi_process_env_t instance */
    syslog_trace("Allocating 1 page for with %d bits for env...", PAGE_BITS_4K);
    system->env = (sel4osapi_process_env_t*) vspace_new_pages(&system->vspace, seL4_AllRights, 1, PAGE_BITS_4K);
    assert(system->env != NULL);

    /* root_task has pid 0 */
    system->env->pid = 0;
    system->env->priority = seL4_MaxPrio - 1;

    system->env->page_directory = simple_get_pd(&system->simple);
    system->env->root_cnode = simple_get_cnode(&system->simple);
    system->env->tcb = simple_get_tcb(&system->simple);
    system->env->cspace_size_bits = simple_get_cnode_size_bits(&system->simple);
    system->env->fault_endpoint = seL4_CapNull;
    snprintf(system->env->name, SEL4OSAPI_USER_PROCESS_NAME_MAX_LEN, "%s", "root_task");
    system->env->idling_aep = system->idling_aep.capPtr;

    syslog_trace("Allocating simple pool for thread... - count=%d, size=%d", SEL4OSAPI_MAX_THREADS_PER_PROCESS, sizeof(sel4osapi_thread_t));
    system->env->threads = simple_pool_new(SEL4OSAPI_MAX_THREADS_PER_PROCESS, sizeof(sel4osapi_thread_t), NULL, NULL, NULL);
    assert(system->env->threads);

    syslog_trace("Root task environment successfully initialized");
}

/*
 * Initialize an instance of sel4osapi_thread_t that will be used
 * to represent a process's initial thread (in both root and user
 * processes).
 */
static void
sel4osapi_system_initialize_main_thread(sel4osapi_system_t *system)
{
    snprintf(system->main_thread.info.name,SEL4OSAPI_THREAD_NAME_MAX_LEN,"p-%02d-main", system->env->pid);
    system->main_thread.info.arg = NULL;
    system->main_thread.info.tls = NULL;
    system->main_thread.info.tid = 0;
    system->main_thread.info.priority = system->env->priority;
    /*system->main_thread.info.wait_aep = system->wait_aep;*/
    system->main_thread.info.active = 1;
    {
        int error = 0;
        vka_object_t aep_obj = {0};
        error = vka_alloc_notification(&system->vka, &aep_obj);
        assert(error == 0);
        system->main_thread.info.wait_aep = aep_obj.cptr;
    }

    system->main_thread.info.ipc =  (void*) seL4_GetUserData();
    seL4_SetUserData((seL4_Word)&system->main_thread);
}

/*
 * Initialize the "global" mutex instance (both root and user),
 * which can be used to provide global mutual exclusion within
 * between threads of the same process.
 */
static void
sel4osapi_system_initialize_global_mutex(sel4osapi_system_t *system)
{
    system->global_mutex = sel4osapi_mutex_create();
    assert(system->global_mutex != NULL);
}

/*
 * Initialize system resources from the root task.
 */
int
sel4osapi_system_initialize(void *bootstrap_mem_pool)
{
    int error = 0;
    sel4osapi_system_t *system;
    vka_object_t aep_obj = { 0 };
    int i;
    
    /* check that we have a bootstrap buffer*/
    assert(bootstrap_mem_pool != NULL);

    syslog_trace("Initializing system resources...");
    system = sel4osapi_system_get_instanceI();

    syslog_trace("Bootstrapping system root...");
    sel4osapi_system_bootstrap_root(system, bootstrap_mem_pool);

    syslog_trace("Initializing global mutex...");
    sel4osapi_system_initialize_global_mutex(system);

    syslog_trace("Allocating AEP for root task 1/2");

    /* allocate an asynchronous endpoint for the root task to idle on */
    error = vka_alloc_notification(&system->vka, &aep_obj);
    assert(error == 0);

    syslog_trace("Allocating AEP for root task 2/2");
    vka_cspace_make_path(&system->vka, aep_obj.cptr, &system->idling_aep);

    {
        /* reserve untyped memory for user processes by
         * creating untyped memory objects using the VKA.
         */
        vka_object_t root_task_uts[ROOT_TASK_NUM_UNTYPEDS];
        UNUSED unsigned int reserve_num = 0;

        /* Reserve some memory for the root_task (we'll free it after allocating untypeds for the process) */
        syslog_trace("Allocating untyped memory for root task");
        reserve_num = sel4osapi_util_allocate_untypeds(&system->vka, root_task_uts, SEL4OSAPI_ROOT_TASK_UNTYPED_MEM_SIZE, ROOT_TASK_NUM_UNTYPEDS);
        syslog_trace("\t-> %d bytes allocated", reserve_num);

        /* Now allocate everything else for the user processes */
        syslog_trace("Allocating untyped memory for user process");
        system->user_untypeds_num = sel4osapi_util_allocate_untypeds(&system->vka, system->user_untypeds,
                                            UINT_MAX, ARRAY_SIZE(system->user_untypeds));
        syslog_trace("\t-> %d bytes allocated", system->user_untypeds_num);

        /* Fill out the size_bits list */
        syslog_trace("Initializing size bits list...");
        for (i = 0; i < system->user_untypeds_num; i++) {
            system->user_untypeds_size_bits[i] = system->user_untypeds[i].size_bits;
        }
        /* Return reserve memory */
        syslog_trace("Release reserved memory...");
        for (i = 0; i < reserve_num; i++) {
            vka_free_object(&system->vka, &root_task_uts[i]);
        }
        assert(system->user_untypeds_num > 0);
        /* initialize untypeds allocation map */
        syslog_trace("Initializing allocation map...");
        for (i = 0; i < system->user_untypeds_num; ++i) {
            system->user_untypeds_allocation[i] = 0;
        }
    }

    syslog_trace("Creating simple pool: count=%d, size=%d...", SEL4OSAPI_USER_PROCESS_MAX, sizeof(sel4osapi_process_t));
    system->processes = simple_pool_new(SEL4OSAPI_USER_PROCESS_MAX, sizeof(sel4osapi_process_t), NULL, NULL, NULL);
    assert(system->processes != NULL);
    
    syslog_trace("Initializing root task envornment...");
    sel4osapi_system_initialize_root_task_env(system);

    syslog_trace("Initializing main thread...");
    sel4osapi_system_initialize_main_thread(system);

    syslog_trace("Initializing logger...");
    sel4osapi_log_initialize();

#ifdef CONFIG_LIB_OSAPI_SYSCLOCK
    syslog_trace("Initializing sysclock...");
    sel4osapi_sysclock_initialize(&system->sysclock);

    syslog_trace("Starting sysclock...");
    sel4osapi_sysclock_start(&system->sysclock);
#endif

    syslog_trace("Initializing IPC Server...");
    sel4osapi_ipcserver_initialize(&system->ipc);

#if defined(CONFIG_LIB_OSAPI_NET) || defined(CONFIG_LIB_OSAPI_SERIAL)
    syslog_trace("Initializing I/O...");
    sel4osapi_io_initialize(&system->io_ops);
#endif

#ifdef CONFIG_LIB_OSAPI_SERIAL
    syslog_trace("Initializing serial port...");
    sel4osapi_io_serial_initialize(&system->serial, system->env->priority);
#endif

    syslog_trace("sel4osapi_system_initialize completed successfully");
    return seL4_NoError;
}

#ifdef CONFIG_LIB_OSAPI_NET

int
sel4osapi_system_initialize_network(
        char *iface_name, sel4osapi_netiface_driver_t *iface_driver)
{
    sel4osapi_system_t *system = sel4osapi_system_get_instanceI();

    assert(iface_name);
    assert(iface_driver);

    sel4osapi_network_initialize(&system->net, &system->ipc, iface_name, iface_driver);

    sel4osapi_udp_initialize(&system->udp, system->env->priority);

    return seL4_NoError;
}

#endif

/*
 * Initialize system resources from a user process.
 */
int
sel4osapi_system_initialize_process(void *bootstrap_mem_pool, int argc, char **argv)
{
    sel4osapi_process_env_t *env;
    seL4_CPtr endpoint;
    seL4_Word badge;
    UNUSED seL4_MessageInfo_t info;
    sel4osapi_system_t *system = sel4osapi_system_get_instanceI();

    /* check that we have a bootstrap buffer and
     * the process's arguments */
    assert(bootstrap_mem_pool != NULL && argc == 3 && argv != NULL);

    syslog_trace("Initializing user process..."); 
    /* parse received args */
    endpoint = (seL4_CPtr) atoi(argv[2]);
    /* wait for a message from root_task
     * containing this process' environment */

    info = seL4_Recv(endpoint, &badge);

    /* check the label is correct */
    assert(seL4_MessageInfo_get_label(info) == seL4_Fault_NullFault);
    assert(seL4_MessageInfo_get_length(info) == 1);
    env = (sel4osapi_process_env_t*) seL4_GetMR(0);
    assert(env != 0);

    syslog_trace("Calling system_bootstrap_user");

    sel4osapi_system_bootstrap_user(system, bootstrap_mem_pool, env);

    syslog_trace("Calling system_initialize_global_mutex");
    sel4osapi_system_initialize_global_mutex(system);

    syslog_trace("Calling simple_pool_new");
    system->env->threads = simple_pool_new(SEL4OSAPI_MAX_THREADS_PER_PROCESS, sizeof(sel4osapi_thread_t), NULL, NULL, NULL);
    assert(system->env->threads);

    syslog_trace("Calling system_initialize_main_thread");
    sel4osapi_system_initialize_main_thread(system);

    syslog_trace("Calling log_initialize");
    sel4osapi_log_initialize();

    syslog_trace("Creating semaphores...");
    {
        assert(env->ipcclient.rx_buf);
        assert(env->ipcclient.tx_buf);
        assert(env->ipcclient.rx_buf_size > 0);
        assert(env->ipcclient.tx_buf_size > 0);
        env->ipcclient.rx_buf_avail = sel4osapi_semaphore_create(1);
        assert(env->ipcclient.rx_buf_avail);
        env->ipcclient.tx_buf_avail = sel4osapi_semaphore_create(1);
        assert(env->ipcclient.tx_buf_avail);
    }
#ifdef CONFIG_LIB_OSAPI_NET
    syslog_trace("Creating network mutex...");
    {
        env->udp_iface.mutex = sel4osapi_mutex_create();
        assert(env->udp_iface.mutex);
        env->udp_iface.sockets = simple_pool_new(SEL4OSAPI_UDP_MAX_SOCKETS, sizeof(sel4osapi_udp_socket_t), NULL, NULL, NULL);
        assert(env->udp_iface.sockets);
        assert(env->udp_iface.stack_op_ep);
    }
#endif
#ifdef CONFIG_LIB_OSAPI_SERIAL
    syslog_trace("Creating serial semaphore...");
    {
        assert(env->serial.buf);
        assert(env->serial.buf_size > 0);
        env->serial.buf_avail = sel4osapi_semaphore_create(1);
        assert(env->serial.buf_avail);
    }
#endif

    syslog_trace("Init completed successfully");
    return seL4_NoError;
}

/*
 * Block the calling thread on the containing process'
 * "idling AEP". This will effectively stop the thread
 * from executing further, but avoid its termination.
 */
void
sel4osapi_idle()
{
    sel4osapi_system_t *system = sel4osapi_system_get_instanceI();
    seL4_Word sender_badge = 0;

    syslog_info("Idle...");
    seL4_Wait(system->env->idling_aep, &sender_badge);
}

vka_t*
sel4osapi_system_get_vka()
{
    return &sel4osapi_gv_system.vka;
}

vspace_t*
sel4osapi_system_get_vspace()
{
    return &sel4osapi_gv_system.vspace;
}

simple_t*
sel4osapi_system_get_simple()
{
    return &sel4osapi_gv_system.simple;
}

#if defined(CONFIG_LIB_OSAPI_SERIAL) || defined(CONFIG_LIB_OSAPI_NET)
ps_io_ops_t*
sel4osapi_system_get_io_ops()
{
    return &sel4osapi_gv_system.io_ops;
}
#endif

#ifdef CONFIG_LIB_OSAPI_NET
sel4osapi_netstack_t*
sel4osapi_system_get_netstack()
{
    return &sel4osapi_gv_system.net;
}
#endif
