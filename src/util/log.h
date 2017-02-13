#ifndef __STORE_UTIL_LOG_H_
#define __STORE_UTIL_LOG_H_

#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>

namespace store {

extern int g_log_level;
extern pthread_key_t g_log_data_thread_key;

enum log_error_t {
    LOG_PARAM_ERROR        = -5500,
    LOG_CREATE_DIR_ERROR   = -5501,
    LOG_OPEN_FILE_ERROR    = -5502,
    LOG_CREATE_THREAD_FAIL = -5503,
    LOG_MALLOC_ERROR       = -5504,
    LOG_CREATE_KEY_FAIL    = -5505,
};

enum log_level_t {
    STORE_LOG_DEBUG   = 16,
    STORE_LOG_TRACE   = 8,
    STORE_LOG_NOTICE  = 4,
    STORE_LOG_WARNING = 2,
    STORE_LOG_FATAL   = 1,
};

struct log_data_t;

#define log_base(data, lvl, ...) do {       \
    log_data_t* ld = data;                  \
    if (ld == NULL) {                       \
        ld = (log_data_t*)pthread_getspecific(g_log_data_thread_key); \
    }                                       \
    if (g_log_level >= STORE_LOG_ ## lvl) { \
        log_printf(ld, STORE_LOG_ ## lvl, __FILE__ ":" LINESTR, __FUNCTION__, #lvl, __VA_ARGS__); \
    }                                       \
} while(0)

#define _TOSTR(x) #x
#define _LINESTR(x) _TOSTR(x)
#define LINESTR _LINESTR(__LINE__)

#define rlog_debug(data, ...) log_base(data, DEBUG, __VA_ARGS__)
#define rlog_trace(data, ...) log_base(data, TRACE, __VA_ARGS__)
#define rlog_notice(data, ...) log_base(data, NOTICE, __VA_ARGS__)
#define rlog_warning(data, ...) log_base(data, WARNING, __VA_ARGS__)
#define rlog_fatal(data, ...) log_base(data, FATAL, __VA_ARGS__)
#define log_debug(...)   log_base(NULL, DEBUG, __VA_ARGS__)
#define log_trace(...)   log_base(NULL, TRACE, __VA_ARGS__)
#define log_notice(...)  log_base(NULL, NOTICE, __VA_ARGS__)
#define log_warning(...) log_base(NULL, WARNING, __VA_ARGS__)
#define log_fatal(...)   log_base(NULL, FATAL, __VA_ARGS__)

#define _log_level_check_base(lvl) (g_log_level >= STORE_LOG_ ## lvl)
#define log_has_debug() _log_level_check_base(DEBUG)
#define log_has_trace() _log_level_check_base(TRACE)
#define log_has_notice() _log_level_check_base(NOTICE)
#define log_has_warning() _log_level_check_base(WARNING)
#define log_has_fatal() _log_level_check_base(FATAL)

log_data_t* log_data_new();
void log_data_free(log_data_t* ld);

int log_init(const char* dir, const char* bin_name, int log_level = STORE_LOG_DEBUG);
int log_close();
int log_join();
void log_data_push(log_data_t* ld, const char* key, const char* value);  // alias for log_data_push_str
void log_set_thread_log_data(log_data_t* ld);
log_data_t* log_get_thread_log_data();
void log_data_push_str(log_data_t* ld, const char* key, const char* value);
void log_data_push_int(log_data_t* ld, const char* key, long value);
void log_data_push_uint(log_data_t* ld, const char* key, unsigned long value);
void log_data_push_double(log_data_t* ld, const char* key, double value);

// inner function
int log_printf(log_data_t* ld, log_level_t level, const char* fileline, const char* function, const char* levelstr, const char* format, ...) __attribute__ ((format (printf, 6, 7)));

}  // namespace store

#endif  // __STORE_UTIL_LOG_H_
