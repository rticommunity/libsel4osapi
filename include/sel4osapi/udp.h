/*
 * FILE: upd.h - UDP stack for sel4osapi
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#ifndef SEL4OSAPI_UDP_H_
#define SEL4OSAPI_UDP_H_

#include <lwip/lwipopts.h>

#define SEL4OSAPI_UDP_PORT_BASE             8000

#define SEL4OSAPI_UDP_MAX_MSGS_PER_CLIENT   5

#define SEL4OSAPI_UDP_MAX_SOCKETS           MEMP_NUM_UDP_PCB
#define SEL4OSAPI_UDP_SOCKET_FIRST_PORT     50000

typedef struct udp_message {
    struct pbuf *pbuf;
    ip_addr_t addr;
    uint16_t port;
} sel4osapi_udp_message_t;


typedef struct sel4osapi_udp_socket
{
    int id;

    ip_addr_t addr;
    uint16_t port;

    seL4_CPtr ep_tx_ready;
    seL4_CPtr ep_rx_ready;
    seL4_CPtr aep_rx_data;

} sel4osapi_udp_socket_t;

typedef struct sel4osapi_udp_socket_server
{
    sel4osapi_netiface_t *iface;

    sel4osapi_udp_socket_t socket;

    struct udp_pcb *udp_pcb;
    sel4osapi_ipcclient_t *client;

    sel4osapi_thread_t *tx_thread;
    sel4osapi_thread_t *rx_thread;

    sel4osapi_mutex_t *msgs_mutex;
    simple_pool_t *msgs;

} sel4osapi_udp_socket_server_t;

typedef enum sel4osapi_udpstack_opcode
{
    UDPSTACK_CREATE_SOCKET = 300,
    UDPSTACK_BIND_SOCKET = 301,
    UDPSTACK_CONNECT_SOCKET = 302
} sel4osapi_udpstack_opcode_t;

typedef struct sel4osapi_udpstack
{
    sel4osapi_mutex_t *mutex;
    seL4_CPtr stack_op_ep;
    simple_pool_t *socket_servers;
    sel4osapi_thread_t *server_thread;
} sel4osapi_udpstack_t;


typedef struct sel4osapi_udp_interface
{
    sel4osapi_mutex_t *mutex;
    simple_pool_t *sockets;
    seL4_CPtr stack_op_ep;
} sel4osapi_udp_interface_t;

int
sel4osapi_udp_initialize(sel4osapi_udpstack_t *udp, int priority);

sel4osapi_udp_socket_t*
sel4osapi_udp_create_socket(ip_addr_t *addr);

int
sel4osapi_udp_bind(sel4osapi_udp_socket_t *sd, uint16_t port);

int
sel4osapi_udp_send(sel4osapi_udp_socket_t *sd, void *msg, size_t len, ip_addr_t *ipaddr, uint16_t port);

int
sel4osapi_udp_recv(sel4osapi_udp_socket_t *sd, void *msg, unsigned int max_len, unsigned int *len_out, ip_addr_t *ipaddr_out, uint16_t *port_out);

int
sel4osapi_udp_send_sd(int sd, void *msg, size_t len, ip_addr_t *ipaddr, uint16_t port);

int
sel4osapi_udp_recv_sd(int sd, void *msg, unsigned int max_len, unsigned int *len_out, ip_addr_t *ipaddr_out, uint16_t *port_out);


#endif /* SEL4OSAPI_UDP_H_ */
