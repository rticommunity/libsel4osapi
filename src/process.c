/*
 * FILE: process.c - process abstraction for sel4osapi
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#include <sel4osapi/osapi.h>

#include <string.h>
#include <vka/capops.h>

int
sel4osapi_process_init_env(sel4osapi_process_t *process,
                            int pid,
                            char *name,
                            uint8_t priority,
                            vka_t *parent_vka,
                            vspace_t *parent_vspace,
                            int user_untypeds_num,
                            uint8_t *user_untypeds_allocation,
                            uint8_t *user_untypeds_size_bits,
                            vka_object_t *user_untypeds,
                            seL4_CPtr sysclock_ep,
                            seL4_CPtr udp_stack_ep)
{
    int error;

    {
        int i = 0;
        uint32_t untyped_size = 0;
        uint32_t untyped_count = 0;

        process->env->pid = pid;
        snprintf(process->env->name, SEL4OSAPI_USER_PROCESS_NAME_MAX_LEN, "%s", name);

        process->env->priority = priority;

        /* set up caps about the process */
        process->env->page_directory = sel4osapi_process_copy_cap_into(process, parent_vka, process->native.pd.cptr, seL4_AllRights);
        process->env->root_cnode = SEL4UTILS_CNODE_SLOT;
        process->env->tcb = sel4osapi_process_copy_cap_into(process, parent_vka, process->native.thread.tcb.cptr, seL4_AllRights);
        /* setup data about untypeds */
        for (i = 0; i < user_untypeds_num && untyped_size < SEL4OSAPI_USER_PROCESS_UNTYPED_MEM_SIZE; i++) {
            seL4_CPtr proc_ut_cap = 0;

            if (user_untypeds_allocation[i] != 0)
            {
                continue;
            }

            proc_ut_cap = sel4osapi_process_copy_cap_into(process, parent_vka, user_untypeds[i].cptr, seL4_AllRights);

            /* set up the cap range */
            if (untyped_count == 0) {
                process->env->untypeds.start = proc_ut_cap;
            }
            process->env->untypeds.end = proc_ut_cap;

            user_untypeds_allocation[i] = process->env->pid;

            process->env->untyped_size_bits_list[untyped_count] = user_untypeds_size_bits[i];
            process->env->untyped_indices[untyped_count] = i;
            untyped_count++;

            untyped_size += 1 << user_untypeds_size_bits[i];
        }
        assert((process->env->untypeds.end - process->env->untypeds.start) + 1 <= user_untypeds_num);
#ifdef CONFIG_LIB_OSAPI_SYSCLOCK
        {
            /* copy the sysclock's EP */
            process->env->sysclock_server_ep = sel4osapi_process_copy_cap_into(process, parent_vka, sysclock_ep, seL4_AllRights);
            assert(process->env->sysclock_server_ep != 0);
        }
#endif
        {
            /* allocate an AEP for the process' idling */
            vka_object_t aep_obj = { 0 };

            error = vka_alloc_notification(parent_vka, &aep_obj);
            assert(error == 0);

            process->parent_idling_aep = aep_obj.cptr;

            process->env->idling_aep = sel4osapi_process_copy_cap_into(process, parent_vka, process->parent_idling_aep, seL4_AllRights);
            assert(process->env->idling_aep != 0);
        }
        {
            /* register process with ipc server */
            seL4_CPtr rx_pages[SEL4OSAPI_PROCESS_RX_BUF_PAGES];
            seL4_CPtr tx_pages[SEL4OSAPI_PROCESS_TX_BUF_PAGES];
            seL4_CPtr rx_pages_mint[SEL4OSAPI_PROCESS_RX_BUF_PAGES];
            seL4_CPtr tx_pages_mint[SEL4OSAPI_PROCESS_TX_BUF_PAGES];
            cspacepath_t dest, src;

            process->env->ipcclient.id = process->ipcclient->id;

            for (i = 0; i < SEL4OSAPI_PROCESS_RX_BUF_PAGES; ++i) {
                rx_pages[i] = vspace_get_cap(parent_vspace, process->ipcclient->rx_buf + i * PAGE_SIZE_4K);
                assert(rx_pages[i] != seL4_CapNull);
                vka_cspace_make_path(parent_vka, rx_pages[i], &src);
                error = vka_cspace_alloc(parent_vka, &rx_pages_mint[i]);
                assert(error == 0);
                vka_cspace_make_path(parent_vka, rx_pages_mint[i], &dest);
                error = vka_cnode_copy(&dest, &src, seL4_AllRights);
                assert(error == 0);
            }

            for (i = 0; i < SEL4OSAPI_PROCESS_TX_BUF_PAGES; ++i) {
                tx_pages[i] = vspace_get_cap(parent_vspace, process->ipcclient->tx_buf + i * PAGE_SIZE_4K);
                assert(tx_pages[i] != seL4_CapNull);
                vka_cspace_make_path(parent_vka, tx_pages[i], &src);
                error = vka_cspace_alloc(parent_vka, &tx_pages_mint[i]);
                assert(error == 0);
                vka_cspace_make_path(parent_vka, tx_pages_mint[i], &dest);
                error = vka_cnode_copy(&dest, &src, seL4_AllRights);
                assert(error == 0);
            }

            process->env->ipcclient.rx_buf = vspace_map_pages(&process->native.vspace, rx_pages_mint, NULL, seL4_AllRights, SEL4OSAPI_PROCESS_RX_BUF_PAGES, PAGE_BITS_4K, 1);
            assert(process->env->ipcclient.rx_buf != NULL);
            process->env->ipcclient.rx_buf_size = SEL4OSAPI_PROCESS_RX_BUF_SIZE;

            process->env->ipcclient.tx_buf = vspace_map_pages(&process->native.vspace, tx_pages_mint, NULL, seL4_AllRights, SEL4OSAPI_PROCESS_RX_BUF_PAGES, PAGE_BITS_4K, 1);
            assert(process->env->ipcclient.tx_buf != NULL);
            process->env->ipcclient.tx_buf_size = SEL4OSAPI_PROCESS_TX_BUF_SIZE;
        }
