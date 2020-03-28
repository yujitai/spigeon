/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file Lock.h
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2012/11/21 12:22:57
 * @brief 
 **/

#ifndef __STORE_UTIL_LOCK_H__
#define __STORE_UTIL_LOCK_H__

#include <pthread.h>

#include "util/zframework_define.h"
#include "util/log.h"

namespace zf {

class CondVar;
class Mutex {
public:
    Mutex()             { ASSERT_EQUAL_0(pthread_mutex_init(&_m, NULL)); }
    ~Mutex()            { ASSERT_EQUAL_0(pthread_mutex_destroy(&_m)); }
    void lock()         { ASSERT_EQUAL_0(pthread_mutex_lock(&_m)); }
    void unlock()       { ASSERT_EQUAL_0(pthread_mutex_unlock(&_m)); }
private:
    friend class CondVar;
    pthread_mutex_t _m;
};

class MutexHolder {
public:
    MutexHolder(Mutex &m) : _m(m) { _m.lock(); }
    ~MutexHolder()      { _m.unlock(); }
private: 
    Mutex &_m;
};

class CondVar {
public:
    CondVar(Mutex &m) : _m(m) { ASSERT_EQUAL_0(pthread_cond_init(&_c, NULL)); }
    ~CondVar()          { ASSERT_EQUAL_0(pthread_cond_destroy(&_c)); }

    void wait()         { ASSERT_EQUAL_0(pthread_cond_wait(&_c, &_m._m)); }
    void signal()       { ASSERT_EQUAL_0(pthread_cond_signal(&_c)); }
    void broadcast()    { ASSERT_EQUAL_0(pthread_cond_broadcast(&_c)); }
private:
    Mutex &_m;
    pthread_cond_t _c;
};

class RWLock {
public:
    RWLock()            { ASSERT_EQUAL_0(pthread_rwlock_init(&_l, NULL)); }
    ~RWLock()           { ASSERT_EQUAL_0(pthread_rwlock_destroy(&_l)); }
    void rdlock()       { ASSERT_EQUAL_0(pthread_rwlock_rdlock(&_l)); }
    void tryrdlock()    { ASSERT_EQUAL_0(pthread_rwlock_tryrdlock(&_l)); }
    void wrlock()       { ASSERT_EQUAL_0(pthread_rwlock_wrlock(&_l)); }
    void unlock()       { ASSERT_EQUAL_0(pthread_rwlock_unlock(&_l)); }
private:
    pthread_rwlock_t _l;
};

class RLockHolder {
public:
    RLockHolder(RWLock &l) : _l(l) { _l.rdlock(); }
    ~RLockHolder()       { _l.unlock(); }
private:
    RWLock &_l;
};

class WLockHolder {
public:
    WLockHolder(RWLock &l) : _l(l) { _l.wrlock(); }
    ~WLockHolder()      { _l.unlock(); }
private:
    RWLock &_l;
};

} // namespace zf

#endif //__STORE_UTIL_LOCK_H__


