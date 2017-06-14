/*
 * FILE: util.h - utility functions for sel4osapi
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#ifndef SEL4OSAPI_UTIL_H_
#define SEL4OSAPI_UTIL_H_

#define sel4osapi_printf    printf

/*
 * Allocate untyped objects from the specified Virtual Kernel Allocator,
 * until either the specified amount of allocated memory or the number of
 * allocated objects are reached.
 */
unsigned int
sel4osapi_util_allocate_untypeds(
        vka_t *vka, vka_object_t *untypeds, size_t bytes, unsigned int max_untypeds);

/*
 * Create a copy of a cap to a free slot within the same CSpace.
 */
void
sel4osapi_util_copy_cap(
        vka_t *vka, seL4_CPtr src, seL4_CPtr *dest_out);

#endif
