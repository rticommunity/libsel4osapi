/*
 * FILE: list.h - simple linked list implementation
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#ifndef SEL4OSAPI_LIST_H_
#define SEL4OSAPI_LIST_H_

/*
 * Data structure representing
 * a list/node.
 */
typedef struct simple_list_node
{
    void *el;
    struct simple_list_node *next;
    struct simple_list_node *prev;
} simple_list_t;


/*
 * Insert a new node into a list.
 * If the list is empty, add it to head,
 * otherwise, allocate a new node and insert
 * it at the head of the list.
 * Return the new head of the list.
 */
simple_list_t*
simple_list_insert(simple_list_t *head, void *el);

simple_list_t*
simple_list_insert_node(simple_list_t *head, simple_list_t *node);

/*
 * Remove a node from the list by unlinking it.
 * If the node wasn't the list's head, then
 * free its memory.
 */
simple_list_t*
simple_list_unlink(simple_list_t * head, simple_list_t *node);

int
simple_list_to_array(simple_list_t *list, void **array, int array_len_max, int *array_len_out);

void
simple_list_print(simple_list_t *list, char *prefix);

#endif /* SEL4OSAPI_LIST_H_ */
