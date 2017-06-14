/*
 * FILE: memory.h - memory allocation for sel4osapi
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#ifndef SEL4OSAPI_MEMORY_H_
#define SEL4OSAPI_MEMORY_H_

/*
 * Allocate a buffer of the specified size.
 * Wrapper to malloc().
 */
void*
sel4osapi_heap_allocate(size_t size);

/*
 * Free the specified memory.
 * Wrapper to free().
 */
void
sel4osapi_heap_free(void* ptr);


#endif /* SEL4OSAPI_MEMORY_H_ */
