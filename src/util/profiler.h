/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file profiler.h
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2012/12/06 21:20:55
 * @brief 
 **/

#ifndef __STORE_UTIL_PROFILER_H__
#define __STORE_UTIL_PROFILER_H__

#include <sys/time.h>

namespace store {

#define PROF_DEFAULT_TIME 100
#ifdef ENABLE_PROF
#define PROF_START(name, slow_time_us) Profiler p##name(#name, slow_time_us)
#define PROF_END(name) p##name.end()
#define PROF_KILL(name, slow_time_us) Profiler p##name(#name, slow_time_us, true)
#define PROF_HOLDER(name, slow_time_us) Profiler p##name(#name, slow_time_us)
#else
#define PROF_START(name, slow_time_us)
#define PROF_END(name)
#define PROF_KILL(name, slow_time_us) 
#define PROF_HOLDER(name, slow_time_us) 
#endif

class Profiler {
public:
    Profiler(const char *name, int slow_time_us = PROF_DEFAULT_TIME, bool kill = false)
        : _name(name), _slow_time_us(slow_time_us), _start(current_time_us()), _end(0), _kill(kill) {
        //log_warning("profiler[%s] start", _name);
    }

    ~Profiler() {
        end();
    }

    void end() {
        if (_end == 0) {
            _end = current_time_us();
            if ((_end - _start) >= _slow_time_us) {
                if (_kill) {
                    abort();
                }
                log_warning("slow profiler[%s] %luus", _name, _end - _start);
            }
        }
    }

private:
    static uint64_t current_time_us() {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000000 + tv.tv_usec;
        //return EventLoop::current_time();
    }

    const char *_name;
    uint64_t _slow_time_us;
    uint64_t _start;
    uint64_t _end;
    bool _kill;
};

} // end of namespace

#endif //__STORE_UTIL_PROFILER_H__