#ifdef CONFIG_LIB_OSAPI_NET
        {
            process->env->udp_iface.stack_op_ep = sel4osapi_process_copy_cap_into(process, parent_vka, udp_stack_ep, seL4_AllRights);
            assert(process->env->udp_iface.stack_op_ep != seL4_CapNull);

        }
#endif
#ifdef CONFIG_LIB_OSAPI_SERIAL
        {
            /* register process with network driver */
            seL4_CPtr buf_pages[SEL4OSAPI_SERIAL_BUF_PAGES];
            seL4_CPtr buf_pages_mint[SEL4OSAPI_SERIAL_BUF_PAGES];
            cspacepath_t dest, src;

            process->env->serial.id = process->serialclient->id;

            process->env->serial.server_ep = sel4osapi_process_copy_cap_into(process, parent_vka, process->serialclient->server_ep, seL4_AllRights);
            assert(process->env->serial.server_ep != seL4_CapNull);

            for (i = 0; i < SEL4OSAPI_SERIAL_BUF_PAGES; ++i) {
                buf_pages[i] = vspace_get_cap(parent_vspace, process->serialclient->buf + i * PAGE_SIZE_4K);
                assert(buf_pages[i] != seL4_CapNull);
                vka_cspace_make_path(parent_vka, buf_pages[i], &src);
                error = vka_cspace_alloc(parent_vka, &buf_pages_mint[i]);
                assert(error == 0);
                vka_cspace_make_path(parent_vka, buf_pages_mint[i], &dest);
                error = vka_cnode_copy(&dest, &src, seL4_AllRights);
                assert(error == 0);
            }

            process->env->serial.buf = vspace_map_pages(&process->native.vspace, buf_pages_mint, NULL, seL4_AllRights, SEL4OSAPI_SERIAL_BUF_PAGES, PAGE_BITS_4K, 1);
            assert(process->env->serial.buf != NULL);
            process->env->serial.buf_size = SEL4OSAPI_SERIAL_BUF_SIZE;

        }
#endif
        /* copy the fault endpoint - we wait on the endpoint for a message
         * or a fault to see when the test finishes */
        process->env->fault_endpoint = sel4osapi_process_copy_cap_into(process, parent_vka, process->native.fault_endpoint.cptr, seL4_AllRights);
        assert(process->env->fault_endpoint != 0);

        /* WARNING: DO NOT COPY MORE CAPS TO THE PROCESS BEYOND THIS POINT,
         * AS THE SLOTS WILL BE CONSIDERED FREE AND OVERRIDDEN BY THE TEST PROCESS. */
        /* set up free slot range */
        process->env->cspace_size_bits = CONFIG_SEL4UTILS_CSPACE_SIZE_BITS;
        process->env->free_slots.start = process->env->fault_endpoint + 1;
        process->env->free_slots.end = (1u << CONFIG_SEL4UTILS_CSPACE_SIZE_BITS);
        assert(process->env->free_slots.start < process->env->free_slots.end);
    }

    return 0;
}

