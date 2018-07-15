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


/*
 * Log Subsystem Usage Summary
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * Initialize:
 *      sel4osapi_log_initialize();
 * 
 * Set verbosity:
 *      sel4osapi_log_set_level(SEL4OSAPI_LOG_LEVEL_INFO);      // Default value
 *
 * Log a message (with and without arguments):
 *      syslog_xxxx("Hello world");
 *      syslog_xxxx("Hello world, arg=%d", 10);
 *
 * Log functions:
 *      syslog_trace
 *      syslog_info
 *      syslog_warn
 *      syslog_error
 *
 * NOTE:
 *      Syslog will work even before calling sel4osapi_log_initialize(), but will not
 *      be thread-safe. After calling sel4osapi_log_initialize() the log subsystem will
 *      be thread-safe (as it will use a global mutex to serialize log messages).
 *
 */

#ifndef SEL4OSAPI_LOG_H_
#define SEL4OSAPI_LOG_H_

#define SEL4OSAPI_LOG_FORMAT    "[%s][%03d][%s][%5s] "

#define SEL4OSAPI_LOG_ARGS \
	sel4osapi_thread_get_current()->name, \
	sel4osapi_thread_get_current()->priority, \
        __FUNCTION__

typedef enum sel4osapi_loglevel
{
    SEL4OSAPI_LOG_LEVEL_ERROR = 0,
    SEL4OSAPI_LOG_LEVEL_WARN = 1,
    SEL4OSAPI_LOG_LEVEL_INFO = 2,
    SEL4OSAPI_LOG_LEVEL_TRACE = 3,
} sel4osapi_loglevel_t;

extern sel4osapi_loglevel_t sel4osapi_gv_loglevel;
extern sel4osapi_mutex_t  * sel4osapi_gv_logmutex;
extern int sel4osapi_logconsole;

/*
 * Start monitoring a thread's status by
 * spawning a thread which will "subscribe" to
 * the thread's fault endpoint.
 */
int
sel4osapi_syslog_monitor_thread(sel4osapi_thread_t *thread);

#if 1
/*
#define __syslog_logMessage(msg, level, levelStr, ...) \
    if (sel4osapi_gv_loglevel >= level) { \
        if (sel4osapi_gv_logmutex) { \
            sel4osapi_log_lock();\
            printf(SEL4OSAPI_LOG_FORMAT msg"\n", SEL4OSAPI_LOG_ARGS , levelStr, ##__VA_ARGS__); \
            sel4osapi_log_unlock();\
        } else { \
            printf(SEL4OSAPI_LOG_FORMAT msg"\n", "N/A", 0, __FUNCTION__, levelStr, ##__VA_ARGS__); \
        } \
    }

#define syslog_trace(msg, ...)  __syslog_logMessage(msg, SEL4OSAPI_LOG_LEVEL_TRACE, "TRACE", ##__VA_ARGS__)
#define syslog_info(msg, ...)   __syslog_logMessage(msg, SEL4OSAPI_LOG_LEVEL_INFO,  "INFO",  ##__VA_ARGS__)
#define syslog_warn(msg, ...)   __syslog_logMessage(msg, SEL4OSAPI_LOG_LEVEL_WARN,  "WARN",  ##__VA_ARGS__)
*/

#define SEL4OSAPI_DEFAULT_ARGS      __FILE__,__FUNCTION__, __LINE__

extern void __syslog_logMessage(sel4osapi_loglevel_t level,
                                const char *levelStr,
                                const char *file,
                                const char *function,
                                const int line,
                                const char *msg, ...);

#define syslog_trace(msg, ...)  __syslog_logMessage(SEL4OSAPI_LOG_LEVEL_TRACE, "TRACE", SEL4OSAPI_DEFAULT_ARGS, msg, ##__VA_ARGS__)
#define syslog_info(msg, ...)   __syslog_logMessage(SEL4OSAPI_LOG_LEVEL_INFO,  "INFO",  SEL4OSAPI_DEFAULT_ARGS, msg, ##__VA_ARGS__)
#define syslog_warn(msg, ...)   __syslog_logMessage(SEL4OSAPI_LOG_LEVEL_WARN,  "WARN",  SEL4OSAPI_DEFAULT_ARGS, msg, ##__VA_ARGS__)
#define syslog_error(msg, ...)  __syslog_logMessage(SEL4OSAPI_LOG_LEVEL_ERROR, "ERROR", SEL4OSAPI_DEFAULT_ARGS, msg, ##__VA_ARGS__)

#else

/*
 * Log a message at the TRACE level
 */
#define syslog_trace(msg, ...)\
if (sel4osapi_gv_loglevel >= SEL4OSAPI_LOG_LEVEL_TRACE)\
{\
    sel4osapi_log_lock();\
    printf(SEL4OSAPI_LOG_FORMAT msg"\n", SEL4OSAPI_LOG_ARGS , "TRACE", ##__VA_ARGS__);\
    sel4osapi_log_unlock();\
}



/*
 * Log a message at the INFO level
 */
#define syslog_info(msg, ...)\
if (sel4osapi_gv_loglevel >= SEL4OSAPI_LOG_LEVEL_INFO)\
{\
    sel4osapi_log_lock();\
    printf(SEL4OSAPI_LOG_FORMAT msg"\n", SEL4OSAPI_LOG_ARGS , "INFO", ##__VA_ARGS__);\
    sel4osapi_log_unlock();\
}


/*
 * Log a message at the warning level
 */
#define syslog_warn(msg, ...)\
if (sel4osapi_gv_loglevel >= SEL4OSAPI_LOG_LEVEL_WARN)\
{\
    sel4osapi_log_lock();\
    printf(SEL4OSAPI_LOG_FORMAT msg"\n", SEL4OSAPI_LOG_ARGS , "WARN", ##__VA_ARGS__);\
    sel4osapi_log_unlock();\
}

/*
 * Log a message at the error level
 */
#define syslog_error(msg, ...)\
if (sel4osapi_gv_loglevel >= SEL4OSAPI_LOG_LEVEL_ERROR)\
{\
    sel4osapi_log_lock();\
    printf(SEL4OSAPI_LOG_FORMAT msg"\n", SEL4OSAPI_LOG_ARGS , "ERROR", ##__VA_ARGS__);\
    sel4osapi_log_unlock();\
}
#endif

void
sel4osapi_log_set_level(sel4osapi_loglevel_t level);

int
sel4osapi_log_initialize();

void
sel4osapi_log_lock();

void
sel4osapi_log_unlock();


static inline void sel4osapi_log_set_console(int logConsole) {
    syslog_info("================================");
    syslog_info("Switching logging console...");
    syslog_info("================================");
    sel4osapi_logconsole = logConsole;
    syslog_info("Logging continue on secondary console...");
}

#endif /* SEL4OSAPI_LOG_H_ */
