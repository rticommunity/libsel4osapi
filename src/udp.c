/*
 * FILE: udp.c - UDP stack for sel4osapi
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#include <sel4osapi/osapi.h>

#include <lwip/init.h>
#include <netif/etharp.h>

#include <string.h>

#include <vka/capops.h>

#include <lwip/stats.h>


static void
udprecv(void *arg, struct udp_pcb *pcb, struct pbuf *p, ip_addr_t *addr, uint16_t port)
{
    seL4_MessageInfo_t msg = seL4_MessageInfo_new(0, 0, 0, 1);
    sel4osapi_udp_socket_server_t *server = (sel4osapi_udp_socket_server_t*)arg;
    sel4osapi_udp_message_t *m = NULL;

    int error = sel4osapi_mutex_lock(server->msgs_mutex);
    assert(!error);

    syslog_trace("new message on sd=%d", server->socket.id);

    m = simple_pool_alloc(server->msgs);

    if (m == NULL) {
        syslog_warn("discarding msg on sd=%d", server->socket.id);
        pbuf_free(p);
        goto notify;
    }

    m->pbuf = p;
    m->addr = *addr;
    m->port = port;

notify:
    sel4osapi_mutex_unlock(server->msgs_mutex);
    seL4_SetMR(0, 1);
    seL4_Send(server->socket.aep_rx_data, msg);
}
static void
sel4osapi_udp_socket_tx_thread(sel4osapi_thread_info_t *thread)
{
    sel4osapi_udp_socket_server_t *server = (sel4osapi_udp_socket_server_t*) thread->arg;

    while (thread->active)
    {
            int error = 1;
            int len = 0;
            seL4_MessageInfo_t minfo;
            seL4_Word sender_badge;
            ip_addr_t addr;
            uint16_t port;
            struct pbuf *p;
            err_t lwerr = ERR_MEM;

            syslog_trace("waiting for data...");

            minfo = seL4_Recv(server->socket.ep_tx_ready, &sender_badge);
            assert(seL4_MessageInfo_get_length(minfo) == 3);
            len = sel4osapi_getMR(0);
            addr.addr = sel4osapi_getMR(1);
            port = sel4osapi_getMR(2);

            if (len > server->client->tx_buf_size)
            {
                syslog_warn("truncating msg from %d to %d", len, server->client->tx_buf_size);
                len = server->client->tx_buf_size;
            }


            error = sel4osapi_mutex_lock(server->iface->mutex);
            assert(!error);
            p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
            if (p) {
                syslog_trace("transmitting msg [len=%d, addr=%s, port=%d]", len, ipaddr_ntoa(&addr), port);
                memcpy(p->payload, server->client->tx_buf, len);
                lwerr = udp_sendto(server->udp_pcb, p, &addr,port);
                syslog_trace("udp_sendto=%d",lwerr);
                pbuf_free(p);
            }
            else
            {
                syslog_error("cannot allocate pbuf");
            }
            sel4osapi_mutex_unlock(server->iface->mutex);

            error = (lwerr != ERR_OK)?lwerr:0;

            minfo = seL4_MessageInfo_new(0,0,0,1);
            sel4osapi_setMR(0,error);
            seL4_Reply(minfo);
    }
}

static void
sel4osapi_udp_socket_rx_thread(sel4osapi_thread_info_t *thread)
{
    seL4_MessageInfo_t minfo;
    sel4osapi_udp_socket_server_t *server = (sel4osapi_udp_socket_server_t*) thread->arg;
    unsigned int packet_len = 0;
    sel4osapi_udp_message_t *msg = NULL;
    uint16_t port;
    ip_addr_t ipaddr;
    int error;
    seL4_Word sender_badge;
    int remaining_msgs = 0;
    uint32_t total_msgs = 0;

    while (thread->active)
    {
        packet_len = 0;
        port = 0;
        ipaddr.addr = 0;

        if (remaining_msgs > 0)
        {
            minfo = seL4_MessageInfo_new(0, 0, 0, 1);
            seL4_SetMR(0, 0);
            seL4_Send(server->socket.aep_rx_data, minfo);
        }

        /* wait for client to be ready to receive */
        minfo = seL4_Recv(server->socket.ep_rx_ready, &sender_badge);

        error = sel4osapi_mutex_lock(server->msgs_mutex);
        assert(!error);
        if (server->msgs->entries == NULL)
        {
            syslog_warn("awaken without messages.");
            sel4osapi_mutex_unlock(server->msgs_mutex);
            goto reply;
        }
        msg = (sel4osapi_udp_message_t*) server->msgs->entries->el;
        remaining_msgs = simple_pool_get_current_size(server->msgs);
        assert(msg);
        assert(msg->pbuf);
        sel4osapi_mutex_unlock(server->msgs_mutex);

        total_msgs++;

        port = msg->port;
        ipaddr = msg->addr;
        /* copy packet into rx_buf */
        for (struct pbuf *q = msg->pbuf; q; q = q->next)
        {
            memcpy(server->client->rx_buf + packet_len, q->payload, q->len);
            packet_len += q->len;
        }
        assert(packet_len <= server->client->rx_buf_size);

        syslog_trace("received msg [ip=%s (%d), port=%d, size=%d]",
                            ipaddr_ntoa(&ipaddr), ipaddr.addr, port, packet_len);

        remaining_msgs--;

