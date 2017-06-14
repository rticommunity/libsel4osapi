/*
 * FILE: network.h - network communications for sel4osapi
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#ifndef SEL4OSAPI_NETWORK_H_
#define SEL4OSAPI_NETWORK_H_

#define SEL4OSAPI_IP_ADDR_DEFAULT           CONFIG_LIB_OSAPI_IP_ADDR
#define SEL4OSAPI_IP_GW_DEFAULT             CONFIG_LIB_OSAPI_IP_GW
#define SEL4OSAPI_IP_MASK_DEFAULT           CONFIG_LIB_OSAPI_IP_MASK

#define SEL4OSAPI_NET_VFACES_MAX            CONFIG_LIB_OSAPI_NET_VFACES_MAX

#define SEL4OSAPI_NET_PHYS_IFACES           CONFIG_LIB_OSAPI_NET_PHYS_MAX

#define SEL4OSAPI_NETIFACE_NAME_MAX_LEN     CONFIG_LIB_OSAPI_NET_NAME_MAX

#define SEL4OSAPI_NETIFACE_NAME_DEFAULT     CONFIG_LIB_OSAPI_NET_NAME


typedef struct sel4osapi_netstack
{
    sel4osapi_mutex_t *mutex;
    simple_pool_t *ifaces;
    sel4osapi_ipcserver_t *ipc;
} sel4osapi_netstack_t;

typedef struct sel4osapi_netvface
{
    struct netif lwip_netif;
    ip_addr_t ip_mask;
    ip_addr_t ip_addr;
    ip_addr_t ip_gw;
} sel4osapi_netvface_t;

typedef void (*sel4osapi_netif_handle_irq_fn)(void *state, int irq_num);

typedef struct sel4osapi_netiface_driver
{
    int irq_num;
    netif_init_fn init_fn;
    sel4osapi_netif_handle_irq_fn handle_irq_fn;
    void *state;
} sel4osapi_netiface_driver_t;


typedef struct sel4osapi_netiface
{
    char name[SEL4OSAPI_NETIFACE_NAME_MAX_LEN];
    sel4osapi_mutex_t *mutex;
    sel4osapi_netiface_driver_t *driver;
    seL4_CPtr irq;
    seL4_CPtr irq_aep;
    sel4osapi_thread_t *irq_thread;
    simple_pool_t *vfaces;
    uint32_t last_checked_ip_t;
    uint32_t last_checked_arp_t;
} sel4osapi_netiface_t ;


int
sel4osapi_network_initialize(sel4osapi_netstack_t *net, sel4osapi_ipcserver_t *ipc,
        char *iface_name, sel4osapi_netiface_driver_t *iface_driver);

sel4osapi_netiface_t*
sel4osapi_network_create_interface(sel4osapi_netstack_t *net, char *name, sel4osapi_netiface_driver_t *driver);


#endif /* SEL4OSAPI_NETWORK_H_ */