sel4osapi_process_t*
sel4osapi_process_create(char *process_name, uint8_t priority)
{
    int error, pid;
    sel4osapi_system_t *system = sel4osapi_system_get_instanceI();
    vka_t *vka = sel4osapi_system_get_vka();
    vspace_t *vspace = sel4osapi_system_get_vspace();
    sel4osapi_process_t *process = NULL;

    process = (sel4osapi_process_t*) simple_pool_alloc(system->processes);
    if (process == NULL)
    {
        return NULL;
    }
    pid = simple_pool_get_current_size(system->processes);

    process->env = (sel4osapi_process_env_t*) vspace_new_pages(vspace, seL4_AllRights, 1, PAGE_BITS_4K);

    /* Configure new process */
    error = sel4utils_configure_process(&process->native, vka, vspace, priority, process_name);
    assert(error == 0);

#ifdef CONFIG_LIB_OSAPI_SERIAL
    process->serialclient = sel4osapi_io_serial_create_client(&system->serial);
    assert(process->serialclient != NULL);
#endif

    process->ipcclient = sel4osapi_ipc_create_client(&system->ipc, pid);
    assert(process->ipcclient != NULL);

    error = sel4osapi_process_init_env(process,
            pid, process_name, priority,
            vka, vspace,
            system->user_untypeds_num,
            system->user_untypeds_allocation,
            system->user_untypeds_size_bits,
            system->user_untypeds,
#ifdef CONFIG_LIB_OSAPI_SYSCLOCK
            system->sysclock.server_ep_obj.cptr
#else
            seL4_CapNull
#endif
            ,
#ifdef CONFIG_LIB_OSAPI_NET
            system->udp.stack_op_ep
#else
            seL4_CapNull
#endif
          );
    assert(!error);

    return process;

}

#if 0

extern uintptr_t sel4_vsyscall[];

sel4osapi_process_t*
sel4osapi_process_fork(char *process_name, uint8_t priority, void *entry_point)
{
    int error, pid;
    sel4osapi_system_t *system = sel4osapi_system_get_instance();
    sel4osapi_process_t *process = NULL;

    process = (sel4osapi_process_t*) simple_pool_alloc(system->processes);
    if (process == NULL)
    {
        return NULL;
    }
    pid = simple_pool_get_current_size(system->processes);

    process->env = (sel4osapi_process_env_t*) vspace_new_pages(&system->vspace, seL4_AllRights, 1, PAGE_BITS_4K);

    {
#define MAX_REGIONS     4
#define IMAGE_NAME      "root_task"
        sel4utils_elf_region_t elf_regions[MAX_REGIONS];
        int num_elf_regions;

        num_elf_regions = sel4utils_elf_num_regions(IMAGE_NAME);
        assert(num_elf_regions < MAX_REGIONS);
        sel4utils_elf_reserve(NULL, IMAGE_NAME, elf_regions);
    }

    sel4utils_process_config_t config = {
        .is_elf = false,
        .image_name = "",
        .do_elf_load = false,
        .create_cspace = true,
        .one_level_cspace_size_bits = CONFIG_SEL4UTILS_CSPACE_SIZE_BITS,
        .create_vspace = true,
        .create_fault_endpoint = true,
        .priority = priority,
#ifndef CONFIG_KERNEL_STABLE
        .asid_pool = seL4_CapInitThreadASIDPool,
#endif
        .sysinfo = (uintptr_t)sel4_vsyscall,
        .entry_point = entry_point,
    };

    /* Configure new process */
    error = sel4utils_configure_process_custom(&process->native, &system->vka, &system->vspace, config);
    assert(error == 0);

    process->serialclient = sel4osapi_io_serial_create_client();
    assert(process->serialclient != NULL);

    process->netclient = sel4osapi_network_create_client();
    assert(process->netclient != NULL);

    error = sel4osapi_process_init_env(process, pid, process_name, system->env->priority,
            &system->vka, &system->vspace,
            system->user_untypeds_num,
            system->user_untypeds_allocation,
            system->user_untypeds_size_bits,
            system->user_untypeds,
            system->sysclock.server_ep_obj.cptr,
            system->udp.stack_op_ep);
    assert(!error);

    return process;
}

#endif

int
sel4osapi_process_join(sel4osapi_process_t *process)
{
    /* wait on it to finish or fault, report result */
    seL4_Word badge;
    seL4_MessageInfo_t info = seL4_Recv(process->native.fault_endpoint.cptr, &badge);
    UNUSED int result = sel4osapi_getMR(0);
    seL4_Uint32 label = seL4_MessageInfo_get_label(info);
    return label;
}


