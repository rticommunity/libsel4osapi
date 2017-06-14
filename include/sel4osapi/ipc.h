/*
 * FILE: ipc.h - IPC via shared memory for sel4osapi
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#ifndef SEL4OSAPI_IPC_H_
#define SEL4OSAPI_IPC_H_


typedef struct sel4osapi_ipcclient
{
    int id;
    sel4osapi_semaphore_t *tx_buf_avail;
    sel4osapi_semaphore_t *rx_buf_avail;
    void *rx_buf;
    uint32_t rx_buf_size;
    void *tx_buf;
    uint32_t tx_buf_size;
} sel4osapi_ipcclient_t;

typedef struct sel4osapi_ipcserver
{
    sel4osapi_mutex_t *mutex;
    simple_pool_t *clients;
} sel4osapi_ipcserver_t;


int
sel4osapi_ipcserver_initialize(sel4osapi_ipcserver_t *ipc);

sel4osapi_ipcclient_t*
sel4osapi_ipc_create_client(sel4osapi_ipcserver_t *ipc, int id);



#endif /* SEL4OSAPI_NETWORK_H_ */
