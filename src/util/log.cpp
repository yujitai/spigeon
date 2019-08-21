#include "util/log.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <list>
#include <string.h>

#include "util/url_snprintf.h"
#include "util/zmalloc.h"
#include "server/event.h"
#include "def.h"

namespace zf {

static const size_t FILE_NAME_MAX    = 1024;
static const size_t BIN_NAME_MAX     = 32;
static const int    LOG_MSLEEP_TIME  = 5;
static const int    LOG_BUF_LEN_MAX  = 2 * 2048;
static const int    LOG_ITEM_LEN_MAX = 2048;

typedef struct log_data_t {
    char   buf[LOG_ITEM_LEN_MAX];
    int    len;
    struct log_data_t* prev;
    struct log_data_t* next;
    struct log_data_t* tail;
} log_data_t;

typedef struct _log_buf_t {
    char buf[LOG_BUF_LEN_MAX];
    int  len;
    log_level_t level;
} log_buf_t;

int g_log_level;
static bool  g_write_to_stderr = false;
static char  g_log_filename[FILE_NAME_MAX];
static char  g_logwf_filename[FILE_NAME_MAX];
static char   g_logmonitor_filename[FILE_NAME_MAX];
static FILE* g_log_f = NULL;
static FILE* g_logwf_f = NULL;
static FILE* g_logmonitor_f = NULL;
static char  g_log_dir[FILE_NAME_MAX];
static char  g_bin_name[BIN_NAME_MAX];
typedef std::list<log_buf_t*> store_log_list_t;
static bool g_log_running = true;
static store_log_list_t g_log_list;
static pthread_mutex_t g_log_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t g_log_thread;
pthread_key_t g_log_data_thread_key;

log_data_t* log_data_new() {
    log_data_t* ld = (log_data_t*)zmalloc(sizeof(log_data_t));
    ld->buf[0] = '\0';
    ld->len  = 0;
    ld->next = NULL;
    ld->prev = NULL;
    ld->tail = ld;
    return ld;
}

void log_data_free(log_data_t* ld) {
    log_data_t* next = ld;
    log_data_t* tmp;
    while(next) {
        tmp = next->next;
        zfree(next);
        next = tmp;
    }
}

void log_data_push(log_data_t* ld, const char* key, const char* value) {
    log_data_push_str(ld, key, value);
}

#ifdef NOT_URLENCODE_LOG
#define ENCODED_SNPRINTF snprintf
#define ENCODED_VSNPRINTF vsnprintf
#else
#define ENCODED_SNPRINTF url_snprintf
#define ENCODED_VSNPRINTF url_vsnprintf
#endif

#define _ADJUST_LOG_BUF_CURSOR(ret, buf, size)  do { \
    if (ret < 0) {                                   \
        ret = 0;                                     \
    } else if (ret >= (size)) {                      \
        ret = (size) - 1;                            \
    }                                                \
    (buf) += ret;                                    \
    (size) -= ret;                                   \
} while (0)

// not use the first element
#define _PUSH(ld, key, value, value_fmt) do { \
    log_data_t* node = ld->tail;              \
    if (node->len > 0) {                      \
        node->next = log_data_new();          \
        if (node->next == NULL) {             \
            return;                           \
        }                                     \
        node->next->prev = node;              \
        node = node->next;                    \
        ld->tail = node;                      \
    }                                         \
    char* buf = node->buf;                    \
    int   size = (int)sizeof(node->buf);      \
    int   ret;                                \
    ret = ENCODED_SNPRINTF(buf, size, "%s", key); \
    _ADJUST_LOG_BUF_CURSOR(ret, buf, size);   \
    ret = snprintf(buf, size, "=");           \
    _ADJUST_LOG_BUF_CURSOR(ret, buf, size);   \
    ret = ENCODED_SNPRINTF(buf, size, value_fmt, value); \
    _ADJUST_LOG_BUF_CURSOR(ret, buf, size);   \
    node->len = buf - node->buf;              \
} while (0)

#define _ADD_BLANK_SPACE(buf, size) do { \
    if (size > 0) {                      \
        *buf = ' ';                      \
        *++buf = '\0';                   \
        --size;                          \
    }                                    \
} while (0)

time_t get_current_time() {
#ifdef USE_SYS_TIME
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec;
#else
    return store::EventLoop::current_time() / 1000000;
#endif
}

void log_data_push_str(log_data_t* ld, const char* key, const char* value) {
    _PUSH(ld, key, value, "%s");
}
void log_data_push_int(log_data_t* ld, const char* key, long value) {
    _PUSH(ld, key, value, "%ld");
}
void log_data_push_uint(log_data_t* ld, const char* key, unsigned long value) {
    _PUSH(ld, key, value, "%lu");
}
void log_data_push_double(log_data_t* ld, const char* key, double value) {
    _PUSH(ld, key, value, "%lf");
}