reply:
        /* notify client to read rx_buf */
        minfo = seL4_MessageInfo_new(0, 0, 0, 3);
        seL4_SetMR(0, packet_len);
        seL4_SetMR(1, port);
        seL4_SetMR(2, ipaddr.addr);
        seL4_Reply(minfo);

        if (msg)
        {
            /* release pbuf */
            error = sel4osapi_mutex_lock(server->iface->mutex);
            assert(!error);
            pbuf_free(msg->pbuf);
#if 0
            if (total_msgs % 10000 == 0)
            {
                stats_display();
            }
#endif
            sel4osapi_mutex_unlock(server->iface->mutex);

            /* return msg to pool */
            error = sel4osapi_mutex_lock(server->msgs_mutex);
            assert(!error);
            error = simple_pool_free(server->msgs, msg);
            assert(error == 0);
            sel4osapi_mutex_unlock(server->msgs_mutex);

            msg = NULL;
        }


    }
}

static void
sel4osapi_udp_stack_thread(sel4osapi_thread_info_t *thread)
{
    sel4osapi_udpstack_t *udp = (sel4osapi_udpstack_t*) thread->arg;
    vka_t *vka = sel4osapi_system_get_vka();
    sel4osapi_netstack_t *net = sel4osapi_system_get_netstack();
    seL4_MessageInfo_t minfo;
    seL4_Word mr0, mr1, mr2, mr3, sender_badge;
    int error = 0;
    int opcode = 0;
    ip_addr_t addr;
    char thread_name[SEL4OSAPI_THREAD_NAME_MAX_LEN];
    sel4osapi_udp_socket_server_t *socket_server = NULL;
    sel4osapi_ipcclient_t *client = NULL;
    sel4osapi_netiface_t *iface = NULL;
    cspacepath_t dest, src;
    int socket_id = 0;
    uint16_t bind_port = 0;
    int client_id = 0;
    seL4_CPtr tx_ready_ep_mint;
    seL4_CPtr rx_ready_ep_mint;
    int args_num;

    syslog_info("started serving requests...");

    vka_cspace_alloc(vka, &rx_ready_ep_mint);
    assert(rx_ready_ep_mint != seL4_CapNull);
    vka_cspace_make_path(vka, rx_ready_ep_mint, &dest);
    seL4_SetCapReceivePath(dest.root, dest.capPtr, dest.capDepth);

    while (thread->active)
    {

        syslog_trace("waiting for next request...");
        minfo = seL4_Recv(udp->stack_op_ep, &sender_badge);
        mr0 = seL4_GetMR(0);
        mr1 = seL4_GetMR(1);
        mr2 = seL4_GetMR(2);
        mr3 = seL4_GetMR(3);
        args_num = seL4_MessageInfo_get_length(minfo);
        assert(args_num >= 1);

        opcode = mr0;

        syslog_trace("new request: opcode=%d, args_num=%d", opcode, args_num);

        switch (opcode) {
            case UDPSTACK_CREATE_SOCKET:
            {
                assert(args_num == 3);

                client_id = mr1;
                addr.addr = mr2;

                syslog_trace("create socket request: client=%d, add=%s", client_id, ipaddr_ntoa(&addr));

                {
                    simple_list_t *cursor;
                    sel4osapi_ipcclient_t *ncursor;

                    cursor = net->ipc->clients->entries;
                    while (cursor != NULL)
                    {
                        ncursor = (sel4osapi_ipcclient_t*) cursor->el;
                        if (ncursor->id == client_id)
                        {
                            client = ncursor;
                            break;
                        }
                        cursor = cursor->next;
                    }
                    assert(client != NULL);
                }

                {
                    simple_list_t *cursor, *cursor2;
                    sel4osapi_netiface_t *ncursor;
                    sel4osapi_netvface_t *vcursor;

                    cursor = net->ifaces->entries;
                    while (cursor != NULL)
                    {
                        ncursor = (sel4osapi_netiface_t*) cursor->el;
                        cursor2 = ncursor->vfaces->entries;
                        while (cursor2 != NULL)
                        {
                            vcursor = (sel4osapi_netvface_t*) cursor2->el;
                            if (addr.addr == 0 || vcursor->ip_addr.addr == addr.addr)
                            {
                                iface = ncursor;
                                break;
                            }
                            cursor2 = cursor2->next;
                        }
                        cursor = cursor->next;
                    }
                    assert(iface != NULL);
                }

                socket_server = simple_pool_alloc(udp->socket_servers);
                assert(socket_server != NULL);

                socket_server->iface = iface;
                socket_server->client = client;
                socket_server->udp_pcb = udp_new();
                assert(socket_server->udp_pcb);
                socket_server->socket.id = simple_pool_get_current_size(udp->socket_servers);
                socket_server->socket.addr = addr;

                socket_server->socket.port = 0;
                socket_server->socket.aep_rx_data = 0;
                socket_server->socket.ep_rx_ready= 0;

                socket_server->socket.ep_tx_ready = vka_alloc_endpoint_leaky(vka);
                assert(socket_server->socket.ep_tx_ready != seL4_CapNull);

                snprintf(thread_name, SEL4OSAPI_THREAD_NAME_MAX_LEN, "udp-%d-tx", socket_server->socket.id);
                socket_server->tx_thread = sel4osapi_thread_create(thread_name, sel4osapi_udp_socket_tx_thread, socket_server, thread->priority);
                assert(socket_server->tx_thread);

                socket_server->rx_thread = NULL;
                socket_server->msgs_mutex = NULL;
                socket_server->msgs = NULL;

                vka_cspace_alloc(vka, &tx_ready_ep_mint);
                assert(tx_ready_ep_mint != seL4_CapNull);
                vka_cspace_make_path(vka, tx_ready_ep_mint, &dest);
                vka_cspace_make_path(vka, socket_server->socket.ep_tx_ready, &src);
                error = vka_cnode_copy(&dest, &src, seL4_AllRights);
                assert(error == 0);

                mr0 = error;
                mr1 = socket_server->socket.id;

                seL4_SetCap(0, tx_ready_ep_mint);
                seL4_SetMR(0, mr0);
                seL4_SetMR(1, mr1);
                minfo = seL4_MessageInfo_new(0,0,1,2);
                seL4_Reply(minfo);

                error = sel4osapi_thread_start(socket_server->tx_thread);
                assert(error == 0);

                syslog_trace("new socket created: sd=%d, addr=%s, client=%d",
                        socket_server->socket.id, ipaddr_ntoa(&socket_server->socket.addr),
                        socket_server->client->id)

                break;
            }
            case UDPSTACK_BIND_SOCKET:
            {
                assert(args_num == 3);
                assert(seL4_MessageInfo_get_extraCaps(minfo) == 1);

                socket_id = mr1;
                bind_port = mr2;

                syslog_trace("bind socket request: socket=%d, port=%d", socket_id, bind_port);

                {
                    simple_list_t *cursor;
                    sel4osapi_udp_socket_server_t *scursor;

                    cursor = udp->socket_servers->entries;
                    while (cursor != NULL)
                    {
                        scursor = (sel4osapi_udp_socket_server_t*) cursor->el;
                        if (scursor->socket.id == socket_id)
                        {
                            socket_server = scursor;
                            break;
                        }
                        cursor = cursor->next;
                    }
                    assert(socket_server != NULL);
                }

                assert(socket_server->socket.port == 0);

                socket_server->msgs = simple_pool_new(SEL4OSAPI_UDP_MAX_MSGS_PER_CLIENT, sizeof(sel4osapi_udp_message_t), NULL, NULL, NULL);
                assert(socket_server->msgs);

                socket_server->msgs_mutex = sel4osapi_mutex_create();
                assert(socket_server->msgs_mutex);

                snprintf(thread_name, SEL4OSAPI_THREAD_NAME_MAX_LEN, "udp-%d-rx", socket_server->socket.id);
                socket_server->rx_thread = sel4osapi_thread_create(thread_name, sel4osapi_udp_socket_rx_thread, socket_server, thread->priority);
                assert(socket_server->rx_thread);

                socket_server->socket.port = bind_port;

                socket_server->socket.aep_rx_data = rx_ready_ep_mint;
                assert(socket_server->socket.aep_rx_data != seL4_CapNull);

                socket_server->socket.ep_rx_ready = vka_alloc_endpoint_leaky(vka);
                assert(socket_server->socket.ep_rx_ready != seL4_CapNull);

                vka_cspace_alloc(vka, &rx_ready_ep_mint);
                assert(rx_ready_ep_mint != seL4_CapNull);
                vka_cspace_make_path(vka, rx_ready_ep_mint, &dest);
                vka_cspace_make_path(vka, socket_server->socket.ep_rx_ready, &src);
                error = vka_cnode_copy(&dest, &src, seL4_AllRights);
                assert(error == 0);

                error = sel4osapi_mutex_lock(socket_server->iface->mutex);
                assert(!error);

                udp_recv(socket_server->udp_pcb, udprecv, socket_server);
                error = udp_bind(socket_server->udp_pcb, &socket_server->socket.addr, socket_server->socket.port);
                assert(error == ERR_OK);

                sel4osapi_mutex_unlock(socket_server->iface->mutex);

                mr0 = error;
                seL4_SetMR(0, mr0);
                seL4_SetCap(0, rx_ready_ep_mint);
                minfo = seL4_MessageInfo_new(0,0,1,1);
                seL4_Reply(minfo);

                error = sel4osapi_thread_start(socket_server->rx_thread);
                assert(error == 0);

                syslog_trace("socket bound: sd=%d, addr=%s, port=%d, client=%d",
                        socket_server->socket.id, ipaddr_ntoa(&socket_server->socket.addr),
                        socket_server->socket.port, socket_server->client->id)

                vka_cspace_alloc(vka, &rx_ready_ep_mint);
                assert(rx_ready_ep_mint != seL4_CapNull);
                vka_cspace_make_path(vka, rx_ready_ep_mint, &dest);
                seL4_SetCapReceivePath(dest.root, dest.capPtr, dest.capDepth);

                break;
            }
            case UDPSTACK_CONNECT_SOCKET:
            {
                int socket_id = 0;
                ip_addr_t connect_addr = { 0 };
                uint16_t connect_port = 0;

                socket_id = mr1;
                connect_addr.addr = mr2;
                connect_port = mr3;

                syslog_trace("socket %d connected to %s:%d", socket_id, ipaddr_ntoa(&connect_addr), connect_port);

                mr0 = error;

                seL4_SetMR(0, mr0);
                minfo = seL4_MessageInfo_new(0,0,0,1);
                seL4_Reply(minfo);

                break;
            }
        }
    }

}


