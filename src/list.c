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

simple_list_t*
simple_list_insert_node(simple_list_t *head, simple_list_t *node)
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
    simple_list_t *cursor = NULL, *prev = NULL;

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

simple_list_t*
simple_list_insert(simple_list_t *head, void *el)
{
    simple_list_t *new_node = (simple_list_t*) malloc(sizeof(simple_list_t));
    assert(new_node != NULL);
    new_node->el = el;

    return simple_list_insert_node(head, new_node);
}

simple_list_t*
simple_list_unlink(simple_list_t * head, simple_list_t *node)
{
    simple_list_t *prev, *next;

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
simple_list_to_array(simple_list_t *list, void **array, int array_len_max, int *array_len_out)
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
simple_list_print(simple_list_t *list, char *prefix)
{
    simple_list_t *cursor = NULL;
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
simple_list_tester_print(simple_list_t *entries, simple_list_t *free_entries)
{
    simple_list_t *cursor = NULL;

#define print_entry_val(cursor_, val_) \
    syslog_info_a("[%x][%d]\t[p=%x][n=%x]",(unsigned int) (cursor_), val_, (unsigned int) (cursor_)->prev, (unsigned int) (cursor_)->next);

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
simple_list_tester(sel4osapi_thread_info_t *thread)
{
    int i = 0;
    int op = 0;
    simple_list_t *free_entries = NULL;
    simple_list_t *entries = NULL;
    int pool_size = 5;

    for (i = 0; i < pool_size; ++i) {
        int *n = (int*) sel4osapi_heap_allocate(sizeof(int));
        *n = 0;
        free_entries = simple_list_insert(free_entries, n);
        syslog_info_a("allocated new entry [%x]", (unsigned int) free_entries);
    }

    i = 0;

    simple_list_tester_print(entries, free_entries);

    while (thread->active && i < 100)
    {
        op = rand() % 2;

        syslog_info_a("performing op=%d",op);

        switch (op) {
            case 0:
            {
                /* schedule an entry */
                simple_list_t *entry = free_entries;
                if (entry == NULL)
                {
                    syslog_info("no more free entries");
                    continue;
                }
                int *n = (int*) entry->el;
                assert(*n == 0);
                free_entries = simple_list_unlink(free_entries, entry);

                *n = i % pool_size + 1;

                entries = simple_list_insert_node(entries, entry);

                syslog_info_a("added entry [%x][%d]", (unsigned int) entry, *n);

                break;
            }
            case 1:
            {
                /* return an entry */
                simple_list_t *cursor = NULL, *entry = NULL;

                if (entries == NULL)
                {
                    continue;
                }

                while (entry == NULL)
                {
                    int val = rand() % pool_size + 1;
                    cursor = entries;

                    syslog_info_a("looking for n=%d",val);

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
                entries = simple_list_unlink(entries, entry);
                *((int*)entry->el) = 0;
                free_entries = simple_list_insert_node(free_entries, entry);
                syslog_info_a("freed entry [%x][%d]", (unsigned int) entry, el_n);

                break;
            }
            case 2:
            {

                break;
            }
        }

        simple_list_tester_print(entries, free_entries);
        i++;

    }

}
