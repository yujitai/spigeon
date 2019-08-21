/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file key_lock.cpp
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2012/12/05 12:36:37
 * @brief 
 **/

#include "key_lock.h"

namespace zf {

KeyLock::KeyLock(uint32_t bucket_power)
    : _bucket_num(1 << bucket_power),
      _bucket_mask(_bucket_num - 1),
      _bucket_list(NULL)
{
    _bucket_list = new (std::nothrow) pthread_mutex_t[_bucket_num];
    ASSERT(_bucket_list);
    for (uint32_t i = 0; i < _bucket_num; ++i) {
        ASSERT_EQUAL_0(pthread_mutex_init(&_bucket_list[i], NULL));
    }
}

KeyLock::~KeyLock() {
    if (_bucket_list) {
        for (uint32_t i = 0; i < _bucket_num; ++i) {
            pthread_mutex_destroy(&_bucket_list[i]);
        }
        delete [] _bucket_list;
        _bucket_list = NULL;
    }
}

} // namespace zf


