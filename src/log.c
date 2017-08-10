/*
 * FILE: log.c - logging functions for sel4osapi
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#include <sel4osapi/osapi.h>


sel4osapi_mutex_t * sel4osapi_gv_logmutex = NULL;

sel4osapi_loglevel_t sel4osapi_gv_loglevel = SEL4OSAPI_LOG_LEVEL_INFO;

void
sel4osapi_syslog_monitoring_thread(sel4osapi_thread_info_t *thread)
{
    sel4osapi_thread_t * monitored = (sel4osapi_thread_t*) thread->arg;
    seL4_Word sender;
    while (thread->active)
    {
        printf("[syslog][MONITORING][%s]\n",monitored->info.name);
        seL4_Wait(monitored->fault_endpoint,&sender);
        printf("[syslog][FAULT][%s]\n",monitored->info.name);
        fflush(stdout);
        /*printf("[syslog][%s][RESTARTING]\n",monitored->info.name);
        seL4_MessageInfo_t reply_tag = seL4_MessageInfo_new(0,0,0,1);
        seL4_Reply(reply_tag);*/
    }
}

int
sel4osapi_syslog_monitor_thread(sel4osapi_thread_t *thread)
{
    sel4osapi_thread_t *monitoring = NULL;
    char monitoring_name[SEL4OSAPI_THREAD_NAME_MAX_LEN];
    int error = 0;

    snprintf(monitoring_name, SEL4OSAPI_THREAD_NAME_MAX_LEN, "syslog::%s",thread->info.name);
    monitoring = sel4osapi_thread_create(monitoring_name, sel4osapi_syslog_monitoring_thread,thread,thread->info.priority);
    assert(monitoring != NULL);
    error = sel4osapi_thread_start(monitoring);
    assert(error == 0);

    return 0;
}

void
sel4osapi_log_set_level(sel4osapi_loglevel_t level)
{
    assert((level >= SEL4OSAPI_LOG_LEVEL_ERROR) && (level <= SEL4OSAPI_LOG_LEVEL_TRACE));
    sel4osapi_gv_loglevel = level;
}

int
sel4osapi_log_initialize()
{
    if (!sel4osapi_gv_logmutex) {
        sel4osapi_gv_logmutex = sel4osapi_mutex_create();
        assert(sel4osapi_gv_logmutex != NULL);
    } else {
        syslog_warn("OSAPI Syslog initialized more than once, ignoring subsequent initializations...");
    }
    return 0;
}

void
sel4osapi_log_lock()
{
    if (sel4osapi_gv_logmutex) {
        int error = sel4osapi_mutex_lock(sel4osapi_gv_logmutex);
        assert(!error);
    }
}

void
sel4osapi_log_unlock()
{
    if (sel4osapi_gv_logmutex) {
        assert(sel4osapi_gv_logmutex);
        sel4osapi_mutex_unlock(sel4osapi_gv_logmutex);
    }
}