int
sel4osapi_udp_initialize(sel4osapi_udpstack_t *udp, int priority)
{
    vka_object_t ep_obj = { 0 };
    int error = 0;
    vka_t *vka = sel4osapi_system_get_vka();

    udp->mutex = sel4osapi_mutex_create();
    assert(udp->mutex);

    udp->socket_servers = simple_pool_new(SEL4OSAPI_UDP_MAX_SOCKETS, sizeof(sel4osapi_udp_socket_server_t), NULL, NULL, NULL);
    assert(udp->socket_servers);

    error = vka_alloc_endpoint(vka, &ep_obj);
    assert(error == 0);
    udp->stack_op_ep = ep_obj.cptr;

    udp->server_thread = sel4osapi_thread_create("udp::stack", sel4osapi_udp_stack_thread, udp, priority);
    assert(udp->server_thread);

    error = sel4osapi_thread_start(udp->server_thread);
    assert(error == 0);

    return 0;
}


sel4osapi_udp_socket_t*
sel4osapi_udp_create_socket(ip_addr_t *addr)
{
    seL4_Word mr0, mr1, mr2;
    seL4_MessageInfo_t minfo;
    vka_t *vka = sel4osapi_system_get_vka();
    sel4osapi_process_env_t *process = sel4osapi_process_get_current();
    sel4osapi_ipcclient_t *client = &process->ipcclient;
    sel4osapi_udp_interface_t *udp_iface = &process->udp_iface;
    sel4osapi_udp_socket_t *socket = NULL;
    int error = 0;
    cspacepath_t path;

    assert(addr);

    error = sel4osapi_mutex_lock(udp_iface->mutex);
    assert(!error);
    socket = simple_pool_alloc(udp_iface->sockets);
    assert(socket);

    socket->addr = *addr;
    socket->port = 0;
    socket->ep_rx_ready = seL4_CapNull;
    socket->aep_rx_data = seL4_CapNull;

    vka_cspace_alloc(vka, &socket->ep_tx_ready);
    assert(socket->ep_tx_ready != seL4_CapNull);
    vka_cspace_make_path(vka, socket->ep_tx_ready, &path);
    seL4_SetCapReceivePath(path.root, path.capPtr, path.capDepth);

    mr0 = UDPSTACK_CREATE_SOCKET;
    mr1 = client->id;
    mr2 = addr->addr;
    seL4_SetMR(0, mr0);
    seL4_SetMR(1, mr1);
    seL4_SetMR(2, mr2);
    minfo = seL4_MessageInfo_new(0,0,0,3);
    minfo = seL4_Call(udp_iface->stack_op_ep, minfo);
    assert(seL4_MessageInfo_get_length(minfo) == 2);
    assert(seL4_MessageInfo_get_extraCaps(minfo) == 1);
    mr0 = seL4_GetMR(0);
    mr1 = seL4_GetMR(1);
    error = mr0;
    socket->id = mr1;
    assert(error == 0);

    /* reset receive cap path */
    seL4_SetCapReceivePath(seL4_CapNull,seL4_CapNull,seL4_CapNull);
    sel4osapi_mutex_unlock(udp_iface->mutex);

    syslog_trace("new socket created: sd=%d, addr=%s, client=%d", socket->id, ipaddr_ntoa(&socket->addr), client->id);

    return socket;
}

