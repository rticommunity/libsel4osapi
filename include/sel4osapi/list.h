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

/* TODO: 
 *      sel4utils (https://github.com/seL4/util_libs) already have an
 *      implementation of a linked list.
 *      We should remove this list from OSAPI and use the other one
 *      instead.
 */


#ifndef SEL4OSAPI_LINKED_LIST_H_
#define SEL4OSAPI_LINKED_LIST_H_

/* Defines the behavior of the list:
 *  LIFO - elements are inserted at the beginning
 *  FIFO - elements are inserted at the end (list is traversed to reach the end)
 *
 * Default (if commneted): FIFO
 */
// #define SIMPLE_LIST_LIFO

/*
 * Data structure representing
 * a list/node.
 */
typedef struct sel4osapi_list_node
{
    void *el;
    struct sel4osapi_list_node *next;
    struct sel4osapi_list_node *prev;
} sel4osapi_list_t;


/** \brief Insert a new node into a list.
 *
 * If the list is empty, add it to head, otherwise, allocate a new node and 
 * insert it at the head (or end, depending if SIMPLE_LIST_LIFO is defined - 
 * See above) of the list.
 *
 * \param head      pointer to the first element of the list
 * \param el        pointer to the element to insert into the list
 * \return          the (potentially) new head of the list.
 */
sel4osapi_list_t*
sel4osapi_list_insert(sel4osapi_list_t *head, void *el);


/** \brief inserts a node in the list
 * 
 * Node is inserted to the end or to the beginning of the list depending
 * on how the list is defined (see SIMPLE_LIST_LIFO macro)
 *
 * \param head      the head of the list
 * \param node      the node to insert in the list
 *
 * \return          the (potentially) new head of the list
 */
sel4osapi_list_t*
sel4osapi_list_insert_node(sel4osapi_list_t *head, sel4osapi_list_t *node);

/** \brief Remove a node from the list by unlinking it.
 *
 * The actual node is not deleted. It is responsibility of the caller
 * to free the memory of the node.
 *
 * \param head      the head of the list
 * \param node      the node to remove
 * \return          the (potentially) new head of the ist
 */
sel4osapi_list_t*
sel4osapi_list_unlink(sel4osapi_list_t * head, sel4osapi_list_t *node);


/** \brief Converts a simple list to an array of nodes
 *
 * Caller must pre-allocate an array of pointers to hold all the elements
 * of the list. This function simply copies the pointers to the contained
 * elements (for each node) in the array at the appropriate position.
 *
 * \param head      the head of the list
 * \param array     a pointer to an area of memory that will be allocated to hold the element of the array
 * \param array_len_max size of the array
 * \param array_len_out number of pointers copied
 * \return          this function always return 0
 */
int
sel4osapi_list_to_array(sel4osapi_list_t *head, void **array, int array_len_max, int *array_len_out);

void
sel4osapi_list_print(sel4osapi_list_t *list, char *prefix);

#endif /* SEL4OSAPI_LINKED_LIST_H_ */
