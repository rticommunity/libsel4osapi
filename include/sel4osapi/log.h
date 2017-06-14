/*
 * FILE: log.h - log functions for sel4osapi
 *
 * Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#ifndef SEL4OSAPI_LOG_H_
#define SEL4OSAPI_LOG_H_

#define SEL4OSAPI_LOG_FORMAT    "[%08d][%s][%03d][%5s] "

#ifdef CONFIG_LIB_OSAPI_SYSCLOCK
#define SEL4OSAPI_LOG_ARGS \
	sel4osapi_sysclock_get_time(), \
	sel4osapi_thread_get_current()->name, \
	sel4osapi_thread_get_current()->priority
#else
#define SEL4OSAPI_LOG_ARGS \
	0, \
	sel4osapi_thread_get_current()->name, \
	sel4osapi_thread_get_current()->priority
#endif

typedef enum sel4osapi_loglevel
{
    SEL4OSAPI_LOG_LEVEL_ERROR = 0,
    SEL4OSAPI_LOG_LEVEL_WARN = 1,
    SEL4OSAPI_LOG_LEVEL_INFO = 2,
    SEL4OSAPI_LOG_LEVEL_TRACE = 3,
} sel4osapi_loglevel_t;

extern sel4osapi_loglevel_t sel4osapi_gv_loglevel;

/*
 * Start monitoring a thread's status by
 * spawning a thread which will "subscribe" to
 * the thread's fault endpoint.
 */
int
sel4osapi_syslog_monitor_thread(sel4osapi_thread_t *thread);

/*
 * Log a new message.
 */
void
sel4osapi_syslog_log(sel4osapi_loglevel_t level, char *msg);

/*
 * Log a message at the TRACE level
 */
#define syslog_trace(msg)\
if (sel4osapi_gv_loglevel >= SEL4OSAPI_LOG_LEVEL_TRACE)\
{\
    sel4osapi_log_lock();\
    printf(SEL4OSAPI_LOG_FORMAT msg"\n", SEL4OSAPI_LOG_ARGS, "TRACE");\
    sel4osapi_log_unlock();\
}

#define syslog_trace_a(msg, ...)\
if (sel4osapi_gv_loglevel >= SEL4OSAPI_LOG_LEVEL_TRACE)\
{\
    sel4osapi_log_lock();\
    printf(SEL4OSAPI_LOG_FORMAT msg"\n", SEL4OSAPI_LOG_ARGS , "TRACE",  __VA_ARGS__);\
    sel4osapi_log_unlock();\
}
/*
 * Log a message at the INFO level
 */
#define syslog_info(msg)\
if (sel4osapi_gv_loglevel >= SEL4OSAPI_LOG_LEVEL_INFO)\
{\
    sel4osapi_log_lock();\
    printf(SEL4OSAPI_LOG_FORMAT msg"\n", SEL4OSAPI_LOG_ARGS, "INFO");\
    sel4osapi_log_unlock();\
}

/*
 * Log a message at the INFO level, with the specified arguments
 */

#define syslog_info_a(msg, ...)\
if (sel4osapi_gv_loglevel >= SEL4OSAPI_LOG_LEVEL_INFO)\
{\
    sel4osapi_log_lock();\
    printf(SEL4OSAPI_LOG_FORMAT msg"\n", SEL4OSAPI_LOG_ARGS , "INFO",  __VA_ARGS__);\
    sel4osapi_log_unlock();\
}


/*
 * Log a message at the warning level
 */
#define syslog_warn_a(msg, ...)\
if (sel4osapi_gv_loglevel >= SEL4OSAPI_LOG_LEVEL_WARN)\
{\
    sel4osapi_log_lock();\
    printf(SEL4OSAPI_LOG_FORMAT msg"\n", SEL4OSAPI_LOG_ARGS , "WARN",  __VA_ARGS__);\
    sel4osapi_log_unlock();\
}

/*
 * Log a message at the error level
 */
#define syslog_error_a(msg, ...)\
if (sel4osapi_gv_loglevel >= SEL4OSAPI_LOG_LEVEL_ERROR)\
{\
    sel4osapi_log_lock();\
    printf(SEL4OSAPI_LOG_FORMAT msg"\n", SEL4OSAPI_LOG_ARGS , "ERROR",  __VA_ARGS__);\
    sel4osapi_log_unlock();\
}

/*
 * Log a message at the warning level
 */
#define syslog_warn(msg)\
if (sel4osapi_gv_loglevel >= SEL4OSAPI_LOG_LEVEL_WARN)\
{\
    sel4osapi_log_lock();\
    printf(SEL4OSAPI_LOG_FORMAT msg"\n", SEL4OSAPI_LOG_ARGS , "WARN");\
    sel4osapi_log_unlock();\
}

/*
 * Log a message at the error level
 */
#define syslog_error(msg)\
if (sel4osapi_gv_loglevel >= SEL4OSAPI_LOG_LEVEL_ERROR)\
{\
    sel4osapi_log_lock();\
    printf(SEL4OSAPI_LOG_FORMAT msg"\n", SEL4OSAPI_LOG_ARGS , "ERROR");\
    sel4osapi_log_unlock();\
}

void
sel4osapi_log_set_level(sel4osapi_loglevel_t level);

int
sel4osapi_log_initialize();

void
sel4osapi_log_lock();

void
sel4osapi_log_unlock();

#endif /* SEL4OSAPI_LOG_H_ */
