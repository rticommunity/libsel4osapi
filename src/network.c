/*
 * FILE: network.c - network communication for sel4osapi
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
#include <lwip/init.h>
#include <netif/etharp.h>
#include <lwip/ip_frag.h>

void
sel4osapi_eth_irq1_thread(sel4osapi_thread_info_t *thread)
{
    sel4osapi_netiface_t *iface = (sel4osapi_netiface_t *) thread->arg;
#if CLEAR_BUFFERS
    uint32_t current_time = 0;
    uint32_t elapsed_ip_t = 0, elapsed_arp_t = 0;
#endif
    int error = 0;

    syslog_trace("ethernet driver started");
    while (thread->active)
    {
        seL4_Wait(iface->irq_aep,NULL);
        /*syslog_trace_a("handling IRQ %d",iface->driver->irq_num);*/
        error = sel4osapi_mutex_lock(iface->mutex);
        assert(!error);

#if CLEAR_BUFFERS
        current_time = sel4osapi_sysclock_get_time();

        elapsed_ip_t = current_time - iface->last_checked_ip_t;
        elapsed_arp_t = current_time - iface->last_checked_arp_t;
#endif

        iface->driver->handle_irq_fn(iface->driver->state, iface->driver->irq_num);

#if CLEAR_BUFFERS
        if (elapsed_arp_t > ARP_TMR_INTERVAL)
        {
            etharp_tmr();
            iface->last_checked_arp_t = current_time;
        }
        if (elapsed_ip_t > IP_TMR_INTERVAL)
        {
            ip_reass_tmr();
            iface->last_checked_ip_t = current_time;
        }
#endif

        seL4_IRQHandler_Ack(iface->irq);
        sel4osapi_mutex_unlock(iface->mutex);
        /*syslog_trace_a("IRQ %d handled",iface->driver->irq_num);*/
    }
}

int
sel4osapi_netiface_add_vface(sel4osapi_netiface_t *iface,
        ip_addr_t *addr, ip_addr_t *mask, ip_addr_t *gw, unsigned char is_default)
{
    struct netif *netif;
    sel4osapi_netvface_t *vface = NULL;

    assert(iface);
    assert(addr);
    assert(mask);
    assert(gw);

    vface = simple_pool_alloc(iface->vfaces);
    assert(vface);

    vface->ip_addr = *addr;
    vface->ip_mask = *mask;
    vface->ip_gw = *gw;

    netif = netif_add(&vface->lwip_netif,
                            addr, &vface->ip_mask, &vface->ip_gw,
                            iface->driver->state, iface->driver->init_fn,
                            ethernet_input);
    assert(netif);
    netif_set_up(netif);

    if (is_default)
    {
        netif_set_default(netif);
    }

    return 0;
}

sel4osapi_netiface_t*
sel4osapi_network_create_interface(sel4osapi_netstack_t *net, char *name, sel4osapi_netiface_driver_t *driver)
{
    sel4osapi_process_env_t *process = sel4osapi_process_get_current();
    vka_t *vka = sel4osapi_system_get_vka();
    simple_t *simple = sel4osapi_system_get_simple();
    sel4osapi_netiface_t *iface = NULL;
    vka_object_t irq_aep_obj = { 0 };
    cspacepath_t irq_path = { 0 };
    int error = 0;

    assert(net);
    assert(name);
    assert(driver);
    assert(driver->handle_irq_fn);
    assert(driver->init_fn);
    assert(driver->irq_num > 0);

    iface = simple_pool_alloc(net->ifaces);
    assert(iface);

    iface->mutex = sel4osapi_mutex_create();
    assert(iface->mutex);

    {
        iface->driver = driver;
    }
    {
        error = vka_cspace_alloc(vka, &iface->irq);
        assert(error == 0);
        vka_cspace_make_path(vka, iface->irq, &irq_path);

        error = simple_get_IRQ_control(simple, iface->driver->irq_num, irq_path);
        assert(error == 0);

        error = vka_alloc_async_endpoint(vka, &irq_aep_obj);
        assert(error == 0);

        iface->irq_aep = irq_aep_obj.cptr;
        error = seL4_IRQHandler_SetEndpoint(irq_path.capPtr, iface->irq_aep);
        assert(error == 0);
    }

    {
        syslog_trace("initializing lwip...");
        lwip_init();
    }
    {
        iface->vfaces = simple_pool_new(SEL4OSAPI_NET_VFACES_MAX, sizeof(sel4osapi_netvface_t), NULL, NULL, NULL);
        assert(iface->vfaces);
    }
    {
        char tname[SEL4OSAPI_THREAD_NAME_MAX_LEN];
        snprintf(tname, SEL4OSAPI_THREAD_NAME_MAX_LEN, "%s::irq", name);
        iface->irq_thread = sel4osapi_thread_create("eth::irq",sel4osapi_eth_irq1_thread,iface,process->priority);
        assert(iface->irq_thread != NULL);
    }
    {
        iface->last_checked_ip_t = 0;
        iface->last_checked_arp_t = 0;
    }

    return iface;
}


int
sel4osapi_network_initialize(sel4osapi_netstack_t *net, sel4osapi_ipcserver_t *ipc,
        char *iface_name, sel4osapi_netiface_driver_t *iface_driver)
{
    UNUSED int error = 0;
    sel4osapi_netiface_t *iface = NULL;

    net->mutex = sel4osapi_mutex_create();
    assert(net->mutex);

    net->ifaces = simple_pool_new(SEL4OSAPI_NET_PHYS_IFACES, sizeof(sel4osapi_netiface_t), NULL, NULL, NULL);
    assert(net->ifaces);

    net->ipc = ipc;

    syslog_info_a("initializing network interface [%s, irq=%d]", iface_name, iface_driver->irq_num)

    iface = sel4osapi_network_create_interface(net, iface_name, iface_driver);
    assert(iface);

    {
        ip_addr_t addr, gw, mask;
        ipaddr_aton(SEL4OSAPI_IP_GW_DEFAULT,        &gw);
        ipaddr_aton(SEL4OSAPI_IP_ADDR_DEFAULT,  &addr);
        ipaddr_aton(SEL4OSAPI_IP_MASK_DEFAULT, &mask);
        syslog_info_a("adding virtual inteface: addr=%s, mask=%s, gw=%s", SEL4OSAPI_IP_ADDR_DEFAULT, SEL4OSAPI_IP_MASK_DEFAULT, SEL4OSAPI_IP_GW_DEFAULT);
        error = sel4osapi_netiface_add_vface(iface, &addr, &mask, &gw, TRUE);
        assert(error == 0);
    }

    error = sel4osapi_thread_start(iface->irq_thread);
    assert(error == 0);

    syslog_trace("network initialized.");

    return 0;
}
