/*
 * FILE: util.c - utility functions for sel4osapi
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#include <sel4osapi/osapi.h>

#include <vka/capops.h>


/*
 * Allocate untypeds on the supplied VKA, till either a certain
 * number of bytes is allocated or a certain number of untyped objects
 * is allocated.,
 *
 * @param [in]  vka          the VKA on which to allocate the untypeds.
   @param [out] untypeds     array of untyped objects allocated by the operation.
 * @param [in]  bytes        the maximum number of bytes of untyped memory to allocate
 * @param [in]  max_untypeds the maximum number of untypeds objects to allocate
 *
 * @return the number of untyped objects that were allocated.
 *
 */
unsigned int
sel4osapi_util_allocate_untypeds(
        vka_t *vka, vka_object_t *untypeds, size_t bytes, unsigned int max_untypeds)
{
    unsigned int num_untypeds = 0;
    size_t allocated = 0;
    uint8_t size_bits =  23;
    while (num_untypeds < max_untypeds &&
            allocated + BIT(size_bits) <= bytes &&
               vka_alloc_untyped(vka, size_bits, &untypeds[num_untypeds]) == 0) {
            allocated += BIT(size_bits);
            num_untypeds++;
        }
    return num_untypeds;
}

void
sel4osapi_util_copy_cap(
        vka_t *vka, seL4_CPtr src, seL4_CPtr *dest_out)
{
    int error = 0;
    cspacepath_t copy_src, copy_dest;
    /* copy the cap to map into the remote process */
    vka_cspace_make_path(vka, src, &copy_src);
    error = vka_cspace_alloc(vka, dest_out);
    assert(error == 0);
    vka_cspace_make_path(vka, *dest_out, &copy_dest);
    error = vka_cnode_copy(&copy_dest, &copy_src, seL4_AllRights);
    assert(error == 0);
}