void
sel4osapi_process_delete(sel4osapi_process_t *process)
{
    sel4osapi_system_t *system = sel4osapi_system_get_instanceI();
    vka_t *vka = sel4osapi_system_get_vka();
    cspacepath_t path;

    /* unmap the env.init data frame */
    vspace_unmap_pages(&process->native.vspace, process->env_remote_vaddr, 1, PAGE_BITS_4K, NULL);

    int assigned_untypeds = (process->env->untypeds.end - process->env->untypeds.start)+1;
    /* reset all the untypeds that were assigned to the process*/
    for (int i = 0; i < assigned_untypeds; i++) {
        vka_cspace_make_path(vka, process->env->untypeds.start + i, &path);
        vka_cnode_revoke(&path);
        system->user_untypeds_allocation[process->env->untyped_indices[i]] = 0;
    }

    /* destroy the process */
    sel4utils_destroy_process(&process->native, vka);
    fflush(stdout);
}

void
sel4osapi_process_signal_root(sel4osapi_process_env_t *process, int exit_code)
{

    seL4_MessageInfo_t info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    sel4osapi_setMR(0, (seL4_Word) exit_code);
    seL4_Send(process->fault_endpoint, info);
    sel4osapi_idle();
}

void
sel4osapi_process_spawn(sel4osapi_process_t *process)
{
    int error = 0;
    /* set up args for the test process */
    char process_name[SEL4OSAPI_USER_PROCESS_NAME_MAX_LEN + 1];
    char endpoint_string[10];
    char pid_string[10];
    char *argv[] = {process_name, pid_string, endpoint_string};
    seL4_CPtr process_data_frame;
    seL4_CPtr process_data_frame_copy;
    /*sel4osapi_system_t *system = sel4osapi_system_get_instance();*/
    vka_t *vka = sel4osapi_system_get_vka();
    vspace_t *vspace = sel4osapi_system_get_vspace();
    void *vaddr = NULL;

#ifdef SEL4_DEBUG_KERNEL
    seL4_DebugNameThread(process->native.thread.tcb.cptr, process->env->name);
#endif
    snprintf(process_name, SEL4OSAPI_USER_PROCESS_NAME_MAX_LEN, "%s", process->env->name);
    snprintf(endpoint_string, 10, "%d", process->env->fault_endpoint);
    snprintf(pid_string, 10, "%d", process->env->pid);

    /* spawn the process */
    error = sel4utils_spawn_process_v(&process->native, vka, vspace, ARRAY_SIZE(argv), argv, 1);
    assert(error == 0);

    /* send env to the new process */

    /* Create a copy of the process_data_frame for the new process */
    process_data_frame = vspace_get_cap(vspace, process->env);
    sel4osapi_util_copy_cap(vka, process_data_frame, &process_data_frame_copy);

    /* map the cap into remote vspace */
    vaddr = vspace_map_pages(&process->native.vspace, &process_data_frame_copy, NULL, seL4_AllRights, 1, PAGE_BITS_4K, 1);
    assert(vaddr != 0);

    /* now send a message telling the process what address the data is at */
    seL4_MessageInfo_t info = seL4_MessageInfo_new(seL4_Fault_NullFault, 0, 0, 1);
    sel4osapi_setMR(0, (seL4_Word) vaddr);
    seL4_Send(process->native.fault_endpoint.cptr, info);
}

void
sel4osapi_process_print_env(sel4osapi_process_env_t *env)
{
    printf("page_directory=%u\n", env->page_directory);
    printf("root_cnode=%u\n", env->root_cnode);
    printf("tcb=%u\n", env->tcb);
    printf("cspace_size_bits=%u\n", env->cspace_size_bits);
    printf("free_slots.start=%u\n", env->free_slots.start);
    printf("free_slots.end=%u\n", env->free_slots.end);
    printf("untypeds.start=%u\n", env->untypeds.start);
    printf("untypeds.end=%u\n", env->untypeds.end);
    int i = 0;
    for (i = 0; i < CONFIG_MAX_NUM_BOOTINFO_UNTYPED_CAPS; i++)
    {
        if (env->untyped_size_bits_list[i] == 0)
        {
            continue;
        }
        printf("process_data.untypeds_size_bits_list[%d]=%u\n", i, env->untyped_size_bits_list[i]);
    }
    printf("fault_endpoint=%u\n", env->fault_endpoint);
}

sel4osapi_process_env_t*
sel4osapi_process_get_current()
{
    sel4osapi_system_t *system = sel4osapi_system_get_instanceI();
    return system->env;
}


seL4_CPtr
sel4osapi_process_copy_cap_into(sel4osapi_process_t *process, vka_t *parent_vka, seL4_CPtr cap, seL4_CapRights_t rights)
{
    seL4_CPtr minted_cap;
    seL4_CapData_t cap_badge;
    cspacepath_t src_path;

    cap_badge = seL4_CapData_Badge_new(process->env->pid);

    vka_cspace_make_path(parent_vka, cap, &src_path);
    minted_cap = sel4utils_mint_cap_to_process(&process->native, src_path, rights, cap_badge);
    assert(minted_cap != 0);

    return minted_cap;
}