int
sel4osapi_udp_bind(sel4osapi_udp_socket_t *socket, uint16_t port)
{
    vka_t *vka = sel4osapi_system_get_vka();
    sel4osapi_process_env_t *process = sel4osapi_process_get_current();
    sel4osapi_udp_interface_t *udp_iface = &process->udp_iface;
    seL4_Word mr0, mr1, mr2;
    seL4_MessageInfo_t minfo;
    cspacepath_t dest, src;
    int error = 0;
    seL4_CPtr rx_read_ep_copy;

    assert(port > 0);

    assert(socket);
    assert(socket->port == 0);

    socket->aep_rx_data = vka_alloc_notification_leaky(vka);
    assert(socket->aep_rx_data != seL4_CapNull);
    vka_cspace_alloc(vka, &rx_read_ep_copy);
    assert(rx_read_ep_copy != seL4_CapNull);
    vka_cspace_make_path(vka, socket->aep_rx_data, &src);
    vka_cspace_make_path(vka, rx_read_ep_copy, &dest);
    error = vka_cnode_copy(&dest, &src, seL4_AllRights);
    assert(error == 0);

    vka_cspace_alloc(vka, &socket->ep_rx_ready);
    assert(socket->ep_rx_ready != seL4_CapNull);
    vka_cspace_make_path(vka, socket->ep_rx_ready, &dest);
    seL4_SetCapReceivePath(dest.root, dest.capPtr, dest.capDepth);

    mr0 = UDPSTACK_BIND_SOCKET;
    mr1 = socket->id;
    mr2 = port;

    seL4_SetMR(0, mr0);
    seL4_SetMR(1, mr1);
    seL4_SetMR(2, mr2);
    seL4_SetCap(0, rx_read_ep_copy);
    minfo = seL4_MessageInfo_new(0,0,1,3);
    minfo = seL4_Call(udp_iface->stack_op_ep, minfo);
    assert(seL4_MessageInfo_get_length(minfo) == 1);
    assert(seL4_MessageInfo_get_extraCaps(minfo) == 1);
    error = seL4_GetMR(0);
    assert(error == 0);

    socket->port = port;

    /* reset receive cap path */
    seL4_SetCapReceivePath(seL4_CapNull,seL4_CapNull,seL4_CapNull);

    return 0;
}


