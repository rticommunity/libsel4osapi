/*
 * FILE: list.c - simple linked list implementation
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#include <sel4osapi/osapi.h>

sel4osapi_list_t*
sel4osapi_list_insert_node(sel4osapi_list_t *head, sel4osapi_list_t *node)
{
#if SIMPLE_LIST_LIFO
    assert(node != NULL);
    node->next = head;
    node->prev = NULL;
    if (head != NULL)
    {
        head->prev = node;
    }
    head = node;
#else
    sel4osapi_list_t *cursor = NULL, *prev = NULL;

    assert(node != NULL);

    cursor = head;
    while (cursor != NULL)
    {
        prev = cursor;
        cursor = cursor->next;
    }

    node->next = NULL;
    if (prev != NULL)
    {
        prev->next = node;
        node->prev = prev;
    }
    else
    {
        head = node;
        node->prev = NULL;
    }
#endif

    return head;
}

sel4osapi_list_t*
sel4osapi_list_insert(sel4osapi_list_t *head, void *el)
{
    sel4osapi_list_t *new_node = (sel4osapi_list_t*) malloc(sizeof(sel4osapi_list_t)+64);
    assert(new_node != NULL);
    new_node->el = el;

    return sel4osapi_list_insert_node(head, new_node);
}

sel4osapi_list_t*
sel4osapi_list_unlink(sel4osapi_list_t * head, sel4osapi_list_t *node)
{
    sel4osapi_list_t *prev, *next;

    assert(node != NULL);
    assert(head != NULL);

    prev = node->prev;
    next = node->next;

    if (prev != NULL)
    {
        prev->next = next;
    }
    if (next != NULL)
    {
        next->prev = prev;
    }

    if (head == node)
    {
        assert(prev == NULL);
        head = next;
    }

    node->prev = NULL;
    node->next = NULL;

    return head;
}

int
sel4osapi_list_to_array(sel4osapi_list_t *list, void **array, int array_len_max, int *array_len_out)
{
    int i = 0;

    assert(list != NULL);
    assert(array != NULL);
    assert(array_len_out != NULL);

    *array_len_out = 0;
    for (i = 0; i < array_len_max && list != NULL; ++i) {
        array[i] = list->el;
        list = list->next;
    }
    *array_len_out = i;
    return 0;
}

void
sel4osapi_list_print(sel4osapi_list_t *list, char *prefix)
{
    sel4osapi_list_t *cursor = NULL;
    int i = 0;

#define print_entry(i_,cursor_) \
    printf("%-20s\t[%03d]\t[%08x]\t[p=%08x]\t[n=%08x]\t[el=%08x]\n",(prefix==NULL)?"":prefix,i_,(unsigned int) (cursor_), (unsigned int) (cursor_)->prev, (unsigned int) (cursor_)->next, (unsigned int) cursor->el);

    cursor = list;
    while (cursor != NULL)
    {
        assert(cursor->el != NULL);
        print_entry(i, cursor);
        cursor = cursor->next;
        i++;
    }
}


void
sel4osapi_list_tester_print(sel4osapi_list_t *entries, sel4osapi_list_t *free_entries)
{
    sel4osapi_list_t *cursor = NULL;

#define print_entry_val(cursor_, val_) \
    syslog_info("[%x][%d]\t[p=%x][n=%x]",(unsigned int) (cursor_), val_, (unsigned int) (cursor_)->prev, (unsigned int) (cursor_)->next);

    syslog_info("-----current entries-----");

    cursor = entries;
    while (cursor != NULL)
    {
        assert(cursor->el != NULL);

        int *n = (int*) cursor->el;

        print_entry_val(cursor, *n);

        cursor = cursor->next;
    }

    syslog_info("-------free entries------");
    cursor = free_entries;
    while (cursor != NULL)
    {
        assert(cursor->el != NULL);

        int *n = (int*) cursor->el;

        print_entry_val(cursor, *n);

        cursor = cursor->next;
    }
    syslog_info("-------------------------");
}

void
sel4osapi_list_tester(sel4osapi_thread_info_t *thread)
{
    int i = 0;
    int op = 0;
    sel4osapi_list_t *free_entries = NULL;
    sel4osapi_list_t *entries = NULL;
    int pool_size = 5;

    for (i = 0; i < pool_size; ++i) {
        int *n = (int*) sel4osapi_heap_allocate(sizeof(int));
        *n = 0;
        free_entries = sel4osapi_list_insert(free_entries, n);
        syslog_info("allocated new entry [%x]", (unsigned int) free_entries);
    }

    i = 0;

    sel4osapi_list_tester_print(entries, free_entries);

    while (thread->active && i < 100)
    {
        op = rand() % 2;

        syslog_info("performing op=%d",op);

        switch (op) {
            case 0:
            {
                /* schedule an entry */
                sel4osapi_list_t *entry = free_entries;
                if (entry == NULL)
                {
                    syslog_info("no more free entries");
                    continue;
                }
                int *n = (int*) entry->el;
                assert(*n == 0);
                free_entries = sel4osapi_list_unlink(free_entries, entry);

                *n = i % pool_size + 1;

                entries = sel4osapi_list_insert_node(entries, entry);

                syslog_info("added entry [%x][%d]", (unsigned int) entry, *n);

                break;
            }
            case 1:
            {
                /* return an entry */
                sel4osapi_list_t *cursor = NULL, *entry = NULL;

                if (entries == NULL)
                {
                    continue;
                }

                while (entry == NULL)
                {
                    int val = rand() % pool_size + 1;
                    cursor = entries;

                    syslog_info("looking for n=%d",val);

                    while (cursor != NULL)
                    {
                        assert(cursor->el != NULL);

                        int *n = (int*) cursor->el;

                        if (*n == val)
                        {
                            entry = cursor;
                        }

                        cursor = cursor->next;
                    }
                }
                assert(entry != NULL);
                int el_n = *((int*)entry->el);
                entries = sel4osapi_list_unlink(entries, entry);
                *((int*)entry->el) = 0;
                free_entries = sel4osapi_list_insert_node(free_entries, entry);
                syslog_info("freed entry [%x][%d]", (unsigned int) entry, el_n);

                break;
            }
            case 2:
            {

                break;
            }
        }

        sel4osapi_list_tester_print(entries, free_entries);
        i++;

    }

}