int log_printf(log_data_t *ld, log_level_t level, const char *fileline, 
        const char* function, const char *levelstr, const char* format, ...) 
{
    log_buf_t* p_log = (log_buf_t*)zmalloc(sizeof(log_buf_t));
    if (NULL == p_log) {
        fprintf(stderr, "zmalloc failed!");
        return LOG_MALLOC_ERROR;
    }
    p_log->level = level;
    
    int   ret;
    char* buf = p_log->buf;
    int   size = (int)sizeof(p_log->buf);
    log_data_t *cur = ld;
    time_t nowtime = get_current_time();

    // 学生端网络监控日志
    if(level == STORE_LOG_MONITOR){
        goto user_msg;
    }
    // prefix part
    // construct time
    char timebuf[32];
    struct tm* nowtm;
    nowtm = localtime(&nowtime);
    strftime(timebuf, sizeof(timebuf), "%m-%d %X", nowtm);
    //(yujitai) add color control code
    if (STORE_LOG_WARNING == level) {
        ret = snprintf(buf, size, "%s%s: %s %s * %d [", YELLOW, levelstr, timebuf, g_bin_name, getpid());
    } else if (STORE_LOG_FATAL == level){
        ret = snprintf(buf, size, "%s%s: %s %s * %d [", BOLDRED, levelstr, timebuf, g_bin_name, getpid());
    } else if (STORE_LOG_NOTICE == level){
        ret = snprintf(buf, size, "%s%s: %s %s * %d [", CYAN, levelstr, timebuf, g_bin_name, getpid());
    } else if (STORE_LOG_TRACE == level){
        ret = snprintf(buf, size, "%s%s: %s %s * %d [", WHITE, levelstr, timebuf, g_bin_name, getpid());
    } else {
        ret = snprintf(buf, size, "%s%s: %s %s * %d [", BOLDWHITE, levelstr, timebuf, g_bin_name, getpid());
    }
    _ADJUST_LOG_BUF_CURSOR(ret, buf, size);

    // suffix part
    if (fileline) {
        ret = snprintf(buf, size, "file=");
        _ADJUST_LOG_BUF_CURSOR(ret, buf, size);
        ret = ENCODED_SNPRINTF(buf, size, "%s", fileline);
        _ADJUST_LOG_BUF_CURSOR(ret, buf, size);
    }
    if (function) {
        ret = snprintf(buf, size, " function=");
        _ADJUST_LOG_BUF_CURSOR(ret, buf, size);
        ret = ENCODED_SNPRINTF(buf, size, "%s", function);
        _ADJUST_LOG_BUF_CURSOR(ret, buf, size);
        _ADD_BLANK_SPACE(buf, size);
    }
    while(cur) {
        if (size > cur->len) {
            ret = cur->len;
        } else if (size > 0) {
            ret = size - 1;
        } else {
            ret = 0;
        }
        memcpy(buf, cur->buf, ret);
        buf[ret] = '\0';
        buf += ret;
        size -= ret;
        // use blank space to seperate log_data_pushed items
        _ADD_BLANK_SPACE(buf, size);
        cur = cur->next;
    }
user_msg:
    // user message part
    va_list fmtargs;
    va_start(fmtargs, format);
    if(STORE_LOG_MONITOR != level){
        ret = snprintf(buf, size, "msg=");
        _ADJUST_LOG_BUF_CURSOR(ret, buf, size);
    }
    ret = ENCODED_VSNPRINTF(buf, size, format, fmtargs);
    _ADJUST_LOG_BUF_CURSOR(ret, buf, size);
    va_end(fmtargs);

    // put ending "]\n" to buf
    if(STORE_LOG_MONITOR != level){
        // (yujitai)add color control code
        ret = snprintf(buf, size, "]\033[0m\n");
    } else {
        ret = snprintf(buf, size, "]\033[0m\n");
    }
    _ADJUST_LOG_BUF_CURSOR(ret, buf, size);
    if (*(buf - 1) != '\n') {
        *(buf-1) = '\n';
    }
    p_log->len = buf - p_log->buf;
    ret = p_log->len;

    // push back to the logging list
    if (g_write_to_stderr) {
        fprintf(stderr, "%s", p_log->buf);
    }
    pthread_mutex_lock(&g_log_list_mutex);
    g_log_list.push_back(p_log);
    pthread_mutex_unlock(&g_log_list_mutex);

    // ret the written log size
    return ret;
}

void log_set_thread_log_data(log_data_t* ld) {
    pthread_setspecific(g_log_data_thread_key, ld);
}

log_data_t* log_get_thread_log_data() {
    log_data_t* ld = (log_data_t*)pthread_getspecific(g_log_data_thread_key);
    if (ld == NULL) {
        ld = log_data_new();
        pthread_setspecific(g_log_data_thread_key, ld);
    }
    return ld;
}