int
sel4osapi_udp_send(sel4osapi_udp_socket_t *socket, void *msg, size_t len, ip_addr_t *ipaddr, uint16_t port)
{
    int error = 0;
    sel4osapi_process_env_t *process = sel4osapi_process_get_current();
    sel4osapi_ipcclient_t *client = &process->ipcclient;
    seL4_MessageInfo_t minfo;

    assert(msg != NULL);
    assert(len < client->tx_buf_size);
    assert(ipaddr != NULL);

    assert(socket);

    sel4osapi_semaphore_take(client->tx_buf_avail, 0);

    memcpy(client->tx_buf, msg, len);

    minfo = seL4_MessageInfo_new(0,0,0,3);
    sel4osapi_setMR(0, len);
    sel4osapi_setMR(1, ipaddr->addr);
    sel4osapi_setMR(2, port);
    minfo = seL4_Call(socket->ep_tx_ready, minfo);
    assert(seL4_MessageInfo_get_length(minfo) == 1);
    error = sel4osapi_getMR(0);

    sel4osapi_semaphore_give(client->tx_buf_avail);

    return error;
}

int
sel4osapi_udp_recv(sel4osapi_udp_socket_t *socket, void *msg, unsigned int max_len, unsigned int *len_out, ip_addr_t *ipaddr_out, uint16_t *port_out)
{
    sel4osapi_process_env_t *process = sel4osapi_process_get_current();
    sel4osapi_ipcclient_t *client = &process->ipcclient;
    seL4_Word sender_badge;
    seL4_MessageInfo_t minfo;
    seL4_Word mr0, mr1, mr2;

    uint16_t port = 0;
    ip_addr_t addr;
    unsigned int len;

    int error = 0;

    assert(msg != NULL);
    assert(max_len > 0);
    assert(len_out != NULL);
    assert(ipaddr_out != NULL);
    assert(port_out != NULL);

    assert(socket);
    assert(socket->port > 0);

    *len_out = 0;
    ipaddr_out->addr = 0;
    *port_out = 0;


    minfo = seL4_Recv(socket->aep_rx_data, &sender_badge);
    /* a message was received on the socket,
     * take lock on rx_buf */
    sel4osapi_semaphore_take(client->rx_buf_avail, 0);

    /* notify rx server to copy msg to rx_buf */
    minfo = seL4_MessageInfo_new(0,0,0,0);
    minfo = seL4_Call(socket->ep_rx_ready, minfo);
    assert(seL4_MessageInfo_get_length(minfo) == 3);
    mr0 = seL4_GetMR(0);
    mr1 = seL4_GetMR(1);
    mr2 = seL4_GetMR(2);

    len = mr0;
    port = mr1;
    addr.addr = mr2;

    syslog_trace("receiving UDP message [ip=%s, port=%d, size=%d]", ipaddr_ntoa(&addr), port, len);

    if (len > max_len)
    {
        syslog_error("supplied buffer too small for UDP message [size=%d, msg_len=%d]", max_len, len);
        error = seL4_NotEnoughMemory;
        goto done;
    }
    else if (len == 0)
    {
        syslog_error("no data.");
        error = -1;
        goto done;
    }

    memcpy(msg, client->rx_buf, len);

    error = 0;

done:

    sel4osapi_semaphore_give(client->rx_buf_avail);

    *len_out = len;
    *port_out = port;
    ipaddr_out->addr = addr.addr;


    return error;
}

