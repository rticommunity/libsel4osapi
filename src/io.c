/*
 * FILE: io.c - IO subsystem functions for sel4osapi
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#include <sel4osapi/osapi.h>
#include <sel4platsupport/io.h>
#include <sel4utils/page_dma.h>

#include <sel4platsupport/platsupport.h>

#include <platsupport/serial.h>
#include <platsupport/chardev.h>

#include <string.h>



void
sel4osapi_io_initialize(ps_io_ops_t *io_ops) {
#if defined(CONFIG_LIB_OSAPI_SERIAL) || defined(CONFIG_LIB_OSAPI_NET)
    int error = 0;
    vspace_t *vspace = sel4osapi_system_get_vspace();
    vka_t *vka = sel4osapi_system_get_vka();

    error = sel4platsupport_new_io_ops(*vspace, *vka, io_ops);
    assert(error == 0);
    error = sel4utils_new_page_dma_alloc(vka, vspace, &io_ops->dma_manager);
    assert(error == 0);
#endif
}


#ifdef CONFIG_LIB_OSAPI_SERIAL
typedef enum sel4osapi_serial_op
{
    SERIAL_OP_WRITE = 201,
    SERIAL_OP_READ = 202,
    SERIAL_OP_CONFIG = 203
} sel4osapi_serial_op_t;

void
sel4osapi_serial_server_thread(sel4osapi_thread_info_t *thread)
{
    sel4osapi_serialserver_t *server = (sel4osapi_serialserver_t *)thread->arg;
    seL4_Word sender_badge;
    seL4_Word mr0, mr1, mr2, mr3;
    seL4_MessageInfo_t minfo;
    int client_id = 0;
    sel4osapi_serial_op_t opcode  = 0;
    sel4osapi_serialdevice_t devid = 0;
    uint32_t op_size = 0;
    sel4osapi_serialclient_t *client = NULL;
    ps_chardevice_t *device = NULL;

    syslog_trace("started serving requests...");

    while (thread->active)
    {
            minfo = seL4_Recv(server->server_ep, &sender_badge);
            assert(seL4_MessageInfo_get_length(minfo) == 4);
            mr0 = seL4_GetMR(0);
            mr1 = seL4_GetMR(1);
            mr2 = seL4_GetMR(2);
            mr3 = seL4_GetMR(3);

            client_id = mr0;
            opcode = mr1;
            devid = mr2;

            sel4osapi_list_t *cursor = server->clients->entries;
            while (cursor != NULL)
            {
                client = (sel4osapi_serialclient_t*) cursor->el;
                if (client->id == client_id)
                {
                    break;
                }
                client = NULL;
                cursor = cursor->next;
            }
            assert(client);
            assert(op_size <= client->buf_size);

            switch (devid) {
                case SERIAL_DEV_UART1:
                    device = &server->uart1;
                    break;
                case SERIAL_DEV_UART2:
                    device = &server->uart2;
                    break;
                default:
                    device = NULL;
                    break;
            }
            assert(device);

            switch (opcode) {
                case SERIAL_OP_WRITE: {
                    int i;
                    op_size = mr3;
                    for (i = 0; i < op_size; ++i) {
                        ps_cdev_putchar(device, ((char *)client->buf)[i]);
                    }
                    mr0 = op_size;
                    minfo = seL4_MessageInfo_new(0,0,0,1);
                    seL4_SetMR(0, mr0);
                    seL4_Reply(minfo);
                    break;
                }

                case SERIAL_OP_READ: {
                    int i;
                    int ret = EOF;
                    uint32_t tStart, tNow;
                    op_size = mr3;
                    size_t timeout = *((size_t *)client->buf);
                    tStart = sel4osapi_sysclock_get_time();
                    for (i = 0; i < op_size; ++i) {
                        while (ret == EOF) {
                            tNow = sel4osapi_sysclock_get_time();
                            if ((timeout > 0) && ((tNow - tStart) > timeout)) {
                                break;
                            }
                            ret = ps_cdev_getchar(device);
                        }
                        if (ret == EOF) {
                            break;
                        }
                        ((char *)(client->buf))[i] = ret;
                    }

                    mr0 = i;
                    minfo = seL4_MessageInfo_new(0,0,0,1);
                    seL4_SetMR(0, mr0);
                    seL4_Reply(minfo);
                    break;
                }
                case SERIAL_OP_CONFIG: {
                    sel4osapi_serial_config_t *config = (sel4osapi_serial_config_t *)client->buf;
                    mr0 = serial_configure(device, config->bps, config->char_size, config->parity, config->stop_bit);
                    minfo = seL4_MessageInfo_new(0,0,0,1);
                    seL4_SetMR(0, mr0);
                    seL4_Reply(minfo);
                    break;
                }

            }
    }

}

int
sel4osapi_io_serial_write(sel4osapi_serialdevice_t dev, void *data, uint32_t size)
{
    seL4_MessageInfo_t minfo;
    seL4_Word mr0, mr1, mr2, mr3;
    sel4osapi_process_env_t *process = sel4osapi_process_get_current();
    sel4osapi_serialclient_t *client = &process->serial;

    assert(size <= client->buf_size);

    sel4osapi_semaphore_take(client->buf_avail, 0);

    memcpy(client->buf, data, size);

    mr0 = client->id;
    mr1 = SERIAL_OP_WRITE;
    mr2 = dev;
    mr3 = size;
    seL4_SetMR(0, mr0);
    seL4_SetMR(1, mr1);
    seL4_SetMR(2, mr2);
    seL4_SetMR(3, mr3);
    minfo = seL4_MessageInfo_new(0,0,0,4);

    minfo = seL4_Call(client->server_ep, minfo);
    assert(seL4_MessageInfo_get_length(minfo) == 1);
    mr0 = seL4_GetMR(0);

    sel4osapi_semaphore_give(client->buf_avail);

    return mr0;
}

int
sel4osapi_io_serial_read(sel4osapi_serialdevice_t dev, void *data, uint32_t size, size_t timeout)
{
    seL4_MessageInfo_t minfo;
    seL4_Word mr0, mr1, mr2, mr3;
    sel4osapi_process_env_t *process = sel4osapi_process_get_current();
    sel4osapi_serialclient_t *client = &process->serial;
    size_t *ptrTimeout;

    assert(size <= client->buf_size);

    sel4osapi_semaphore_take(client->buf_avail, 0);
    ptrTimeout = (size_t *)client->buf;
    *ptrTimeout = timeout;

    mr0 = client->id;
    mr1 = SERIAL_OP_READ;
    mr2 = dev;
    mr3 = size;
    seL4_SetMR(0, mr0);
    seL4_SetMR(1, mr1);
    seL4_SetMR(2, mr2);
    seL4_SetMR(3, mr3);
    minfo = seL4_MessageInfo_new(0,0,0,4);

    minfo = seL4_Call(client->server_ep, minfo);
    assert(seL4_MessageInfo_get_length(minfo) == 1);
    mr0 = seL4_GetMR(0);

    if (mr0 > 0)
    {
        memcpy(data, client->buf, mr0);
    }

    sel4osapi_semaphore_give(client->buf_avail);

    return mr0;
}

int
sel4osapi_io_serial_configure(sel4osapi_serialdevice_t dev, 
        long bps,
        int  char_size,
        enum sel4osapi_serial_parity parity,
        int  stop_bit)
{
    seL4_MessageInfo_t minfo;
    seL4_Word mr0, mr1, mr2, mr3;
    sel4osapi_process_env_t *process = sel4osapi_process_get_current();
    sel4osapi_serialclient_t *client = &process->serial;
    sel4osapi_serial_config_t *config;

    sel4osapi_semaphore_take(client->buf_avail, 0);
    config = (sel4osapi_serial_config_t *)client->buf;
    config->bps = bps;
    config->char_size = char_size;
    config->parity = (enum serial_parity)parity;
    config->stop_bit = stop_bit;

    mr0 = client->id;
    mr1 = SERIAL_OP_CONFIG;
    mr2 = dev;
    mr3 = sizeof(sel4osapi_serial_config_t);
    seL4_SetMR(0, mr0);
    seL4_SetMR(1, mr1);
    seL4_SetMR(2, mr2);
    seL4_SetMR(3, mr3);
    minfo = seL4_MessageInfo_new(0,0,0,4);

    minfo = seL4_Call(client->server_ep, minfo);
    assert(seL4_MessageInfo_get_length(minfo) == 1);
    mr0 = seL4_GetMR(0);
    sel4osapi_semaphore_give(client->buf_avail);

    return mr0;
}

sel4osapi_serialclient_t *
sel4osapi_io_serial_create_client(sel4osapi_serialserver_t *server)
{
    vspace_t *vspace = sel4osapi_system_get_vspace();
    sel4osapi_serialclient_t *client = NULL;

    client = simple_pool_alloc(server->clients);
    assert(client);

    client->id = simple_pool_get_current_size(server->clients);

    client->buf_avail = sel4osapi_semaphore_create(1);
    assert(client->buf_avail);

    client->buf = vspace_new_pages(vspace, seL4_AllRights, SEL4OSAPI_SERIAL_BUF_PAGES, PAGE_BITS_4K);
    assert(client->buf != NULL);
    client->buf_size = SEL4OSAPI_SERIAL_BUF_SIZE;

    client->server_ep = server->server_ep;

    return client;
}

void
sel4osapi_io_serial_initialize(sel4osapi_serialserver_t *server, int priority)
{
    int error = 0;
    ps_chardevice_t *chardev;

    vspace_t *vspace = sel4osapi_system_get_vspace();
    simple_t *simple = sel4osapi_system_get_simple();
    vka_t *vka = sel4osapi_system_get_vka();
#if defined(CONFIG_LIB_OSAPI_SERIAL_UART1) || defined(CONFIG_LIB_OSAPI_SERIAL_UART2)
    ps_io_ops_t *io_ops = sel4osapi_system_get_io_ops();
#endif

    error = platsupport_serial_setup_simple(vspace, simple, vka);
    assert(error == 0);

#ifdef CONFIG_LIB_OSAPI_SERIAL_UART1
    /*Open serial port one */
    chardev = ps_cdev_init(PS_SERIAL0, io_ops, &server->uart1);
    assert(chardev);
    //serial_configure(chardev, 9600, 8, PARITY_ODD, 1);
    
#endif

#ifdef CONFIG_LIB_OSAPI_SERIAL_UART2
    /*Open serial port two*/
    chardev = ps_cdev_init(PS_SERIAL1, io_ops, &server->uart2);
    assert(chardev);
#endif

    server->clients = simple_pool_new(SEL4OSAPI_USER_PROCESS_MAX, sizeof(sel4osapi_serialclient_t), NULL, NULL, NULL);
    assert(server->clients);

    server->mutex = sel4osapi_mutex_create();
    assert(server->mutex);

    server->server_ep = vka_alloc_endpoint_leaky(vka);
    assert(server->server_ep != seL4_CapNull);

    server->thread = sel4osapi_thread_create("serial", sel4osapi_serial_server_thread, server, priority);
    assert(server->thread);

    error = sel4osapi_thread_start(server->thread);
    assert(!error);
}

#endif
