/*
 * FILE: osapi.h - an OS service layer for seL4
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */


#ifndef SEL4OSAPI_H_
#define SEL4OSAPI_H_

#include <stdint.h>
#include <sel4/sel4.h>
#include <simple/simple.h>
#include <vka/object.h>
#include <allocman/allocman.h>
#include <vspace/vspace.h>
#include <sel4utils/vspace.h>
#include <sel4utils/process.h>
#include <sel4platsupport/timer.h>
#include <platsupport/chardev.h>

#ifdef CONFIG_LIB_OSAPI_NET
#include <lwip/netif.h>
#include <lwip/udp.h>
#endif

#include "sel4osapi/list.h"
#include "sel4osapi/pool.h"

#include "sel4osapi/config.h"
#include "sel4osapi/memory.h"
#include "sel4osapi/thread.h"
#include "sel4osapi/mutex.h"
#include "sel4osapi/semaphore.h"

#include "sel4osapi/ipc.h"

#include "sel4osapi/io.h"

#ifdef CONFIG_LIB_OSAPI_NET
#include "sel4osapi/network.h"
#include "sel4osapi/udp.h"
#endif

#include "sel4osapi/process.h"

#ifdef CONFIG_LIB_OSAPI_SYSCLOCK
#include "sel4osapi/clock.h"
#endif

#include "sel4osapi/log.h"
#include "sel4osapi/util.h"
#include "sel4osapi/system.h"

#endif /* SEL4OSAPI_H_ */