static sel4osapi_udp_socket_t*
sel4osapi_udp_sd_to_socket(int sd)
{
    sel4osapi_process_env_t *process = sel4osapi_process_get_current();
    sel4osapi_udp_interface_t *udp_iface = &process->udp_iface;
    sel4osapi_udp_socket_t *socket = NULL;
    simple_list_t *cursor = NULL;

    assert(sd > 0);

    sel4osapi_mutex_lock(udp_iface->mutex);
    cursor = udp_iface->sockets->entries;
    while(cursor != NULL)
    {
        socket = (sel4osapi_udp_socket_t*) cursor->el;
        if (socket->id == sd)
        {
            break;
        }
        socket = NULL;
        cursor = cursor->next;
    }
    assert(socket);
    sel4osapi_mutex_unlock(udp_iface->mutex);
    return socket;
}

int
sel4osapi_udp_recv_sd(int sd, void *msg, unsigned int max_len, unsigned int *len_out, ip_addr_t *ipaddr_out, uint16_t *port_out)
{
    sel4osapi_udp_socket_t *socket = sel4osapi_udp_sd_to_socket(sd);
    return sel4osapi_udp_recv(socket, msg, max_len, len_out, ipaddr_out, port_out);
}

int
sel4osapi_udp_send_sd(int sd, void *msg, size_t len, ip_addr_t *ipaddr, uint16_t port)
{
    sel4osapi_udp_socket_t *socket = sel4osapi_udp_sd_to_socket(sd);
    return sel4osapi_udp_send(socket, msg, len, ipaddr, port);
}