static void* log_thread_loop(void*) {
    struct stat stat_data;
    FILE* ftemp = NULL;
    const unsigned int usecs = LOG_MSLEEP_TIME * 1000;

    pthread_setname_np(pthread_self(), "framework_log");

    while(g_log_running) {
        usleep(usecs); 
        //check if file is moved or deleted
        if (stat(g_log_filename, &stat_data) < 0) {
            ftemp = fopen(g_log_filename, "a");
            if (ftemp != NULL) {
                fclose(g_log_f);
                g_log_f = ftemp;
                ftemp = NULL;
            }
        }
        if (stat(g_logwf_filename, &stat_data) < 0) {
            ftemp = fopen(g_logwf_filename, "a");
            if (ftemp != NULL) {
                fclose(g_logwf_f);
                g_logwf_f = ftemp;
                ftemp = NULL;
            }
        }
        if (stat(g_logmonitor_filename, &stat_data) < 0) {
            ftemp = fopen(g_logmonitor_filename, "a");
            if (ftemp != NULL) {
                fclose(g_logmonitor_f);
                g_logmonitor_f = ftemp;
                ftemp = NULL;
            }
        }
        bool write_log = false;
        bool write_logwf = false;
        bool write_logmonitor = false;
        while (!g_log_list.empty()) {
            log_buf_t* p_log = NULL;
            p_log = *g_log_list.begin();
            FILE* logfile = NULL;
            if (p_log->level <= STORE_LOG_WARNING) {
                logfile = g_logwf_f;
                write_logwf = true;
            } else if(p_log->level <= STORE_LOG_DEBUG){
                logfile = g_log_f;
                write_log = true;
            } else {
                logfile = g_logmonitor_f;
                write_logmonitor = true;
            }
            fwrite(p_log->buf, 1, p_log->len, logfile);
            pthread_mutex_lock(&g_log_list_mutex);
            g_log_list.pop_front();
            pthread_mutex_unlock(&g_log_list_mutex);
            zfree(p_log);
        }
        if (write_log) {
            fflush(g_log_f);
        }
        if (write_logwf) {
            fflush(g_logwf_f);
        }
        if (write_logmonitor) {
            fflush(g_logmonitor_f);
        }
    }
    return NULL;
}

int log_init(const char* dir, const char* bin_name, int log_level) {
    if (strlen(dir) <= 0 || strlen(dir) >= FILE_NAME_MAX || strlen(bin_name) <= 0 || strlen(bin_name) >= BIN_NAME_MAX) {
        return LOG_PARAM_ERROR;
    }
    g_log_level = log_level;
    if (getenv("log_to_stderr") != NULL) {
        g_write_to_stderr = true;
    }

    // open log/logwf files
    int ret = mkdir(dir, 0755);
    if (ret != 0 && errno != EEXIST) {
        perror("create dir failed!");
        return LOG_CREATE_DIR_ERROR;
    }
    snprintf(g_log_dir, sizeof(g_log_dir), "%s", dir);
    snprintf(g_bin_name, sizeof(g_bin_name), "%s", bin_name);
    snprintf(g_log_filename, sizeof(g_log_filename), "%s/%s.log", g_log_dir, g_bin_name);
    snprintf(g_logwf_filename, sizeof(g_logwf_filename), "%s/%s.log.wf", g_log_dir, g_bin_name);
    snprintf(g_logmonitor_filename, sizeof(g_logmonitor_filename), "%s/%s.log.monitor", g_log_dir, g_bin_name);
    g_log_f = fopen(g_log_filename, "a");
    if (NULL == g_log_f) {
        perror("open log file failed");
        return LOG_OPEN_FILE_ERROR;
    }
    g_logwf_f = fopen(g_logwf_filename, "a");
    if (NULL == g_logwf_f) {
        perror("open logwf file failed");
        return LOG_OPEN_FILE_ERROR;
    }
    g_logmonitor_f = fopen(g_logmonitor_filename, "a");
    if (NULL == g_logmonitor_f) {
        perror("open logmonitor file failed");
        return LOG_OPEN_FILE_ERROR;
    }
    ret = pthread_key_create(&g_log_data_thread_key, NULL);
    if (ret != 0) {
        perror("create pthread_key failed");
        return LOG_CREATE_KEY_FAIL;
    }
    // create logging thread
    ret = pthread_create(&g_log_thread, NULL, log_thread_loop, NULL);
    if (ret) {
        perror("error create log thread");
        return LOG_CREATE_THREAD_FAIL;
    }
    return 0;
}

int log_close() {
    g_log_running = false;
    int ret = pthread_join(g_log_thread, NULL);
    if (ret) {
        perror("error join log thread");
        return -1;
    } else {
        return 0;
    }
}

int log_join() {
    //g_log_running = false;
    int ret = pthread_join(g_log_thread, NULL);
    if (ret) {
        perror("error join log thread");
        return -1;
    } else {
        return 0;
    }
}

}  // namespace zf
