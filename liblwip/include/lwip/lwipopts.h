/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Simon Goldschmidt
 *
 */
#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

/* Prevent having to link sys_arch.c (we don't test the API layers in unit tests) */
#define NO_SYS                          1
#define NO_SYS_NO_TIMERS                1
#define LWIP_NETCONN                    0
#define LWIP_SOCKET                     0
#define LWIP_TCP                        0
#define LWIP_IGMP                       1
#define LWIP_RAND                       rand
#define LWIP_STATS                      1
#define LWIP_STATS_DISPLAY              1
#define LINK_STATS                      0
#define TCP_STATS                       0
#define ICMP_STATS                      0
#define IGMP_STATS                      0
#define IP_FRAG                         1
#define IP_REASSEMBLY                   1

#define MEM_ALIGNMENT                   4
#define MEM_LIBC_MALLOC                 1
#define MEM_SIZE                        16000
/*#define PBUF_POOL_SIZE                  100*/
#define MEMP_NUM_PBUF                   15
#define MEMP_NUM_UDP_PCB                15
#define MEMP_NUM_TCP_PCB                0
#define MEMP_NUM_TCP_PCB_LISTEN         0
#define MEMP_NUM_TCP_SEG                0
#define MEMP_NUM_REASSDATA              32
#define MEMP_NUM_ARP_QUEUE              10
#define PBUF_POOL_SIZE                  20
#define LWIP_ARP                        1
#define IP_REASS_MAX_PBUFS              64
#define IP_FRAG_USES_STATIC_BUF         0
#define IP_DEFAULT_TTL                  255
#define IP_SOF_BROADCAST                1
#define IP_SOF_BROADCAST_RECV           1
#define LWIP_ICMP                       1
#define LWIP_BROADCAST_PING             1
#define LWIP_MULTICAST_PING             1
#define LWIP_RAW                        1
#define LWIP_NETIF_STATUS_CALLBACK      1
#define LWIP_NETIF_LINK_CALLBACK        1
#define LWIP_NETIF_HWADDRHINT           1
#define LWIP_NETCONN                    0
#define LWIP_SOCKET                     0
#define LWIP_STATS_DISPLAY              1
#define MEM_STATS                       0
#define SYS_STATS                       0
#define MEMP_STATS                      0
#define LINK_STATS                      0
#define ETHARP_TRUST_IP_MAC             0
#define ETH_PAD_SIZE                    0
#define LWIP_CHKSUM_ALGORITHM           2

#define LWIP_NETIF_LOOPBACK             0
#define LWIP_LOOPBACK_MAX_PBUFS         0
#define LWIP_HAVE_LOOPIF                0

#define ETHARP_SUPPORT_STATIC_ENTRIES   1

#define LWIP_DEBUG                      0

/*#define LWIP_DBG_TYPES_ON               LWIP_DBG_TRACE | LWIP_DBG_STATE | LWIP_DBG_FRESH | LWIP_DBG_HALT*/

#define ETHARP_DEBUG                    LWIP_DBG_OFF
#define NETIF_DEBUG                     LWIP_DBG_OFF
#define PBUF_DEBUG                      LWIP_DBG_OFF
#define API_LIB_DEBUG                   LWIP_DBG_OFF
#define API_MSG_DEBUG                   LWIP_DBG_OFF
#define SOCKETS_DEBUG                   LWIP_DBG_OFF
#define ICMP_DEBUG                      LWIP_DBG_OFF
#define IGMP_DEBUG                      LWIP_DBG_OFF
#define INET_DEBUG                      LWIP_DBG_OFF
#define IP_DEBUG                        LWIP_DBG_OFF
#define IP_REASS_DEBUG                  LWIP_DBG_OFF
#define RAW_DEBUG                       LWIP_DBG_OFF
#define MEM_DEBUG                       LWIP_DBG_OFF
#define MEMP_DEBUG                      LWIP_DBG_OFF
#define SYS_DEBUG                       LWIP_DBG_OFF
#define TIMERS_DEBUG                    LWIP_DBG_OFF
#define UDP_DEBUG                       LWIP_DBG_OFF

#endif /* __LWIPOPTS_H__ */
