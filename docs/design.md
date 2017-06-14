# libsel4osapi Design

This document contains design information about `libsel4osapi`.

```
 Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.

 This software may be distributed and modified according to the terms of
 the BSD 2-Clause license. Note that NO WARRANTY is provided.
 See "LICENSE_BSD2.txt" for details.

 Author  ....: Andrea Sorbini
 Version ....: 1.0
 Date .......: 26 oct 2016
```

## Table of Contents

* [Architecture](#architecture)
* [System interface](#system-interface)
  + [Root Task initialization](#root-task-initialization)
  + [User process initialization](#user-process-initialization)
  + [Network initialization](#network-initialization)
* [SysClock service](#sysclock-service)
  + [SysClock service initialization](#sysclock-service-initialization)
  + [Accessing current time](#accessing-current-time)
  + [Scheduling timeouts](#scheduling-timeouts)
  + [Canceling timeouts](#canceling-timeouts)
* [IPC support](#ipc-support)
  + [IPC server initialization](#ipc-server-initialization)
  + [IPC client initialization](#ipc-client-initialization)
  + [client usage](#client-usage)
* [Serial support](#serial-support)
  + [Serial server initialization](#serial-server-initialization)
  + [Serial client initialization](#serial-client-initialization)
  + [Serial Write](#serial-write)
  + [Serial Read](#serial-read)
* [UDP/IP Network support](#udpip-network-support)
  + [Device Drivers](#device-drivers)
  + [Network stack initialization](#network-stack-initialization)
  + [Network device initialization](#network-device-initialization)
  + [IP address configuration](#ip-address-configuration)
  + [Socket API](#socket-api)
    - [Socket creation](#socket-creation)
    - [Socket binding](#socket-binding)
    - [Sending UDP packets](#sending-udp-packets)
    - [Receiving UDP packets](#receiving-udp-packets)
* [User Processes](#user-processes)
  + [Process creation](#process-creation)
    - [Capability transfer](#capability-transfer)
  + [Process lifecycle](#process-lifecycle)
* [User Threads](#user-threads)
  + [Thread creation](#thread-creation)
  + [Thread lifecycle](#thread-lifecycle)
  + [Thread sleep](#thread-sleep)
  + [Default Thread](#default-thread)
* [Synchronization Primitives](#synchronization-primitives)
  + [Mutex](#mutex)
  + [Semaphore](#semaphore)
* [Utility features](#utility-features)
  + [simple_list_t](#simple_list_t)
  + [simple_pool_t](#simple_pool_t)
  + [Memory allocation](#memory-allocation)
* [Revision History](#revision-history)





## Architecture

libsel4osapi provides high-level system services for seL4 applications including:
  - system bootstrap and configuration
  - Unix-like processes and threads
  - thread synchronization primitives (mutex, semaphore)
  - IP network interface support with socket-like API for UDP communication
  - read/write API for serial interfaces

The library relies on other user libraries currently available for the seL4 kernel:
  - libmuslc
    - Standard C library
  - libsel4muslcsys
    - Implementation of libmuslc for seL4.
  - libsel4allocman
    - User memory allocation manager for seL4.
  - libsel4vka
    - Allocation and management of seL4 kernel objects.
  - libsel4vspace
    - Implementation of a Virtual Address Space for seL4 applications.
  - libsel4utils
    - Utility library to perform high-level operations in seL4 applications
      (e.g. configure Virtual Address Space, spawn process from ELF...)
  - libsel4simple
    - Simplified interface to seL4 kernel resources and services provided by
      other user libraries.
  - libplatsupport
    - Support for generic platform services for seL4 (e.g. timers).
  - libsel4platsupport
    - Platform-specific implementation of platform services for seL4
  - liblwip
    - Lightweight implementation of UDP/IP network stack.
  - libethdrivers
    - Implementation of NIC drivers for sel4.


The library supports UNIX-like **process** and **thread** abstractions.

Each process has its own address space and capability space.
Processes can either be <b>user processes</b> or the special process <b>root
task</b>.

Each process has one or more **threads**. Each thread has a unique id and a
private, thread-specific context. A thread shares address space and capability
space with its parent process and every other thread it contains.

The root task takes care of bootstrapping the system, initializing system
services and spawning user processes.

System bootstrap is performed using the seL4 user libraries.

System services are implemented using a client/server model. Multiple servers
are deployed within the root task. User processes access these services using
seL4 endpoints and memory frames shared with the root task.

An overview of a system based on libsel4osapi is presented Figure 1.

```

 +-----------------------------------+ +-----------------------------------+
 | ROOT TASK                         | | USER PROCESS                      |
 | +-------------------+             | | +-------------------+             |
 | | System            |             | | | System            |             |
 | | +---------------+ | +---------+ | | | +---------------+ | +---------+ |
 | | | Process Env.  | | | THREAD  | | | | | Process Env.  | | | THREAD  | |
 | | +---------------+ | | +-----+ | | | | +---------------+ | | +-----+ | |
 | | +---------------+ | | | Ctx | | | | | +---------------+ | | | Ctx | | |
 | | | Clock Server  | | | +-----+ | | | | | Clock Client  | | | +-----+ | |
 | | +---------------+ | |         | | | | +---------------+ | |         | |
 | | +---------------+ | |         | | | | +---------------+ | |         | |
 | | | Net Server    | | +---------+ | | | | Net Client    | | +---------+ |
 | | +---------------+ |             | | | +---------------+ |             |
 | | +---------------+ |             | | | +---------------+ |             |
 | | | Serial Server | |             | | | | Serial Client | |             |
 | | +---------------+ |             | | | +---------------+ |             |
 | +-------------------+             | | +-------------------+             |
 +-----------------------------------+ +-----------------------------------+
 +-------------+ +-------+ +-----------------------------------------------+
 |             | |       | |            libsel4osapi                       |
 |             | |       | +---------------------------------------+       |
 |             | |       +---------------------------------------+ |       |
 |             | |              sel4 user libraries              | |       |
 |             | +-----------------------------------------------+ +-------+
 |             +-----------------------------------------------------------+
 |                         seL4 kernel                                     |
 +-------------------------------------------------------------------------+

 FIGURE 1 - libsel4osapi system architecture

```

Every user process has local clients to servers in the root task. Use of these
clients is transparent and hidden by the API to provide a consistent interface
between root task and user processes.

In order to use the library, every process must first initialize it. The
initialization takes of creating any required infrastructure (ie. servers in
the root task, service clients in other processes) to provision the library
services to the process.

## System interface

Interaction with the seL4 kernel and the seL4 user libraries is governed by the
**System** interface. A different **System** instance is available as a singleton
inside any process (both root task and user processes).

**System** offers the following operations:
  - **initialize**
    - The root task should call this operation before doing anything else
      in order to bootstrap the system and initialize libsel4osapi (ie. spawn
      the required services).
  - **initialize_process**
    - Any user process should call this operation before doing anything else
      in order to initialize libsel4osapi (ie. configure service clients). The
      operation relies on arguments passed to the seL4 Thread in order properly
      configure service clients (e.g. retrieve the required capabilities passed
      in by the root task).
  - **initialize_network**
    - The root task can use this operation to initialize libsel4osapi's network
      stack by supplying an appropriate device driver.
  - **get_vka**
    - Retrieve the **vka** instance required to use **libsel4vka** and other
      sel4 user libraries.
  - **get_vspace**
    - Retrieve the **vspace** instance required to use **libsel4vspace** and other
      sel4 user libraries.
  - **get_simple**
    - Retrieve the **simple_t** instance required to use **libsel4simple** and other
      sel4 user libraries.
  - **get_io_ops**
    - Retrieve the **io_ops_t** instance required to use **libsel4platform** and
      other sel4 user libraries.
  - **get_netstack**
    - Retrieve the **netstack_t** instance required to use **libsel4osapi**.s
      networking features.

### Root Task initialization

the **sel4osapi_system_initialize** operation takes care of bootstrapping the base seL4 system and
must be the first operation invoked by the root task after being passed control
from the kernel.

Its implementation is mostly derived from existing examples found online (e.g.
sel4-tutorials).

Initialization of an seL4 system can occur in many different ways, given the
great freedom left by the kernel to developers and system designer.

For this reason, libsel4osapi must take some decisions which end up limiting
the developers' freedom and might make the library not suited for all use cases.

The initialize operation can be split in the following steps:
  1. Initialize system using sel4 user libraries
     (wrapped by **sel4osapi_system_bootstrap_root**.
    - System bootstrap requires a static buffer to be passed in by the root task.
    - Configure a static buffer provided by the library to be initially used
      by **libsel4muslcsys**.s implementation of mmap()/brk().
      - Size of buffer: SEL4OSAPI_BOOTSTRAP_MEM_POOL_SIZE
    - Create a **simple_t** from the boot-info provided by the kernel
      using **simple_default_init_bootinfo** from **libsel4simple**.default
      - Use the static buffer passed to **initialize** to bootstrap the
        **allocman_t**.
    - Create an **allocman_t** (system object allocator) using
      **bootstrap_use_current_simple** from **libsel4allocman**
    - Create a **vka_t** (from **libsel4vka**. from the **allocman_t** using
      **allocman_make_vka** from **libsel4allocman**
    - Create a **vspace_t** (from **libsel4vspace**. using
      **sel4utils_bootstrap_vspace_with_bootinfo_leaky** from **libsel4utils**
      - A vspace_t is an interface to manage a Virtual Page Table which keeps
        track of which frame, if any, a virtual memory address is currently
        mapped to. The table is stored at the top of the virtual memory, right
        before the kernel's reserved address range. The address space available
        to the system is 0x00001000 to 0xdf7fefff, but the vspace_t will start
        mapping pages at 0x10000000 if not passed a specific vmem address when
        requested to map one or more pages. Pages are 4K each.
    - Reserve a chunk of virtual memory managed by the **vspace_t** for the
      **allocman_t** by calling **vspace_reserve_range** from **libsel4vspace**
      - Size of memory: SEL4OSAPI_VKA_VMEM_SIZE
      - The vspace will not allocate pages but simply mark the address range as
        RESERVED.
    - Instruct the **allocman_t** to use the reserved range of memory using
      **bootstrap_configure_virtual_pool** from **libsel4allocman**
      - The **allocman_t** doesn't rely on the **vspace_t** to manage virtual
        memory. Instead, **allocman_t** will create a new page table kernel
        object and manually map pages into it at the addresses of the specified
        range.
    - Reserve and assign a chunk of virtual memory to libsel4muslcsys's
      implementation of mmap()/brk() using **sel4utils_reserve_range_no_alloc**
      from **libsel4utils**.
      - Size of memory: SEL4OSAPI_SYSTEM_VMEM_SIZE
      - Virtual memory is manually managed by **libsel4muslcsys** using the
        **vspace_t** and the created reservation.
      - Access to static buffer is reset "revoked" for **libsel4muslcsys**
  2. Create a global mutex
    - The mutex is a **sel4osapi_mutex_t**
    - This mutex is currently not used anywhere in the library and
      it should be DEPRECATED.
  3. Create an "idle" AsyncEndpoint for the root task
    - This AEP is used by **sel4osapi_idle** to "suspend indefinitely" a thread
       by invoking **sel4_Wait** on the AEP.
    - Since there is currently no operation to "resume" the thread (by signaling
      the AEP), **sel4osapi_idle** should only be called by a thread that
      completed execution but shouldn't terminate.
  4. Reserve untyped memory for user processes
    - Untyped memory for the root task is reserved using
      **sel4osapi_util_allocate_untypeds**.
      - Size of memory: SEL4OSAPI_ROOT_TASK_UNTYPED_MEM_SIZE.
      - Max number of untyped memory blocks: ROOT_TASK_NUM_UNTYPEDS
    - Untyped memory for user processes is also reserved using
      **sel4osapi_util_allocate_untypeds**.
      - Size of memory: UINT_MAX
      - Max number of untyped memory blocks: CONFIG_MAX_NUM_BOOTINFO_UNTYPED_CAPS
  5. Pre-allocate a **simple_pool_t** of **sel4osapi_process_t**
    - Size of pool: SEL4OSAPI_USER_PROCESS_MAX
  6. Initialize the root task's environment
    - Environment struct (**sel4osapi_process_env_t**. stored in a new page
      (allocated with **vspace_new_pages**.
    - PID: 0
    - Priority: seL4_MaxPrio - 1
    - Page Directory: from **simple_get_pd**
    - Root CNode: from **simple_get_cnode**
    - TCB: from **simple_get_tcb**
    - CSpace size bits: from **simple_get_cnode_size_bits**
    - Fault Endpoint: seL4_CapNull
    - Name: "root_task"
    - Idling AEP: the AEP previously allocated
    - Threads: a **simple_pool_t** of **sel4osapi_thread_t** (size:
      SEL4OSAPI_MAX_THREADS_PER_PROCESS)
  7. Initialize the root task's main thread's environment using
     **sel4osapi_system_initialize_main_thread**
    - Manual initialization of a **sel4osapi_thread_t** struct to support
      thread operations in the root-task's default thread.
    - Name: p-$PID-main
    - Arguments: NULL
    - Local Storage: NULL
    - ID: 0
    - Priority: same as process' priority.
    - Active: true
    - Wait AEP: new AEP allocated using **vka_alloc_async_endpoint**
    - IPC Buffer: from **seL4_GetUserData**
    - Address of **sel4osapi_thread_t** struct stored using **seL4_SetUserData**.
  8. Initialize the logging service using **sel4osapi_log_initialize**
    - Create a **sel4osapi_mutex_t** to synchronize calls to log coming from
      different threads within the root task.
  9. Initialize the system clock using **sel4osapi_sysclock_initialize** and start
     with **sel4osapi_sysclock_start**.
    - Initialization of **sel4osapi_sysclock_t** is described in section
      [SysClock service](#sysclock-service).
  10. Initialize the IPC buffers used to exchange data between user processes
     and root task using **sel4osapi_ipcserver_initialize**.
     - Initialization of **sel4osapi_ipcserver_t** is described in section
       [IPC support](#ipc-support).
  11. Initialize the serial IO server using **sel4osapi_io_initialize**.
     - Initialization of **sel4osapi_serialserver_t** is described in section
       [Serial support](#serial-support).

### User process initialization

The **sel4osapi_system_initialize_process** operations takes care of initializing **libsel4osapi**
inside a user process' environment. A user process must invoke this operation
before doing anything else, and passing the arguments received from the parent
process.

The operation can be broken down in the following steps:
  1. Check that a static buffer was passed in by the process, and that the
     expected number of arguments is available (3).
  2. Parse arguments received from parent process
     - argv[2] contains an Endpoint copied into the process' CSpace which is
       used to receive the address of the memory page containing the
       **sel4osapi_process_env_t** for the user process built by the parent.
  3. Bootstrap sel4 user libraries and data structures using
     **sel4osapi_system_bootstrap_user**.
     - Assign a static buffer to **libsel4muslcsys**.s implementation of mmap/brk
     - Initialize an **allocman_t** using **bootstrap_use_current_1level**
       - Information passed from process' **sel4osapi_process_env_t**
       - **allocman_t** bootstrapped using static buffer passed by process.
     - Create a **vka_t** from the **allocman_t** using **allocman_make_vka**
     - Pass blocks of untyped memory assigned to the process to **allocmant**.
       - Iterate over slots and call **allocman_utspace_add_uts**
     - Create a **vspace_t** using **sel4utils_bootstrap_vspace**
       - Initial frames: process' **sel4osapi_process_env_t** and address from
         seL4_GetIPCBuffer
     - Reserve virtal memory for **allocman_t** using **vspace_reserve_range**
       - Size of memory: SEL4OSAPI_USER_PROCESS_UNTYPED_MEM_SIZE
     - Assign reserved memory to **allocmant_** using
       **bootstrap_configure_virtual_pool**
     - Reserve virtual memory in the **vspace_t** for mmap/brk using
       **sel4utils_reserve_range_no_alloc**
       - Size of memory: SEL4OSAPI_USER_PROCESS_UNTYPED_MEM_SIZE - (SEL4OSAPI_USER_PROCESS_UNTYPED_MEM_SIZE / 4)
     - Assign reserved virtual memory to **libsel4muslcsys**.s implementation of
       mmap/brk and "revoke" access to static buffer.
  4. Create a global mutex
    - The mutex is a **sel4osapi_mutex_t**
    - This mutex is currently not used anywhere in the library and
      it should be DEPRECATED.
  5. Create a **simple_pool_t** of **sel4osapi_threads_t**
     - Size of pool: SEL4OSAPI_MAX_THREADS_PER_PROCESS
  6. Initialize the process' main thread's environment
     - Manual initialization of a **sel4osapi_thread_t** struct to support
      thread operations in the process' default thread (see
      [Root Task initialization](#root-task-initialization)).
  7. Initialize the logging service using **sel4osapi_log_initialize**
    - Create a **sel4osapi_mutex_t** to synchronize calls to log coming from
      different threads within the process.
  8. Initialize an IPCClient for the process
     - See [IPC support](#ipc-support).
  9. Initialize a Network client for the process
     - See [UDP/IP Network support](#udpip-network-support).
  10. Initialize a Serial client for the process
     - See [Serial support](#serial-support).

### Network initialization

The **sel4osapi_system_initialize_network** operation must be called by the root task to initialize
the **sel4osapi_netstack_t** used by the system.

The operation receives a **sel4osapi_netiface_driver_t** and an interface anme,
which is used to invoke **sel4osapi_network_initialize** to initialize the NIC.

**sel4osapi_udp_initialize** is used to initialize a UDP stack on the NIC (
provided by LwIP).

More information about networking can be found in
[UDP/IP Network support](#udpip-network-support).

## SysClock service

The SysClock service provides the following functionalities:
  - Get current system clock
    - An monotonically incrementing counter with period SEL4OSAPI_SYSCLOCK_PERIOD_MS
      stored as uint32_t
  - Schedule a periodic/one-shot timeout
    - Receive a signal on an AEP every timeout_ms milliseconds (or just once,
      after timeout_ms).

The service is implemented by two threads in the root task:
  - sysclock::timer
    - Periodically increment system clock by handling interrupts from
      hardware clock.
  - sysclock::server
    - Server thread handling requests for scheduling/cancelling timeouts from
      other threads in the same and/or other processes.

### SysClock service initialization

Initialization of the SysClock service can be broken down in the following steps:
  1. Allocate an Endpoint for the server thread to receive external requests.
  2. Allocate an AsyncEndpoint for the timer thread to receive hardware interrupts.
  3. Retrieve the hardware timer using **sel4platsupport_get_default_timer**
  4. Allocate a **simple_pool_t** of **timeout_entry** (size: SEL4OSAPI_SYSCLOCK_MAX_ENTRIES)
  5. Allocate a **sel4osapi_mutex_t** to protect concurrent access to the
   timeouts schedule.
  6. Create a **sel4osapi_thread_t** for thread "sysclock::timer"
     (body: sel4osapi_sysclock_timer_thread).
  7. Create a **sel4osapi_thread_t** for thread "sysclock::server"
     (body: sel4osapi_sysclock_server_thread).
  8. Mint the server EP into the root task's **sel4osapi_process_env_t**

### Accessing current time

The current time is retrieved by invoking the SysClock server's EP with
**seL4_CallWithMRs**. using MR[0] to pass the operation type SYSCLOCK_OP_GET_TIME
(from **sel4osapi_sysclock_opcode**..

The current time is returned via MR[0].

### Scheduling timeouts

In order to schedule a new timeout, a client thread must **seL4_Call** the
SysClock server's EP passing the following values:
  - MR[0]: opcode SYSCLOCK_OP_SET_TIMEOUT
  - MR[1]: flag indicating whether the timeout is periodic or one-shot
  - MR[2]: timeout period in milliseconds

The server returns:
  - An error flag on MR[0]
  - a unique timeout id on MR[1]

### Canceling timeouts

In order to cancel an existing timeout, a client thread must **seL4_Call** the
SysClock server's EP passing the following values:
  - MR[0]: opcode SYSCLOCK_OP_CANCEL_TIMEOUT
  - MR[1]: timeout id

The server returns an error flag on MR[0], signaling whether the timer was
successfully cancelled or not.

## IPC support

libsel4osapi provides some limited support for passing data between the root
task and user processes.

Exchange of data is carried out using two memory buffers (tx and rx) that
the root task maps into every user process' virtual memory space.

### IPC server initialization

A **sel4osapi_ipcserver** is initialized within the root task.

Initialization can be broken down in the following steps:
  1. Create a **sel4osapi_mutex_t** to protect access to the server
  2. Initialize a **simple_pool_t** of **sel4osapi_ipcclient_t**
    - Pool size: SEL4OSAPI_USER_PROCESS_MAX

### IPC client initialization

When a new user process is being created within by the root task (as a result
of calling **sel4osapi_process_create**., a new **sel4osapi_ipcclient_t** is
allocated for the process.

Initialization of a client within the root task's environment (occurring within
**sel4osapi_process_create**. can be broken down in the following steps:
  1. Create a **sel4osapi_semaphore_t** to synchronize access to the Rx buffer.
  2. Create a **sel4osapi_semaphore_t** to synchronize access to the Tx buffer.
  3. Allocate the Rx buffer of size SEL4OSAPI_PROCESS_RX_BUF_SIZE
    - Map SEL4OSAPI_PROCESS_RX_BUF_PAGES pages in the root task's **vspace_t**
      using **vspace_new_pages**
  4. Allocate the Tx buffer of size SEL4OSAPI_PROCESS_TX_BUF_SIZE
    - Map SEL4OSAPI_PROCESS_TX_BUF_PAGES pages in the root task's **vspace_t**
      using **vspace_new_pages**

Initialization of a client within the user process' environment (occurring within
**sel4osapi_system_initializa_process**. can be broken down in the following steps:
  1. Create a **sel4osapi_semaphore_t** to synchronize access to the Rx buffer.
  2. Create a **sel4osapi_semaphore_t** to synchronize access to the Tx buffer.
  3. Verify that the (local) address of the Rx buffer was passed in by the
     root task in the process' **sel4osapi_process_env_t**.
  4. Verify that the (local) address of the Tx buffer was passed in by the
     root task in the process' **sel4osapi_process_env_t**.

### client usage

The Rx buffer of a client is meant to transmit data from the root
task to the user process, while the Tx buffer is used to pass data in the
opposite direction.

A client must acquire access to a buffer by taking the associated semaphore.
Once data has been written/read, the buffer must be released by signaling the
same semaphore.

The API cannot currently support use of these buffers as a generic communication
channel between user processes and root task, since it  assumes
that clients in the root task will use some external "channel" to synchronize
with clients in user processes (ie. make sure there is data to read on either
side and that the appropriate receiver/sender gets it on the other).

The Rx and Tx buffers are not meant for users of libsel4osapi but they are
rather used internally to implement network communication (see
[UDP/IP Network support](#udpip-network-support) for more information on how
they are used).

## Serial support

libsel4osapi provides simplified access from any user process to hardware serial
ports UART1 and UART2.

A **sel4osapi_serialserver_t** is deployed within the root task, while every
user process is assigned a **sel4osapi_serialclient_t**.

A server thread, named "serial", is deployed within the root task and receives
requests to read/write on serial ports via a shared EP.

### Serial server initialization

The serial server is initialized by **sel4osapi_io_initialized**. called by
**sel4osapi_system_initialize**.

Initialization can be broken down in the following steps:
  1. Create a new **ps_io_ops_t** using **sel4platsupport_new_io_ops**.
  2. Create a new DMA manager (**ps_dma_man_t**. using **sel4utils_new_page_dma_alloc**
  3. Configure seL4 serial support using **platsupport_serial_setup_simple**.
  4. Initialize serial ports using **ps_cdev_init**
    - Currently libsel4osapi only supports the SabreLite IMX6 hardware platform
      and only the UART2 device is initialized by the library, using hardcoded
      value IMX6_UART2.
    - UART1 is not initialized to be used via libsel4osapi since it is already
      used as debug console.
  5. Initialize a **simple_pool_t** of **sel4osapi_serialclient_t**
    - Pool size: SEL4OSAPI_USER_PROCESS_MAX
  6. Create a **sel4osapi_mutex_t** to protect access to the client pool.
  7. Allocate an Endpoint for the serial server.
  8. Create a **sel4osapi_thread_t** for the serial server (running
     **sel4osapi_serial_server_thread**.
  9. Start the serial server's thread.

### Serial client initialization

As part of a user process' initialization (within **sel4osapi_process_create**.,
a new **sel4osapi_serialclient_t** is created for the process using
**sel4osapi_io_serial_create_client**.

Initialization of a client inside the root task's environment can be
broken down in the following steps:
  1. Allocate a new **sel4osapi_serialclient_t** from the server's
     **simple_pool_t**.
  2. Create a **sel4osapi_semaphore_t** to synchronize access to the client's
     memory buffer.
  3. Allocate a memory buffer of size SEL4OSAPI_SERIAL_BUF_SIZE
    - Map SEL4OSAPI_SERIAL_BUF_PAGES in the **vspace_t** using **vspace_new_pages**.

Initialization of a client inside a user process' environment can be
broken down in the following steps (these occur within
**sel4osapi_system_initialize_process**.:
  1. Create a **sel4osapi_semaphore_t** to synchronize access to the client's
     memory buffer.
  2. Verify that the (local) address of the memory buffer was passed in by the
     root task in the process' **sel4osapi_process_env_t**.

### Serial Write

A user process can write data to a serial port by using
**sel4osapi_io_serial_write**.

This operation takes a memory buffer (expected to be of size equal or less than
the size of the process' **sel4osapi_serialclient_t**.s memory buffer),
a serial port id (represented by **sel4osapi_serialdevice_t**., and performs
the following operations:
  1. Gain access to the **sel4osapi_serialclient_t**.s memory buffer by taking
     its semaphore.
  2. Copy user's memory buffer into the **sel4osapi_serialclient_t**.s memory
     buffer.
  3. Signal the serial server by **seL4_Call** with the following arguments:
    - MR[0]: **sel4osapi_serialclient_t**.s id
    - MR[1]: opcode SERIAL_OP_WRITE
    - MR[2]: device id
    - MR[3]: size of user buffer
  4. Retrieve error flag from MR[0] in server's reply
  5. Release the **sel4osapi_serialclient_t**.s memory buffer by signaling its
     semaphore.

On the server's side, the server thread performs the following steps upon
detecting opcode SERIAL_OP_WRITE on a seL4_Call to its EP:
  1. Retrieve the (local) **sel4osapi_serialclient_t**
  2. Write data from the **sel4osapi_serialclient_t**.s memory buffer to the
     selected serial device using **ps_cdev_write**
  3. Reply to caller passing error flag on MR[0]

### Serial Read

A user process can read data from a serial port by using
**sel4osapi_io_serial_read**.

This operation takes a memory buffer, a read size (expected to be equal or
less than the size of the process' **sel4osapi_serialclient_t**.s memory buffer),
a serial port id (represented by **sel4osapi_serialdevice_t**., and performs
the following operations:
  1. Gain access to the **sel4osapi_serialclient_t**.s memory buffer by taking
     its semaphore.
  2. Signal the serial server by **seL4_Call** with the following arguments:
    - MR[0]: **sel4osapi_serialclient_t**.s id
    - MR[1]: opcode SERIAL_OP_READ
    - MR[2]: device id
    - MR[3]: size of bytes to read from serial port
  3. Retrieve error flag from MR[0] in server's reply
  4. Copy the **sel4osapi_serialclient_t**.s memory buffer into the user's
     memory buffer.
  5. Release the **sel4osapi_serialclient_t**.s memory buffer by signaling its
     semaphore.

On the server's side, the server thread performs the following steps upon
detecting opcode SERIAL_OP_READ on a seL4_Call to its EP:
  1. Retrieve the (local) **sel4osapi_serialclient_t**
  2. Read data from the selected serial device to the
     **sel4osapi_serialclient_t**.s memory buffer using **ps_cdev_read**.
  3. Reply to caller passing error flag on MR[0]

## UDP/IP Network support

libsel4osapi implements support for UDP/IP communication over a physical network
interface card (NIC).

The network sub-system requires at least one NIC in order to be initialized
with **sel4osapi_network_initialize**. The NIC is automatically
configured with an IP address specified at build time.

Additional NICs can be configured after initialization by using
**sel4osapi_network_create_interface**. while additional IP addresses ("virtual
interfaces") can configured on a NIC using **sel4osapi_netiface_add_vface**.

### Device Drivers

Network device drivers are abstracted by interface
**sel4osapi_netiface_driver_t**.

Every driver must provide the following:
  - seL4 IRQ number for the NIC
  - LwIP device initialization function
  - seL4 IRQ handler

A root task using seL4's **libethdrivers** could implement a network driver
for the SabreLite IMX6 board:

```

#include "autoconf.h"

#include <sel4osapi/osapi.h>

#include <ethdrivers/imx6.h>
#include <ethdrivers/lwip.h>

static int
native_ethdriver_init(
    struct eth_driver *eth_driver,
    ps_io_ops_t io_ops,
    void *config)
{
    ps_io_ops_t *sys_io_ops = (ps_io_ops_t*) config;
    int error;
    error = ethif_imx6_init(eth_driver, *sys_io_ops, NULL);
    return error;
}

static void
handle_irq(void *state, int irq_num)
{
    ethif_lwip_handle_irq(state, irq_num);
}

UNUSED
static
char bootstrap_allocator_mem_pool[SEL4OSAPI_BOOTSTRAP_MEM_POOL_SIZE];

int
main(int argc, char **argv)
{
    int result = 0;
    sel4osapi_netiface_driver_t iface_driver;
    lwip_iface_t lwip_driver;
    ps_io_ops_t *io_ops;
    char *iface_name = "eth0";

    sel4osapi_system_initialize(&bootstrap_allocator_mem_pool);

    io_ops = sel4osapi_system_get_io_ops();

    iface_driver.state = ethif_new_lwip_driver_no_malloc(
                                                    *io_ops,
                                                    NULL,
                                                    native_ethdriver_init,
                                                    io_ops,
                                                    &lwip_driver);
    assert(iface_driver.state);

    iface_driver.init_fn = ethif_get_ethif_init(&lwip_driver);
    assert(iface_driver.init_fn);

    iface_driver.handle_irq_fn = handle_irq;

    iface_driver.irq_num = IMX6_INTERRUPT_ENET;

    sel4osapi_system_initialize_network(iface_name, &iface_driver);

    /* ... */

    return result;
}

```

Every device is assigned an IRQ thread which waits for notifications on the
device's IRQ AEP and invokes the driver's IRQ handler.

Since the network implementation relies on LwIP, this handler should trigger
handling of the IRQ interrupt in LwIP using **ethif_lwip_handle_irq**.

The IRQ thread reset the device's IRQ with **seL4_IRQHandler_Ack**.

### Network stack initialization

The network stack, represented by **sel4osapi_netstack_t**. must be explicitly
initialized by the root task using **sel4osapi_system_initialize_network**.

This operation initializes the network stack with
**sel4osapi_network_initialize**. and then initializes the UDP socket support
with **sel4osapi_udp_initialize**.

**sel4osapi_network_initialize** requires the system's **sel4osapi_ipcserver_t**.
a device name, a device driver, and performs the following steps:
  1. Create a **sel4osapi_mutex_t** to protect access to the stack
  2. Initializes a **simple_pool_t** of **sel4osapi_netiface_t**
    - Pool size: SEL4OSAPI_NET_PHYS_IFACES
  3. Initialize a NIC with the specified name and driver by calling
     **sel4osapi_network_create_interface**.
  4. Configure an IP address on the NIC using **sel4osapi_netiface_add_vface**
    - Address: SEL4OSAPI_IP_ADDR_DEFAULT
    - Netmask: SEL4OSAPI_IP_MASK_DEFAULT
    - Gateway: SEL4OSAPI_IP_GW_DEFAULT

### Network device initialization

A physical network device is added to the network stack by means of
**sel4osapi_network_create_interface**.

This operation performs the following steps:
  1. Allocate a **sel4osapi_netiface_t** from the stack's **simple_pool_t**
  2. Create a **sel4osapi_mutex_t** to protect access to the interface
  3. Retrieve the IRQ control path for the device using **simple_get_IRQ_control**
  4. Allocate an AsyncEndpoint to handle IRQs from the device
  5. Set the AEP at the device's IRQ control path using
     **seL4_IRQHandler_SetEndpoint**.
  6. Initialize LwIP by using **lwip_init**.
  7. Initialize a **simple_pool_t** of **sel4osapi_netvface_t**
    - Pool size: SEL4OSAPI_NET_VFACES_MAX
  8. Create a **sel4osapi_thread_t** to handle the device's IRQs ("<IFACE>::irq").
  9. Start the device's IRQ thread.

### IP address configuration

IP addresses are configured on NICs by means of "virtual interfaces". Each
virtual interface represents an IP address/network mask/gateway configuration
and is configured on an existing NIC using **sel4osapi_netiface_add_vface**.

This operation adds the IP configuration to LwIP using **netif_add**.

Optionally, a virtual interface may be set as LwIP's default, using
**netif_set_default**.

### Socket API

libsel4osapi allows clients to use a socket-like API to write/read UDP
datagrams from a network interface.

The API is exposed to users by the operations:
  - **sel4osapi_udp_create_socket**
    - Creates a new UDP socket on the specified IP address
  - **sel4osapi_udp_bind**
    - Binds a socket to a specific UDP port
  - **sel4osapi_udp_send**
    - Send a UDP message to a specific IP:port
  - **sel4osapi_udp_recv**
    - Wait for a UDP message to be received
  - **sel4osapi_udp_send_sd**
    - Same as **sel4osapi_udp_send** but uses an integer id to identify
      the socket.
  - **sel4osapi_udp_recv_sd**
    - Same as **sel4osapi_udp_recv** but uses an integer id to identify
      the socket.

Internally, each call interacts via seL4 Endpoint with server threads deployed
within the root task.

Initialization of the UDP socket support (**sel4osapi_udpstack_t**. within
**sel4osapi_udp_initialize** includes the following steps:
  1. Create a **sel4osapi_mutex_t** to protect access to the UDP stack
  2. Initialize a **simple_pool_t** of **sel4osapi_udp_socket_server_t**
    - Pool size: SEL4OSAPI_UDP_MAX_SOCKETS
  3. Allocate an Endpoint for the stack server
  4. Create a **sel4osapi_thread_t** for the stack server (name: "udp::stack",
     routine: sel4osapi_udp_stack_thread).
  5. Start the "udp::stack" thread.

When a user process is initialized, a **sel4osapi_udp_interface** is configured
as part of **sel4osapi_system_initialize_process**.
  1. Create a **sel4osapi_mutex_t** to protect access to the interface
  2. Initialize a **simple_pool_t** of sel4osapi_udp_socket_t
    - Pool size: SEL4OSAPI_UDP_MAX_SOCKETS
  3. Verify that the stack server's EP has be passed in by the parent.

#### Socket creation

A process creates a socket using **sel4osapi_udp_create_socket**. which performs
the following operations:
  1. Lock the interface's mutex
  2. Allocate a **sel4osapi_udp_socket_t** from the process's
     **sel4osapi_udp_interface_t** **simple_pool_t**.
  3. Allocate a CNode to store the socket's Tx endpoint
  4. Set the CNode using **seL4_SetCapReceivePath**
  5. Request creation of the socket by **seL4_Call** the interface's EP with
     arguments:
     - MR[0]: opcode UDPSTACK_CREATE_SOCKET
     - MR[1]: id of the process' **sel4osapi_ipcclient_t**
     - MR[2]: IP address for the socket
  6. Check the error flag returned on MR[0]
  7. Store the socket id returned on MR[1]
  8. Reset **seL4_SetCapReceivePath**
  9. Unlock the interface's mutex

On the server side, the udp stack thread performs the following operations
upon detecting an opcode UDPSTACK_CREATE_SOCKET:
  1. Retrieve the **sel4osapi_ipcclient_t**
  2. Retrieve the **sel4osapi_netiface_t** containing a **sel4osapi_netvface_t**
     with a matching IP address.
  3. Allocate a **sel4osapi_udp_socket_server_t** from the UDP stack's
     **simple_pool_t**.
  4. Create a LwIP UDP PCB with **udp_new**.) for the new
     **sel4osapi_udp_socket_server_t**
  5. Create an Endpoint for the new **sel4osapi_udp_socket_server_t**
  6. Create a **sel4osapi_thread_t** (name: "udp-SOCKET_ID-tx", routine:
     sel4osapi_udp_socket_tx_thread) to handle data transmission requests for
     this socket.
  7. Make a copy the **sel4osapi_udp_socket_server_t** EP
  8. Set the minted cap using **seL4_SetCap**.
  9. Reply with **seL4_Reply** and the following arguments:
    - MR[0]: error flag
    - MR[1]: socket id.
  10. Start the **sel4osapi_udp_socket_server_t**.s thread.

```

                User Thread                      UDP Server Thread
                    +                                    +
  socket_create()   |                                    |
+---------------->+-+-+  call(udp_ep,CREATE,PID,addr)    |
                  |   +------------------------------->+-+-+
                  |   |                                |   | lookup_ipclient(PID)
                  |   |                                |   +--+
                  |   |                                |  <---+ipclient
                  +-+-+                                |   | lookup_netiface(addr)
                    |                                  |   +--+
                    |                                  |  <---+netiface
                    |                                  |   | new socket_server_t(ipcclient,netiface)
                    |                                  |   +--+
                    |                                  |  <---+ss
                    |                                  |   | udp_new()
                    |                                  |   +--+
                    |                                  |  <---+udp_pcb
                    |                                  |   | new Endpoint("tx-ready")
                    |                                  |   +--+
                    |                                  |  <---+tx_ready_ep                        Socket Server
                    |                                  |   | new thread("udp-SD-tx",socket_tx_ready)  Thread
                    |                                  |   +------------------------------------------->+
                    |                                  |   |                                            |
                    |    reply(err,ss.sd,tx_ready_ep)  |   |                                            |
                  +-+-+<-------------------------------+   | thread_start()                             |
                  |   |                                |   +----------------------------------------->+-+-+
                  |   |                                +-+-+                                          |   |
  socket_t*       |   |                                  |                                            |   |
<-----------------+-+-+                                  |                                            |   |
                    |                                    |                                            |   |
                    |                                    |                                            |   |
                    |                                    |                                            +-+-+
                    |                                    |                                              |
                    +                                    +                                              +

```

#### Socket binding

A process can enable receival of data from a socket by using
**sel4osapi_udp_socket_bind**. This operation binds a socket to a specific
UDP port and performs the following steps:
  1. Allocate a "data avaiable" AsyncEndpoint which will be used by the UDP
     stack to notify the client that messages have been received.
  2. Make a copy the AEP
  3. Allocate a CNode to receive the server's "rx ready" EP and store it with
     **seL4_SetCapReceivePath**
  4. Set the copied EP using **seL4_SetCap**
  5. Request binding by **seL4_Call** the **sel4osapi_udp_interface_t** with
     the following arguments:
     - MR[0]: opcode UDPSTACK_BIND_SOCKET
     - MR[1]: socket id
     - MR[2]: UDP port
  6. Check error flag returned on MR[0]
  7. Reset **seL4_SetCapReceivePath**

On the server side, the udp stack thread performs the following operations
upon detecting an opcode UDPSTACK_BIND_SOCKET:
  1. Retrieve the **sel4osapi_udp_socket_server_t** with the specified id
  2. Initialize a **simple_pool_t** of **sel4osapi_udp_message_t**
    - Pool size: SEL4OSAPI_UDP_MAX_MSGS_PER_CLIENT
  3. Create a **sel4osapi_mutex_t** to protect the messages pool.
  4. Create a **sel4osapi_thread_t** (name: "udp-SOCKET_ID-rx", routine:
     sel4osapi_udp_socket_rx_thread) to handle receive request from clients.
  5. Store the AEP received from the client in the socket
  6. Allocate a "rx ready" Endpoint which will be used to coordinate with the
     client when passing received messages via the IPC client's Rx buffer.
  7. Create a copy the Endpoint
  8. Lock the **sel4osapi_netiface_t**.s mutex
  9. Set the receive callback on the **sel4osapi_udp_socket_server_t**.s UDP PCB
     with LwIP's **udp_recv**.
    - This callback is notified when a new UDP message is available from LwIP
    - Messages are stored in the **sel4osapi_udp_socket_server_t**.s message pool
    - The EP supplied by the user is notified upon receival.
  10. Bind the UDP socket in LwIP using **udp_bind**.
  11. Unlock the **sel4osapi_netiface_t**.s mutex
  12. Reply with **seL4_Reply** and arguments:
    - MR[0]: error flag
    - Returned cap: copy of newly allocated Endpoint.
  13. Start the rx thread
  14. Allocate a new CNode and set it with **seL4_SetCapReceivePath**

#### Sending UDP packets

UDP packets can be sent over a socket using **sel4osapi_udp_socket_send** (or
**sel4osapi_udp_socket_send_sd**.. This operation
performs the following steps, on the client process' side:
  1. Take the IPC client's Tx buffer's semaphore
  2. Copy message to be sent into Tx buffer
  3. Request sending of message by **seL4_Call** the socket's tx EP with arguments:
     - MR[0]: message length
     - MR[1]: destination IP address
     - MR[2]: destination port
  4. Check error flag returned on MR[0]
  5. Release the IP client's Tx buffer by signaling its semaphore.

On the server side, the socket's tx thread waits on the socket's tx EP and
performs the following upon receiving a request:
  1. Truncate message to Tx buffer's length if length is too long
  2. Lock the **sel4osapi_netiface_t** by acquiring its mutex
  3. Allocate a LwIP **pbuf** using **pbuf_alloc**
  4. Copy the contents of the Tx buffer to the pbuf
  5. Send the pbuf using LwIP's **udp_sendto**
  6. Free the pbuf using **pbuf_free**
  7. Release the **sel4osapi_netiface_t**
  8. Reply with **seL4_Reply** with arguments:
     - MR[0]: error flag

#### Receiving UDP packets

In order to receive UDP packets, a client must first bind a socket using
**sel4osapi_udp_socket_bind**. then invoke **sel4osapi_udp_socket_recv** (or
**sel4osapi_udp_socket_recv_sd**. to wait for a UDP packet to be received. This
operation performs the following steps on the client process' side:
  1. Wait for data to be received by invoking **seL4_Wait** on the socket's "data
     available" AEP.
     - This AEP is notified by the callback installed in LwIP with **udp_recv**.
       which is called every time a new UDP packet is received on the UDP PCB.
  2. Take the IPC client's Rx buffer's semaphore
  3. Notify the socket server to copy the next message into the Rx buffer by
     **seL4_Call** on the server's rx ready EP with no additional arguments.
  4. Retrieve message metadata from server's reply:
     - MR[0]: message length
     - MR[1]: source port
     - MR[2]: source address
  5. Check that the buffer provided by user is big enough to store the new message.
  6. Copy the message from the IPC client's Rx buffer to the user-provided buffer.
  7. Release the IPC client's Rx buffer by signaling its semaphore.

On the server side, the socket's rx thread waits on the socket's rx ready EP and
performs the following upon receiving a request:
  1. Acquire the mutex on the socket server's incoming messages queue.
  2. Retrieve the next message from the incoming messages queue.
  3. Release the mutex on the socket server's incoming messages queue.
  4. Iterate over the pbuf's sub-pbuf and copy them into the Rx buffer
  5. Reply with **seL4_Reply** and arguments:
     - MR[0]: total bytes copied to Rx buffer
     - MR[1]: packet's source port
     - MR[2]: packet's source address
  6. Acquire the mutex on the socket server's incoming messages queue.
  7. Return the message to the socket server's message pool.
  8. Release the mutex on the socket server's incoming messages queue.
  9. Notify the client's data available AEP if there are still messages in
     the incoming messages queue.

## User Processes

libsel4osapi implements a User Process abstraction similar to Unix's. In seL4
terms, each process is a seL4 Thread with its own virtual memory address space
and capability space.

The abstraction supports **user** processes written with a minimal amount
of library-specific code, to facilitate development and porting of applications
to seL4.

As detailed in [User process initialization](#user-process-initialization),
a process must use **sel4osapi_system_initialize_process** in its **main**
function to properly initialize libsel4osapi inside its environment.

Processes are represented within the root task by **sel4osapi_process_t**
data structures, allocated in a **simple_pool_t** of size
SEL4OSAPI_USER_PROCESS_MAX.

A process' environment is represented by a **sel4osapi_process_env_t**. which
can be accessed using **sel4osapi_process_get_current**.

Currently, the API does not support recursive creation of processes. User
processes can only be spawned from the root task.

A user process can, on the other hand, create up to
SEL4OSAPI_MAX_THREADS_PER_PROCESS user threads, which will share its
virtual memory address space and capability space, as detailed in
[User threads](#user-threads).

### Process creation

A user process is created with **sel4osapi_process_create**. by specifying the
name of the process and its priority.

An ELF image with the same name must have been included in the seL4 system image
at build time.

Creation of a user process within the root task includes the following step:
  1. Allocate a **sel4osapi_process_t** from the system's process simple_pool_t.
  2. Map a new page in the root task's address space to store the new process'
     **sel4osapi_process_env_t**.
  3. Initialize a **sel4utils_process_t** using **sel4utils_configure_process**
     - From seL4 docs:
         This is the function to use if you just want to set up a process as
         fast as possible. It creates a simple cspace and vspace for you,
         allocates a fault endpoint and puts it into the new cspace. It loads
         the elf file into the new vspace, parses the elf file and stores the
         entry point into process->entry_point. It uses the same vka for both
         allocations - the new process will not have an allocator. The process
         will have just one thread.
  4. Create a serial client for the process by calling
     **sel4osapi_serial_create_client**.
     - This operation creates the process' serial client used within the root
       task's environment by the Serial server (see
       [Serial support](#serial-support)).
  5. Create an IPC client for the process by calling
     **sel4osapi_ipc_create_client**.
     - This operation creates the process' IPC client used within the root
       task's environment by the UDP socket servers (see
       [UDP/IP network support](#udpip-network-support)).
  6. Initialize the process' environment by calling **sel4osapi_process_init_env**
     - Assign PID
     - Assign process name
     - Assign process priority
     - Copy cap to process' Page Directory into the process' CSpace using
       **sel4osapi_process_copy_cap_into**.
     - Set process' root CNode to SEL4UTILS_CNODE_SLOT (1).
     - Copy cap to process' TCB into the process' CSPace using
       **sel4osapi_process_copy_cap_into**.
     - Reserve untyped memory objects to provide at least
       SEL4OSAPI_USER_PROCESS_UNTYPED_MEM_SIZE memory to the process
       - Current implementation is trivial and inefficient, since it allocates
         the first untyped available until the reserved size is at least the
         required amount, possibly assigning (much) more memory than requested.
     - Copy cap to the SysClock's server EP into the process' CSpace.
     - Allocate an AsyncEndpoint to support **sel4osapi_idle** for the process
     - Initialize the process' IPC client
       - Retrieve caps to the 4K memory pages where the IPC client's Rx and Tx
         buffers are mapped (in the root task's address space), using
         **vspace_get_cap** on the root task's **vspace_t**.
       - Map pages for the Rx and Tx buffers into the process' **vspace_t** using
         **vspace_map_pages**
     - Copy cap to the UDP stack's server EP into the process' CSpace.
     - Initialize the process' Serial client
       - Retrieve caps to the 4K memory pages where the Serial client's memory
         buffer is mapped (in the root task's address space), using
         **vspace_get_cap** on the root task's **vspace_t**.
       - Map pages for the memory buffer into the process' **vspace_t** using
         **vspace_map_pages**
     - Copy cap to the process' Fault EP into the process' CSpace.
     - Configure the initial free slot in the process' CSpace.

When a process is spawned and its **main** function launched, the
**sel4osapi_system_initialize_process** must be invoked to complete initialization
of the **sel4osapi_process_env_t** data structure, as detailed in
[User process initialization](#user-process-initialization).

#### Capability transfer

Capabilities are transferred from the root task to a user process by using
**sel4osapi_process_copy_cap_into**.

This operation uses **sel4utils_mint_cap_to_process** with a badge equal to the
process' PID.

### Process lifecycle

Once a user process has been created with **sel4osapi_process_create**. it can
be started with **sel4osapi_process_spawn**. This operation performs the following
steps:
  1. Construct the argv and argc arguments for the process
     - argv[0]: process name
     - argv[1]: process PID
     - argv[2]: fault endpoint
  2. Spawn the process using **sel4utils_spawn_process_v**
  3. Get cap to process' **sel4osapi_process_env_t** in the root task's CSpace
  4. Copy the cap using **sel4osapi_util_copy_cap**
  5. Map the copied cap into the process' **vspace_t** using **vspace_map_pages**.
     - This operation returns the virtual memory address (in the process'
       **vspace_t**. where the process' **sel4osapi_process_env_t** is mapped.
  6. Send a message to the process by **seL4_Send** on the process' fault EP
     with arguments:
     - MR[0]: address of the **sel4osapi_process_env_t** in the process' **vspace_t**.

As detailed in [User process initialization](#user-process-initialization),
the process will parse the EP from argv and **seL4_Wait** on it to receive the
address of the **sel4osapi_process_env_t** structure.

After completing **sel4osapi_system_initialize_process**. the process can
perform its operations normally, including using the libsel4osapi API.

Once operations have been completed and before returning, the process should call
**sel4osapi_process_signal_root** to signal the root task of its termination.

The root task can wait for the termination of a process by using
**sel4osapi_process_join**. This operation performs a **seL4_Wait** on the
process' Fault EP, which is signaled in case of a fault in the process, or upon
process termination, when **sel4osapi_process_signal_root** is called.

## User Threads

In addition to supporting user process, libsel4osapi also supports creation
of user threads within these processes.

A user thread is represented by a **sel4osapi_thread_t** data structure, allocated
from a **simple_pool_t** in a **sel4osapi_process_t** of size
SEL4OSAPI_MAX_THREADS_PER_PROCESS.

A user thread is a seL4 Thread which shares virtual memory address space and
capability space with its parent seL4 Thread (ie. the parent user process).

### Thread creation

A user thread is created with **sel4osapi_thread_create**. by specifying:
  - A thread name
  - A thread routine (of type **sel4osapi_thread_routine_fn**.
  - An optional thread argument
  - The thread's priority

Creation of a user thread performs the following steps:
  1. Allocate a **sel4osapi_thread_t** from the process' **simple_pool_t**
  2. Allocate an Endpoint that will be signaled by the thread upon termination
     to communicate its termination status to the parent.
  3. Allocate an AsyncEndpoint which will be used to suspend/resume the thread.
  4. Assign the thread's fault EP to be the process' fault EP
  5. Configure a seL4 thread using **sel4utils_configure_thread**
    - This operations configures a **sel4utils_thread_t** to use the parent
      process' **vspace_T** and CSpace
  6. Initialize the thread's **sel4osapi_thread_info_t**
    - This data structure can be accessed using **sel4osapi_thread_get_current**
      and it's stored using **seL4_SetUserData**.

A thread is started using **sel4osapi_thread_start**. which uses
**sel4utils_start_thread** to spawn **sel4osapi_thread_routine_wrapper**.

This routine wraps the user-provided thread routine:
  1. Initialize thread local storage using **sel4osapi_thread_init_tls**
     - Save the current IPC buffer in the thread's **sel4osapi_thread_info_t**
     - Store the thread's **sel4osapi_thread_info_t** using **seL4_SetUserData**.
  2. Set the thread to "active" in its **sel4osapi_thread_info_t**
  3. Invoke the user's thread routine
  4. Set the thread  to "active" in its **sel4osapi_thread_info_t**
  5. Notify parent of termination by **seL4_Call** on the thread's termination
     EP
     - Result should be passed via MR[0] (currently hardcoded to success).
  6. Yield control using **seL4_Yield**.

### Thread lifecycle

The root task can wait for the termination of a thread by calling
**sel4osapi_thread_join**. This operation calls **seL4_Wait** on the thread's
termination EP and then returns the thread's exit code (found on MR[0]).

After termination, a thread's resources can be disposed with
**sel4osapi_thread_delete**.

### Thread sleep

A thread can be suspended for a finite period of time using **sel4osapi_thread_sleep**.

This operation is implemented by scheduling a one-shot timeout on the thread's
synchronization AEP, using **sel4osapi_sysclock_schedule_timeout**. followed by
**sel4osapi_sysclock_wait_for_timeout**.

### Default Thread

Every process, including the root task, has at least one thread, whose
**sel4osapi_thread_info_t** can be accessed with sel4osapi_thread_get_current.

## Synchronization Primitives

libsel4osapi provides some basic synchronization primitives which can be
used to control interactions between user threads.

### Mutex

A **sel4osapi_mutex_t** is a recursive mutex, implemented on top of a seL4
AsyncEndpoint.

Mutexes are dynamically allocated using **sel4osapi_heap_allocate**.

A mutex is acquired with **sel4osapi_mutex_lock**. which invokes **seL4_Wait**
on the mutex AEP if the mutex's owner is not the invoking thread. Once a mutex
has been acquired by a thread, the thread can invoke **sel4osapi_mutex_lock**
multiple times, but the mutex must be released the same number of times, using
**sel4osapi_mutex_unlock**.

**sel4osapi_mutex_unlock** notifies the mutex's AEP, with **seL4_Notify**. once the
mutex has been unlocked a sufficient amount of times.

### Semaphore

A **sel4osapi_semaphore_t** is binary semaphore, implemented on top of a
**sel4osapi_mutex_t**.

Semaphores are dynamically allocated using **sel4osapi_heap_allocate**.

A semaphore is acquired using **sel4osapi_semaphore_take**. This operation
suspends the calling threads and puts its "wait AEP" into a queue. The operation
supports an optional timeout, after which the thread is resumed if the semaphore
has not been released within the specified amount of time.

The semaphore can be released using **sel4osapi_semaphore_give**. which notifies
the AEP of all threads currently queued on it. Depending on scheduling, only one
waiting thread will be able to acquire the semaphore, while the other will be
suspended and put back in the queue again.

## Utility features

libsel4osapi includes some utility features which are used internally to
implement other services, and are also made available to users to help with
development on seL4.

### simple_list_t

**simple_list_t** is an implemenation of linked list.

New nodes can be dynamically allocated and appended to the list using
**simple_list_insert**.

A list element is removed with **simple_list_unlink**.

Each element can store a void* argument.

All elements contained in a list can be serialized to an array using
**simple_list_to_array**.

### simple_pool_t

**simple_pool_t** is an implementation of a pre-allocated memory pool of customizable
size.

A new pool is created using **simple_pool_new**. and specifying:
  - Total number of elements to be allocated by the pool
  - Memory size of each element in the pool
  - Element initialization function (of type **simple_pool_init_el_fn**.
  - An optional initialization argument
  - An optional compate functio (of type simple_pool_compare_el_fn)

All elements are pre-allocated at creation, and can then be retrieved from
the pool using **simple_pool_alloc**. This operation returns a pointer to a new
element, which can then be returned using **simple_pool_free**.

The pool has two **simple_list_t**. one for free elements, and one for allocated
elements. The allocated elements list can be queried using **simple_pool_find_node**.

This operation relies on the compare function if available, or compares
pointers otherwise.

### Memory allocation

libsel4osapi provides two utility functions for handling dynamic memory
allocations, **sel4osapi_heap_allocate** and **sel4osapi_heap_free**.

These operations currently map 1-1 to libmuslc's **malloc** and **free**, thus
they draw on virtual memory reserved during initialization (see
[System interface](#system-interface)).

## Revision History

| Version     | Date        | Author            | Comment                   |
|-------------|-------------|-------------------|---------------------------|
| 1.0         | 2016/10/26  | asorbini@rti.com  | Created                   |
