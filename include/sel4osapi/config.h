/*
 * FILE: config.h - sel4osapi configuration
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#ifndef SEL4OSAPI_CONFIG_H_
#define SEL4OSAPI_CONFIG_H_

/*
 * Minimum amount of memory required to bootstrap
 * the root memory allocator.
 *
 * Default: 40K
 */
#define SEL4OSAPI_BOOTSTRAP_MEM_POOL_SIZE           CONFIG_LIB_OSAPI_BOOTSTRAP_MEM_POOL_SIZE

/*
 * Minimum amount of virtual memory memory required
 * by the root memory allocator.
 *
 * Default: 64MB
 *
 */
#define SEL4OSAPI_SYSTEM_VMEM_SIZE                  CONFIG_LIB_OSAPI_SYSTEM_VMEM_SIZE

/*
 *
 */
#define SEL4OSAPI_VKA_VMEM_SIZE                     CONFIG_LIB_OSAPI_VKA_VMEM_SIZE

/*
 * Amount of untyped memory to reserve for the root_task
 *
 * Default: 64MB

 */
#define SEL4OSAPI_ROOT_TASK_UNTYPED_MEM_SIZE        CONFIG_LIB_OSAPI_ROOT_UNTYPED_MEM_SIZE

/*
 * Amount of untyped memory to reserve for a user process
 *
 * Default: 64MB
 *
 */
#define SEL4OSAPI_USER_PROCESS_UNTYPED_MEM_SIZE     CONFIG_LIB_OSAPI_USER_UNTYPED_MEM_SIZE

/*
 * Maximum number of user processes supported by the system.
 *
 */
#define SEL4OSAPI_USER_PROCESS_MAX                   CONFIG_LIB_OSAPI_MAX_USER_PROCESSES


#define SEL4OSAPI_PROCESS_RX_BUF_SIZE                CONFIG_LIB_OSAPI_IPC_RX_BUF_SIZE

#define SEL4OSAPI_PROCESS_TX_BUF_SIZE                CONFIG_LIB_OSAPI_IPC_TX_BUF_SIZE

#define SEL4OSAPI_PROCESS_RX_BUF_PAGES               ROUND_UP_UNSAFE(SEL4OSAPI_PROCESS_RX_BUF_SIZE, PAGE_SIZE_4K) / PAGE_SIZE_4K

#define SEL4OSAPI_PROCESS_TX_BUF_PAGES                ROUND_UP_UNSAFE(SEL4OSAPI_PROCESS_TX_BUF_SIZE, PAGE_SIZE_4K) / PAGE_SIZE_4K


/*
 * Priority at which user processes are created by default.
 */
#define SEL4OSAPI_USER_PROCESS_PRIORITY_DEFAULT       CONFIG_LIB_OSAPI_USER_PRIORITY

/*
 * Maximum number of threads that can be created within
 * the same process.
 */
#define SEL4OSAPI_MAX_THREADS_PER_PROCESS             CONFIG_LIB_OSAPI_MAX_THREADS_PER_PROCESS

/*
 * Maximum length of a thread's name.
 */
#define SEL4OSAPI_THREAD_NAME_MAX_LEN                   CONFIG_LIB_OSAPI_THREAD_MAX_NAME

/*
 * Period, in milliseconds, at which the sysclock's
 * time should be updated.
 */
#define SEL4OSAPI_SYSCLOCK_PERIOD_MS                    CONFIG_LIB_OSAPI_SYSCLOCK_PERIOD

#endif /* SEL4OSAPI_CONFIG_H_ */
