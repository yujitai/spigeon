/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file key_lock.h
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2012/12/05 12:01:28
 * @brief 
 **/

#ifndef __STORE_UTIL_KEY_LOCK_H__
#define __STORE_UTIL_KEY_LOCK_H__

#include <stdint.h>
#include <pthread.h>
#include <string>

#include "util/hash.h"
#include "util/slice.h"
#include "util/zframework_define.h"

namespace zf {

class KeyLock {
private:
    const static uint32_t DEFAULT_BUCKET_POWER      = 16;

public:
    KeyLock(uint32_t bucket_power = DEFAULT_BUCKET_POWER);
    ~KeyLock();

    int lock(const Slice &key) {
        ASSERT_EQUAL_0(pthread_mutex_lock(&_bucket_list[hash(key.data(), key.size()) & _bucket_mask]));
        return 0;
    }

    int unlock(const Slice &key) {
        ASSERT_EQUAL_0(pthread_mutex_unlock(&_bucket_list[hash(key.data(), key.size()) & _bucket_mask]));
        return 0;
    }

    int try_lock(const Slice &key, bool *try_succeed) {
        int ret = pthread_mutex_trylock(&_bucket_list[hash(key.data(), key.size()) & _bucket_mask]);
        ASSERT(ret == 0 || ret == EBUSY);
        if (ret == 0) {
            *try_succeed = true;
            return 0;
        } else if (ret == EBUSY) {
            *try_succeed = false;
            return 0;
        } else {
            return ret; // should not happen
        }
    }

private:
    const uint32_t _bucket_num;
    const uint32_t _bucket_mask;

    pthread_mutex_t *_bucket_list;
};

class KeyLockHolder {
public:
    KeyLockHolder(KeyLock* key_lock, const Slice& key) : _key_lock(key_lock), _key(key) { _key_lock->lock(_key); }
    ~KeyLockHolder() { _key_lock->unlock(_key); }

private:
    KeyLock* _key_lock;
    const Slice& _key;
};

} // namespace zf

#endif //__STORE_UTIL_KEY_LOCK_H__

