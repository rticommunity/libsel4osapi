/*
 * FILE: io.h - IO functions for sel4osapi
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#ifndef SEL4OSAPI_IO_H_
#define SEL4OSAPI_IO_H_


void
sel4osapi_io_initialize(void);

#ifdef CONFIG_LIB_OSAPI_SERIAL

#define SEL4OSAPI_SERIAL_BUF_SIZE       1 << 12
#define SEL4OSAPI_SERIAL_BUF_PAGES      (SEL4OSAPI_SERIAL_BUF_SIZE/PAGE_SIZE_4K)

typedef enum sel4osapi_serialdevice
{
    SERIAL_DEV_UART1 = 1,
    SERIAL_DEV_UART2 = 2
} sel4osapi_serialdevice_t;

typedef struct sel4osapi_serialclient
{
    int id;
    sel4osapi_semaphore_t *buf_avail;
    void *buf;
    uint32_t buf_size;
    seL4_CPtr server_ep;
} sel4osapi_serialclient_t;

typedef struct sel4osapi_serialserver
{
    seL4_CPtr server_ep;
    sel4osapi_mutex_t *mutex;
    simple_pool_t *clients;
    sel4osapi_thread_t *thread;
    ps_chardevice_t uart1, uart2;
} sel4osapi_serialserver_t;



void
sel4osapi_io_serial_initialize(sel4osapi_serialserver_t *server, int priority);

sel4osapi_serialclient_t *
sel4osapi_io_serial_create_client(sel4osapi_serialserver_t *server);

int
sel4osapi_io_serial_write(sel4osapi_serialdevice_t dev, void *data, uint32_t size);

int
sel4osapi_io_serial_read(sel4osapi_serialdevice_t dev, void *data, uint32_t size);
#endif


#endif /* SEL4OSAPI_IO_H_ */
