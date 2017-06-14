/*
 * FILE: memory.c - memory allocation for sel4osapi
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#include <sel4osapi/osapi.h>

void*
sel4osapi_heap_allocate(size_t size)
{
    return malloc(size);
}

void
sel4osapi_heap_free(void* ptr)
{
    free(ptr);
}
