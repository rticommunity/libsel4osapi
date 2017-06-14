/*
 * FILE: ipc.c - IPC via shared memory for sel4osapi
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#include <sel4osapi/osapi.h>

#include <sel4utils/page_dma.h>


int
sel4osapi_ipcserver_initialize(sel4osapi_ipcserver_t *ipc)
{
    UNUSED int error = 0;

    ipc->mutex = sel4osapi_mutex_create();
    assert(ipc->mutex);

    ipc->clients = simple_pool_new(SEL4OSAPI_USER_PROCESS_MAX, sizeof(sel4osapi_ipcclient_t), NULL, NULL, NULL);
    assert(ipc->clients);

    syslog_trace("ipc server initialized.");

    return 0;
}

sel4osapi_ipcclient_t*
sel4osapi_ipc_create_client(sel4osapi_ipcserver_t *ipc, int id)
{
    vspace_t *vspace = sel4osapi_system_get_vspace();
    sel4osapi_ipcclient_t * client = NULL;

    assert(id >= 0);

    client = (sel4osapi_ipcclient_t*) simple_pool_alloc(ipc->clients);
    assert(client != NULL);

    client->id = id;

    client->rx_buf_avail = sel4osapi_semaphore_create(1);
    assert(client->rx_buf_avail);
    client->tx_buf_avail = sel4osapi_semaphore_create(1);
    assert(client->tx_buf_avail);

    client->rx_buf = vspace_new_pages(vspace, seL4_AllRights, SEL4OSAPI_PROCESS_RX_BUF_PAGES, PAGE_BITS_4K);
    assert(client->rx_buf != NULL);
    client->rx_buf_size = SEL4OSAPI_PROCESS_RX_BUF_SIZE;

    client->tx_buf = vspace_new_pages(vspace, seL4_AllRights, SEL4OSAPI_PROCESS_TX_BUF_PAGES, PAGE_BITS_4K);
    assert(client->tx_buf != NULL);
    client->tx_buf_size = SEL4OSAPI_PROCESS_TX_BUF_SIZE;

    return client;
}
